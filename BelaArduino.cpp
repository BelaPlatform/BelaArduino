#include "BelaArduino.h"
#include "BelaMsg.h"
#include "Arduino.h"
#include "PdArduino.h"
#include <Bela.h>
#define ENABLE_WATCHER
#define ENABLE_GUI
#define ENABLE_SHIFTOUT
#define WATCH_ONCE
#define WATCH_AUDIO

#ifdef ENABLE_WATCHER
#include "Watcher.h"
#endif // ENABLE_WATCHER
#ifdef ENABLE_GUI
#include "Gui.h"
Gui gui;
#endif // ENABLE_GUI
#ifdef ENABLE_SHIFTOUT
#include <libraries/ShiftRegister/ShiftRegister.h>
static ShiftRegisterOut shiftRegisterOut;
static std::vector<bool> shiftOutBits(32);
static bool shiftOutInProgress = false;
#endif // ENABLE_SHIFTOUT

#include <math.h>
#include <array>
#include <algorithm>
/// A constant-CPU, continuous output, CPU- and memory-efficient RMS detector
class RmsDetector {
public:
	RmsDetector()
	{
		setup();
	}
	void setup()
	{
		x0 = x1 = y1 = 0;
		env = 0;
		rms = 0;
		rmsAcc = 0;
		rmsIdx = 0;
		rmsBuffer.fill(0);
		setDecay(0.1);
	}
	void process(float input)
	{
		//high pass
		// [b, a] = butter(1, 10/44250, 'high')
		const float b0 = 0.999630537400518;
		const float b1 = -0.999630537400518;
		const float a1 = -0.999261074801036;
		float x0 = input;
		float y = x1 * b1 + x0 * b0 - y1 * a1;
		x1 = x0;
		y1 = y;
		float in = std::min(1.f, abs(y));
		// compute RMS
		uint16_t newRmsVal = in * in * 65535.f;
		uint16_t oldRmsVal = rmsBuffer[rmsIdx];
		rmsAcc = rmsAcc + newRmsVal - oldRmsVal;
		rmsBuffer[rmsIdx] = newRmsVal;
		rmsIdx++;
		if(rmsIdx >= rmsBuffer.size())
			rmsIdx = 0;
		float envIn = std::min(1.f, rmsAcc / 65535.f / float(rmsBuffer.size()));
		rms = envIn;
		// peak envelope detector
		if(envIn > env)
		{
			env = envIn;
		} else {
			env = env * decay;
		}
	}
	void setDecay(float cutoff)
	{
		// wrangling the range to make it somehow useful
		// TODO: put more method into this
		float par = cutoff;
		float bottom = 0.000005;
		float top = 0.0005;
		// we want par to be between about bottom and top
		// we want values to be closer to the bottom when the slider is higher
		par = 1.f - par;
		// We want changes to the input to have smaller effect closer to 0.000005
		par = powf(par, 3) * (top - bottom) + bottom;
		decay = 1.f - par;
	}
	float getRms()
	{
		return rms;
	}
	float getEnv()
	{
		return env;
	}
private:
	float env;
	float rms;
	uint32_t rmsAcc;
	float decay;
	size_t rmsIdx = 0;
	float x0, x1, y1;
	std::array<uint16_t,512> rmsBuffer;
};

std::vector<DigitalChannel> digital;
Pipe belaArduinoPipe;
std::vector<float> analogIn;
std::vector<float> analogOut;
#ifdef ENABLE_WATCHER
static int gWatcherConnected;
static Watcher<uint32_t>* wdigital;
static std::vector<Watcher<float>*> wAnalogIn;
static std::vector<Watcher<float>*> wAnalogOut;
#ifdef WATCH_AUDIO
static std::vector<Watcher<float>*> wAudioIn;
static std::vector<Watcher<float>*> wAudioOut;
std::vector<Watcher<float>*> wEnvIn;
std::vector<Watcher<float>*> wEnvOut;
std::vector<RmsDetector> rmsIn;
std::vector<RmsDetector> rmsOut;
#endif // WATCH_AUDIO
#endif // ENABLE_WATCHER

static void ArduinoSetup()
{
	setup();
}

static BelaMsgParser nonRtParser(belaArduinoPipe, false);
static void ArduinoLoop(void*)
{
	while(!Bela_stopRequested())
	{
		if(pdReceiveMsg)
		{
			BelaMsgParser::Parsed& p = nonRtParser.process();
			if(p.good)
			{
				// TODO: retrieve data and call pdReceiveMsg()
			}
		}
		loop();
	}
}
void msgSendToArduino(const char* str, const float* vals, size_t count)
{
	BelaSourceThread t = kBelaSourceThreadAudio;
	msgInit(t, kBelaReceiverArduino, count + 1);
	msgAddFS(t, str);
	for(size_t n = 0; n < count; ++n)
		msgAdd(t, vals[n]);
	msgSend(t);
}
static AuxiliaryTask arduinoLoopTask;

