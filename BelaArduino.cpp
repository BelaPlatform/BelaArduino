#include "BelaArduino.h"
#include "BelaMsg.h"
#include "Arduino.h"
#include "PdArduino.h"
#include <Bela.h>

// These defines may be overridden at runtime
// according to settings
#define ENABLE_LIBPD
#define ENABLE_CONTROL_PANEL
#define ENABLE_GUI
#define ENABLE_SHIFTOUT
#define CONTROL_PANEL_AUDIO
static BelaArduinoSettings settings;

#ifdef ENABLE_CONTROL_PANEL
#define WATCHER_DISABLE_DEFAULT
#include <libraries/Watcher/Watcher.h>
static Gui wmGui;
static WatcherManager wm(wmGui);
#endif // ENABLE_CONTROL_PANEL
#ifdef ENABLE_GUI
#include <libraries/Gui/Gui.h>
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
#ifdef ENABLE_CONTROL_PANEL
static int gControlPanelConnected;
static Watcher<uint32_t>* wdigital;
static std::vector<Watcher<float>*> wAnalogIn;
static std::vector<Watcher<float>*> wAnalogOut;
#ifdef CONTROL_PANEL_AUDIO
static std::vector<Watcher<float>*> wAudioIn;
static std::vector<Watcher<float>*> wAudioOut;
std::vector<Watcher<float>*> wEnvIn;
std::vector<Watcher<float>*> wEnvOut;
std::vector<RmsDetector> rmsIn;
std::vector<RmsDetector> rmsOut;
#endif // CONTROL_PANEL_AUDIO
#endif // ENABLE_CONTROL_PANEL

static void ArduinoSetup()
{
	setup();
}

