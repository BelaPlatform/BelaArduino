#include "BelaArduino.h"
#include "BelaMsg.h"
#include "Arduino.h"
#include <Bela.h>
#define ENABLE_WATCHER

#ifdef ENABLE_WATCHER
#include "Watcher.h"
#endif // ENABLE_WATCHER

std::vector<DigitalChannel> digital;
Pipe belaArduinoPipe;
std::vector<float> analogIn;
std::vector<float> analogOut;

#ifdef ENABLE_WATCHER
static Watcher<uint32_t>* wdigital;
static std::vector<Watcher<float>*> wAnalogIn;
static std::vector<Watcher<float>*> wAnalogOut;
#endif // ENABLE_WATCHER

static void ArduinoSetup()
{
	setup();
}
static void ArduinoLoop(void*)
{
	while(!Bela_stopRequested())
		loop();
}
static AuxiliaryTask arduinoLoopTask;

#include <libraries/libpd/libpd.h>

static std::vector<char> selector(1000);
static std::vector<char> type(1000);
void processPipe()
{
	static enum WaitingFor {
		kHeader,
		kPayload
	} waitingFor = kHeader;
	static size_t size;
	while(1) // breaks when no data is available right now
	{
		if(kHeader == waitingFor)
		{
			// The header is a size_t containing the message size
			if(1 == belaArduinoPipe.readRt(size))
				waitingFor = kPayload;
			else
				break;
		}
		if(kPayload == waitingFor)
		{
			// the message is:
			// 1 byte: n of tags
			// n tags
			// arguments (without padding, possibly unaligned)
			uint8_t msg[size];;
			int ret;
			if((ret = belaArduinoPipe.readRt(msg, size)) != size)
				break;
			waitingFor = kHeader;
			uint8_t numTags = msg[0];
			const uint8_t* tags = msg + 1;
			if(numTags < 2 || 's' != tags[0])
			{
				rt_fprintf(stderr, "Messages to Pd need to have at least two elements, the first of which should be a s\n");
			}
			// numTags is number of elements in the incoming message.
			// When sending to Pd, the first element is the receiver name,
			// so it doesn't count towards message length.
			size_t numElements = numTags - 1;
			// Additionally, if the second element is a string, then it
			// becomes the message "type" (see libpd.h for details),
			// otherwise it is sent as a list ("untyped").
			bool isList = ('s' != tags[1]);
			if(isList)
				numElements -= 1;
			libpd_start_message(numElements);
			uint8_t nextArg = 1 + numTags;
			for(size_t n = 0; n < numTags; ++n)
			{
				if('f' == tags[n])
				{
					float val;
					memcpy(&val, msg + nextArg, sizeof(val));
					nextArg += sizeof(val);
					libpd_add_float(val);
				}
				if('s' == tags[n])
				{
					// look for end of null-delimited string
					const char* str = (const char*)msg + nextArg;
					size_t maxLen = sizeof(msg) - nextArg - 1;
					size_t len = strnlen(str, maxLen);
					if('\0' != str[len])
					{
						rt_fprintf(stderr, "Malformed message\n");
						break;
					}
					nextArg += len + 1;
					if(0 == n)
						selector.assign(str, str + len + 1);
					else if(1 == n)
						type.assign(str, str + len + 1);
					else
						libpd_add_symbol(str);
				}
			}
			if(isList)
				libpd_finish_list(selector.data());
			else
				libpd_finish_message(selector.data(), type.data());
		}
	}
}

bool BelaArduino_setup(BelaContext* context)
{
#ifdef ENABLE_WATCHER
	Bela_getDefaultWatcherManager()->setup(context->audioSampleRate);
	Bela_getDefaultWatcherManager()->getGui().setup(context->projectName);
	if(context->digital)
		wdigital = new Watcher<uint32_t>("digital");
	wAnalogIn.resize(context->analogInChannels);
	for(size_t n = 0; n < context->analogInChannels; ++n)
		wAnalogIn[n] = new Watcher<float>({std::string("analogIn") + std::to_string(n)});
	wAnalogOut.resize(context->analogOutChannels);
	for(size_t n = 0; n < context->analogOutChannels; ++n)
		wAnalogOut[n] = new Watcher<float>({std::string("analogOut") + std::to_string(n)});
#endif // ENABLE_WATCHER
	analogIn.resize(context->analogInChannels);
	analogOut.resize(context->analogOutChannels);
	digital.resize(context->digitalChannels);
	for(size_t c = 0; c < digital.size(); ++c)
	{
		digital[c].value = 0;
		digital[c].mode = kDigitalModeInput;
	}
	belaArduinoPipe.setup("BelaArduino");
	ArduinoSetup();
	processPipe();
	arduinoLoopTask = Bela_createAuxiliaryTask(ArduinoLoop, 94, "ArduinoLoop");
	return true;
}

void BelaArduino_renderTop(BelaContext* context)
{
	processPipe();
#ifdef ENABLE_WATCHER
	Bela_getDefaultWatcherManager()->tick(context->audioFramesElapsed);
	for(size_t c = 0; c < context->analogInChannels && c < wAnalogIn.size(); ++c)
	{
		float value = analogRead(context, 0, c);
		wAnalogIn[c]->set(value);
		analogIn[c] = value;
	}
	wdigital->set(context->digital[0]);
#endif // ENABLE_WATCHER
	for(size_t c = 0; c < context->digitalChannels; ++c)
	{
		if(kDigitalModeInput == digital[c].mode)
			digital[c].value = digitalRead(context, 0, c);
	}
	static bool inited = false;
	if(!inited)
		Bela_scheduleAuxiliaryTask(arduinoLoopTask);
	inited = true;
}

void BelaArduino_renderBottom(BelaContext* context)
{
	static uint32_t pwmClock = 0;
	for(size_t c = 0; c < context->digitalChannels; ++c)
	{
		// mode is set from Arduino
		int mode = (kDigitalModeInput == digital[c].mode) ? INPUT : OUTPUT;
		pinMode(context, 0, c, mode);
		if(OUTPUT == mode)
		{
			if(kDigitalModeOutput == digital[c].mode)
				digitalWrite(context, 0, c, digital[c].value);
			if(kDigitalModePwm == digital[c].mode)
			{
				uint8_t clock = pwmClock;
				for(size_t n = 0; n < context->digitalFrames; ++n)
				{
					clock = (clock + 1) & (kPwmPeriod - 1);
					bool value = digital[c].value > clock;
					digitalWriteOnce(context, n, c, value);
				}
			}
		}
	}
	pwmClock = (pwmClock + context->digitalFrames) & (kPwmPeriod - 1);
#ifdef ENABLE_WATCHER
	if(!wdigital->hasLocalControl())
	{
		// this is actually overwriting the inputs as well.
		// TODO: we should only set obeying the _mask_
		// TODO: this will break the PWM
		for(size_t n = 0; n < context->digitalFrames; ++n)
			context->digital[n] = *wdigital;
	}
	for(size_t c = 0; c < context->analogOutChannels; ++c)
	{
		if(!wAnalogOut[c]->hasLocalControl())
			analogWrite(context, 0, c, *wAnalogOut[c]);
	}
#endif // ENABLE_WATCHER
}

void BelaArduino_cleanup(BelaContext* context)
{
#ifdef ENABLE_WATCHER
	delete wdigital;
	for(auto& a : wAnalogIn)
		delete a;
	for(auto& a : wAnalogOut)
		delete a;
#endif // ENABLE_WATCHER
}