void BelaArduino_floatHook(float value)
{
	if(pdReceiveMsg)
		msgSendToArduino("float", &value, 1);
}

void BelaArduino_listHook(int argc, t_atom *argv)
{
	BelaArduino_messageHook("list", argc, argv);
}

void BelaArduino_messageHook(const char *symbol, int argc, t_atom *argv)
{
	float payload[argc];
	for(size_t n = 0; n < argc; ++n)
	{
		if(libpd_is_float(argv + n))
			payload[n] = libpd_get_float(argv + n);
		else
			payload[n] = -1;
	}
	if(pdReceiveMsg)
		msgSendToArduino(symbol, payload, argc);
}

static std::vector<char> selector(1000);
static std::vector<char> type(1000);
static BelaMsgParser rtParser(belaArduinoPipe, true);
void processPipe()
{
	while(1) // breaks when no data is available right now
	{
		BelaMsgParser::Parsed& p = rtParser.process();
		if(!p.good)
			break;
		uint8_t* msg = p.msg.data();
		const uint8_t* tags = p.tags;
		uint8_t numTags = p.numTags;
		uint8_t nextArg = p.nextArg;
		BelaReceiver rec = p.rec;
			// fixed-type messages first
			if(kBelaReceiverShiftOut == rec)
			{
				if(strncmp((const char*)tags, "hhhhj", 5))
					rt_fprintf(stderr, "shiftOut: malformed message\n");
				ShiftRegister::Pins pins;
				pins.data = *(msg + nextArg++);
				pins.clock = *(msg + nextArg++);
				pins.latch = *(msg + nextArg++);
				size_t numBits = *(msg + nextArg++);
				uint32_t bits;
				memcpy(&bits, msg + nextArg, sizeof(bits));
				shiftOutBits.resize(numBits);
				for(size_t n = 0; n < shiftOutBits.size(); ++n)
					shiftOutBits[n] = bits & (1 << n);
				shiftRegisterOut.setup(pins, numBits);

				shiftRegisterOut.setData(shiftOutBits);
				shiftOutInProgress = true;
				continue;
			}
			bool isList = false;
			if(kBelaReceiverPd == rec)
			{
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
				isList = ('s' != tags[1]);
				if(isList)
					numElements -= 1;
				libpd_start_message(numElements);
			}
			for(size_t n = 0; n < numTags; ++n)
			{
				if('f' == tags[n])
				{
					float val;
					memcpy(&val, msg + nextArg, sizeof(val));
					nextArg += sizeof(val);
					if(kBelaReceiverPd == rec)
					{
						libpd_add_float(val);
					}
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
					if(kBelaReceiverPd == rec)
					{
						if(0 == n)
							selector.assign(str, str + len + 1);
						else if(1 == n)
							type.assign(str, str + len + 1);
						else
							libpd_add_symbol(str);
					}
				}
			}
			if(kBelaReceiverPd == rec)
			{
				if(isList)
					libpd_finish_list(selector.data());
				else
					libpd_finish_message(selector.data(), type.data());
			}
	}
}

bool BelaArduino_setup(BelaContext* context)
{
	libpd_bind("bela_arduino");
#ifdef ENABLE_SHIFTOUT
	ShiftRegister::Pins pins {}; // dummy
	shiftRegisterOut.setup(pins, shiftOutBits.size()); // preallocat
#endif // ENABLE_SHIFTOUT
	if((context->analogFrames && context->analogFrames != context->audioFrames) || (context->digitalFrames && context->digitalFrames != context->audioFrames))
	{
		fprintf(stderr, "Error: analog, audio and digital must have the same sampling rate\n");
		return false;
	}
	if(context->flags & BELA_FLAG_INTERLEAVED)
	{
		fprintf(stderr, "Error: buffers most be non-interleaved\n");
		return false;
	}
#ifdef ENABLE_GUI
	gui.setup(context->projectName);
#endif // ENABLE_GUI
#ifdef ENABLE_WATCHER
	Bela_getDefaultWatcherManager()->setup(context->audioSampleRate);
#ifdef ENABLE_GUI
	Bela_getDefaultWatcherManager()->getGui().setup(context->projectName, 5556, "gui_watcher");
#else // ENABLE_GUI
	Bela_getDefaultWatcherManager()->getGui().setup(context->projectName);
#endif // ENABLE_GUI
	if(context->digital)
		wdigital = new Watcher<uint32_t>("digital");
	wAnalogIn.resize(context->analogInChannels);
	for(size_t n = 0; n < context->analogInChannels; ++n)
		wAnalogIn[n] = new Watcher<float>({std::string("analogIn") + std::to_string(n)});
	wAnalogOut.resize(context->analogOutChannels);
	for(size_t n = 0; n < context->analogOutChannels; ++n)
		wAnalogOut[n] = new Watcher<float>({std::string("analogOut") + std::to_string(n)});

#ifdef WATCH_AUDIO
	wAudioIn.resize(context->audioInChannels);
	wEnvIn.resize(context->audioInChannels);
	rmsIn.resize(context->audioInChannels);
	float decay = 0.05;
	for(size_t n = 0; n < context->audioInChannels; ++n)
	{
		wAudioIn[n] = new Watcher<float>({std::string("audioIn") + std::to_string(n)});
		wEnvIn[n] = new Watcher<float>({std::string("envIn") + std::to_string(n)});
		rmsIn[n].setDecay(decay);
	}

	wAudioOut.resize(context->audioOutChannels);
	wEnvOut.resize(context->audioOutChannels);
	rmsOut.resize(context->audioOutChannels);
	for(size_t n = 0; n < context->audioOutChannels; ++n)
	{
		wAudioOut[n] = new Watcher<float>({std::string("audioOut") + std::to_string(n)});
		wEnvOut[n] = new Watcher<float>({std::string("envOut") + std::to_string(n)});
		rmsOut[n].setDecay(decay);
	}
#endif // WATCH_AUDIO
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
	arduinoLoopTask = Bela_createAuxiliaryTask(ArduinoLoop, 0, "ArduinoLoop");
	return true;
}