static BelaMsgParser nonRtParser(belaArduinoPipe, false);
static void ArduinoLoop(void*)
{
	while(!Bela_stopRequested())
	{
		// we check this beforehand to avoid reading overhead
		// if no callback is defined
		if(pdReceiveMsg)
		{
			BelaMsgParser::Parsed& p = nonRtParser.process();
			if(p.good)
			{
				if(kBelaReceiverArduino == p.rec && pdReceiveMsg)
				{
					bool err = false;
					char const* str;
					if(p.isString())
						str = p.popString();
					else
						err = true;
					size_t length = p.numTags - 1;
					float data[length];
					for(size_t n = 0; n < length && !err; ++n)
					{
						if(p.isA("f"))
							p.pop(data[n]);
						else
							err = true;
					}
					if(!err)
						pdReceiveMsg(str, data, length);
				}
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

#ifdef ENABLE_LIBPD
int BelaArduino_floatHook(const char* source, float value)
{
	if(strcmp(source, "bela_arduino"))
		return 1;
	if(pdReceiveMsg)
		msgSendToArduino("float", &value, 1);
	return 0;
}

int BelaArduino_listHook(const char* source, int argc, t_atom *argv)
{
	return BelaArduino_messageHook(source, "list", argc, argv);
}

int BelaArduino_messageHook(const char* source, const char *symbol, int argc, t_atom *argv)
{
	if(strcmp(source, "bela_arduino"))
		return 1;
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
	return 0;
}

static std::vector<char> selector(1000);
static std::vector<char> type(1000);
#endif // ENABLE_LIBPD
static BelaMsgParser rtParser(belaArduinoPipe, true);
void processPipe(BelaContext* context)
{
	while(1) // breaks when no data is available right now
	{
		BelaMsgParser::Parsed& p = rtParser.process();
		if(!p.good)
			break;
		// fixed-type messages first
		if(kBelaReceiverShiftOut == p.rec)
		{
			if(!p.matches("hhhhj"))
			{
				rt_fprintf(stderr, "shiftOut: malformed message\n");
				continue;
			}
			ShiftRegister::Pins pins;
			pins.data = p.popNumeric();
			pins.clock = p.popNumeric();
			pins.latch = p.popNumeric();
			uint8_t numBits = p.popNumeric();
			uint32_t bits = p.popNumeric();
			shiftOutBits.resize(numBits);
#if 0
			rt_printf("data: %d, clock: %d, latch: %d, numBits: %d, bits:  %#010x\n",
					pins.data, pins.clock, pins.latch, numBits, bits);
#endif
			for(size_t n = 0; n < shiftOutBits.size(); ++n)
				shiftOutBits[n] = bits & (1 << n);
			shiftRegisterOut.setup(pins, numBits);

			shiftRegisterOut.setData(shiftOutBits);
			shiftOutInProgress = true;
			continue;
		}
		if(kBelaReceiverDigital == p.rec)
		{
			if(!p.matchesPart("jj"))
			{
				continue;
			}
			enum DigitalMode mode = (DigitalMode)p.popType<unsigned>();
			unsigned int channel = p.popType<unsigned>();
			if(kDigitalModePwm == mode)
			{
				if(!p.matchesRem("ff"))
				{
					rt_fprintf(stderr, "digital pwm: malformed message\n");
					continue;
				}
				float width = p.popType<float>();
				float freq = p.popType<float>();
				unsigned int period = int(context->audioSampleRate / freq + 0.5f);
				uint32_t pwmWidth = width * period;
				digital[channel].value = pwmWidth;
				digital[channel].period = period;
				//rt_printf("width: %f, freq %f, value: %d, period: %d\n" ,  width, freq, pwmWidth, period);
			}
			digital[channel].mode = mode;
		}
#ifdef ENABLE_LIBPD
		if(kBelaReceiverPd == p.rec)
		{
			bool isList = false;
			if(p.numTags < 2 || !p.isString(0))
			{
				rt_fprintf(stderr, "Messages to Pd need to have at least two elements, the first of which should be a s\n");
				continue;
			}
			// numTags is number of elements in the incoming message.
			// When sending to Pd, the first element is the receiver name,
			// so it doesn't count towards message length.
			size_t numElements = p.numTags - 1;
			// Additionally, if the second element is a string, then it
			// becomes the message "type" (see libpd.h for details),
			// otherwise it is sent as a list ("untyped").
			isList = !p.isString(1);
			if(isList)
				numElements -= 1;
			libpd_start_message(numElements);
			size_t n = 0;
			while(!p.done())
			{
				if(p.isA("f"))
				{
					float val = p.popNumeric();
					libpd_add_float(val);
				} else
				if(p.isString())
				{
					const char* str = p.popString();
					if(0 == n)
						selector.assign(str, str + strlen(str) + 1);
					else if(1 == n)
						type.assign(str, str + strlen(str) + 1);
					else
						libpd_add_symbol(str);
				} else {
					rt_fprintf(stderr, "Unexpected type in message for Pd: '%c'\n", p.tag());
					break;
				}
				++n;
			}
			if(isList)
				libpd_finish_list(selector.data());
			else
				libpd_finish_message(selector.data(), type.data());
		}
#endif // ENABLE_LIBPD
	}
}

static void printGui(const std::string& desc, int port)
{
	printf("%s GUI at bela.local/gui", desc.c_str());
	if(5555 == port)
		printf("\n");
	else
		printf("?wsPort=%d\n", port);
}

bool BelaArduino_setup(BelaContext* context, void*, const BelaArduinoSettings& settings)
{
	::settings = settings;
#ifdef ENABLE_LIBPD
	libpd_bind("bela_arduino");
#endif // ENABLE_LIBPD
#ifdef ENABLE_SHIFTOUT
	ShiftRegister::Pins pins {}; // dummy
	shiftRegisterOut.setup(pins, shiftOutBits.size()); // preallocat
#endif // ENABLE_SHIFTOUT
#ifdef ENABLE_GUI
	if(settings.useGui)
	{
		gui.setup(context->projectName, settings.guiPort);
		printGui("BelaArduino: sketch.js", settings.guiPort);
	}
#endif // ENABLE_GUI
#ifdef ENABLE_CONTROL_PANEL
	if(settings.useControlPanel)
	{
		wm.setup(context->audioSampleRate);
		std::string controlPanelSketch = "/libraries/BelaArduino/sketch-control-panel.js";
		wm.getGui().setup(controlPanelSketch, settings.controlPanelPort);
		printGui("BelaArduino: control panel", settings.controlPanelPort);

		if(context->digital)
			wdigital = new Watcher<uint32_t>("digital", wm);
		wAnalogIn.resize(context->analogInChannels);
		for(size_t n = 0; n < context->analogInChannels; ++n)
			wAnalogIn[n] = new Watcher<float>({std::string("analogIn") + std::to_string(n), wm});
		wAnalogOut.resize(context->analogOutChannels);
		for(size_t n = 0; n < context->analogOutChannels; ++n)
			wAnalogOut[n] = new Watcher<float>({std::string("analogOut") + std::to_string(n), wm});

#ifdef CONTROL_PANEL_AUDIO
		if(settings.controlPanelAudio)
		{
			wAudioIn.resize(context->audioInChannels);
			wEnvIn.resize(context->audioInChannels);
			rmsIn.resize(context->audioInChannels);
			float decay = 0.05;
			for(size_t n = 0; n < context->audioInChannels; ++n)
			{
				wAudioIn[n] = new Watcher<float>({std::string("audioIn") + std::to_string(n), wm});
				wEnvIn[n] = new Watcher<float>({std::string("envIn") + std::to_string(n), wm});
				rmsIn[n].setDecay(decay);
			}

			wAudioOut.resize(context->audioOutChannels);
			wEnvOut.resize(context->audioOutChannels);
			rmsOut.resize(context->audioOutChannels);
			for(size_t n = 0; n < context->audioOutChannels; ++n)
			{
				wAudioOut[n] = new Watcher<float>({std::string("audioOut") + std::to_string(n), wm});
				wEnvOut[n] = new Watcher<float>({std::string("envOut") + std::to_string(n), wm});
				rmsOut[n].setDecay(decay);
			}
		}
#endif // CONTROL_PANEL_AUDIO
	}
#endif // ENABLE_CONTROL_PANEL
	analogIn.resize(context->analogInChannels);
	analogOut.resize(context->analogOutChannels);
	digital.resize(context->digitalChannels);
	for(size_t c = 0; c < digital.size(); ++c)
	{
		digital[c].value = 0;
		digital[c].mode = kDigitalModeIgnored;
	}
	belaArduinoPipe.setup("BelaArduino");
	ArduinoSetup();
	processPipe(context);
	arduinoLoopTask = Bela_createAuxiliaryTask(ArduinoLoop, 0, "ArduinoLoop");
	return true;
}

void BelaArduino_renderTop(BelaContext* context, void*)
{
	processPipe(context);
#ifdef ENABLE_CONTROL_PANEL
	gControlPanelConnected = settings.useControlPanel && wm.getGui().numConnections();
	if(gControlPanelConnected)
	{
		wm.tick(context->audioFramesElapsed);
		for(size_t n = 0; n < context->audioFrames; ++n) // assumes uniform sampling rate and non-interleaved buffers
		{
			for(size_t c = 0; c < context->analogInChannels && c < wAnalogIn.size(); ++c)
			{
				if(settings.controlPanelOnce)
				{
					if(n > 0)
						break;
				}
				float value = analogRead(context, n, c);
				wAnalogIn[c]->set(value);
			}
#ifdef CONTROL_PANEL_AUDIO
			if(settings.controlPanelAudio)
			{
				for(size_t c = 0; c < context->audioInChannels && c < wAudioIn.size(); ++c)
				{
					float value = audioRead(context, n, c);
					wAudioIn[c]->set(value);
					rmsIn[c].process(value);
					wEnvIn[c]->set(rmsIn[c].getEnv());
				}
			}
#endif // CONTROL_PANEL_AUDIO
			if(settings.controlPanelOnce)
			{
				if(n > 0)
					continue;
			}
			wdigital->set(context->digital[n]);
		}
	}
#endif // ENABLE_CONTROL_PANEL
	for(size_t c = 0; c < context->analogInChannels; ++c)
		analogIn[c] = analogRead(context, 0, c);
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

void BelaArduino_renderBottom(BelaContext* context, void*)
{
	for(size_t c = 0; c < context->digitalChannels; ++c)
	{
		// mode is set from Arduino
		if(kDigitalModeIgnored == digital[c].mode)
			continue;
		int mode = (kDigitalModeInput == digital[c].mode) ? INPUT : OUTPUT;
		pinMode(context, 0, c, mode);
		if(OUTPUT == mode)
		{
			if(kDigitalModeOutput == digital[c].mode)
				digitalWrite(context, 0, c, digital[c].value);
			if(kDigitalModePwm == digital[c].mode)
			{
				auto& count = digital[c].count;
				const auto& value = digital[c].value;
				const auto& period = digital[c].period;
				for(size_t n = 0; n < context->digitalFrames; ++n)
				{
					bool out = value > count;
					digitalWriteOnce(context, n, c, out);
					count++;
					if(count >= period)
						count = 0;
				}
			}
		}
	}
#ifdef ENABLE_SHIFTOUT
	if(shiftOutInProgress)
	{
		shiftRegisterOut.process(context);
		if(shiftRegisterOut.dataReady())
			shiftOutInProgress = false;
	}
#endif // ENABLE_SHIFTOUT
#ifdef ENABLE_CONTROL_PANEL
	if(gControlPanelConnected)
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
			if(settings.controlPanelOnce)
			{
				if(n > 0)
					break;
			}
			for(size_t c = 0; c < context->analogOutChannels; ++c)
				wAnalogOut[c]->set(context->analogOut[c * context->analogFrames + n]);
		}
		for(size_t c = 0; c < context->analogOutChannels; ++c)
		{
			if(!wAnalogOut[c]->hasLocalControl())
				analogWrite(context, 0, c, *wAnalogOut[c]);
		}
#ifdef CONTROL_PANEL_AUDIO
		if(settings.controlPanelAudio)
		{
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
		}
#endif // CONTROL_PANEL_AUDIO
	}
#endif // ENABLE_CONTROL_PANEL
}

void BelaArduino_cleanup(BelaContext* context, void*)
{
#ifdef ENABLE_CONTROL_PANEL
	delete wdigital;
	for(auto& a : wAnalogIn)
		delete a;
	for(auto& a : wAnalogOut)
		delete a;
#ifdef CONTROL_PANEL_AUDIO
	for(auto& a : wAudioIn)
		delete a;
	for(auto& a : wAudioOut)
		delete a;
	for(auto& a : wEnvIn)
		delete a;
	for(auto& a : wEnvOut)
		delete a;
#endif // CONTROL_PANEL_AUDIO
#endif // ENABLE_CONTROL_PANEL
}