void BelaArduino_renderTop(BelaContext* context)
{
	processPipe();
#ifdef ENABLE_WATCHER
	gWatcherConnected = Bela_getDefaultWatcherManager()->getGui().numConnections();
	if(gWatcherConnected)
	{
		Bela_getDefaultWatcherManager()->tick(context->audioFramesElapsed);
		for(size_t n = 0; n < context->audioFrames; ++n) // assumes uniform sampling rate and non-interleaved buffers
		{
			for(size_t c = 0; c < context->analogInChannels && c < wAnalogIn.size(); ++c)
			{
#ifdef WATCH_ONCE
				if(n > 0)
					break;
#endif // WATCH_ONCE
				float value = analogReadNI(context, n, c);
				wAnalogIn[c]->set(value);
			}
#ifdef WATCH_AUDIO
			for(size_t c = 0; c < context->audioInChannels && c < wAudioIn.size(); ++c)
			{
				float value = audioReadNI(context, n, c);
				wAudioIn[c]->set(value);
				rmsIn[c].process(value);
				wEnvIn[c]->set(rmsIn[c].getEnv());
			}
#endif // WATCH_AUDIO
#ifdef WATCH_ONCE
			if(n > 0)
				continue;
#endif // WATCH_ONCE
			wdigital->set(context->digital[n]);
		}
	}
#endif // ENABLE_WATCHER
	for(size_t c = 0; c < context->analogInChannels; ++c)
		analogIn[c] = analogReadNI(context, 0, c);
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
#ifdef ENABLE_SHIFTOUT
	if(shiftOutInProgress)
	{
		shiftRegisterOut.process(context);
		if(shiftRegisterOut.dataReady())
			shiftOutInProgress = false;
	}
#endif // ENABLE_SHIFTOUT
#ifdef ENABLE_WATCHER
	if(gWatcherConnected)
	{
		unsigned int mask;
		if((mask = wdigital->getMask()))
		{
			for(size_t n = 0; n < context->digitalFrames; ++n)
			{
				context->digital[n] &= ~mask;
				context->digital[n] |= *wdigital & mask;
			}
		}
		for(size_t n = 0; n < context->analogFrames; ++n)
		{
#ifdef WATCH_ONCE
			if(n > 0)
				break;
#endif // WATCH_ONCE
			for(size_t c = 0; c < context->analogOutChannels; ++c)
				wAnalogOut[c]->set(context->analogOut[c * context->analogFrames + n]);
		}
		for(size_t c = 0; c < context->analogOutChannels; ++c)
		{
			if(!wAnalogOut[c]->hasLocalControl())
				analogWriteNI(context, 0, c, *wAnalogOut[c]);
		}
#ifdef WATCH_AUDIO
		for(size_t n = 0; n < context->audioFrames; ++n)
		{
			for(size_t c = 0; c < context->audioOutChannels; ++c)
			{
				float val = context->audioOut[c * context->audioFrames + n];
				wAudioOut[c]->set(val);
				rmsOut[c].process(val);
				wEnvOut[c]->set(rmsOut[c].getEnv());
			}
		}
#endif // WATCH_AUDIO
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
#ifdef WATCH_AUDIO
	for(auto& a : wAudioIn)
		delete a;
	for(auto& a : wAudioOut)
		delete a;
	for(auto& a : wEnvIn)
		delete a;
	for(auto& a : wEnvOut)
		delete a;
#endif // WATCH_AUDIO
#endif // ENABLE_WATCHER
}
