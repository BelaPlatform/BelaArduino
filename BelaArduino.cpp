#include "BelaArduino.h"
#include "BelaMsg.h"
#include "Arduino.h"
#include <Bela.h>
#include "Watcher.h"

std::vector<DigitalChannel> digital;
Pipe belaArduinoPipe;
std::vector<float> analogIn;
std::vector<float> analogOut;

static Watcher<uint32_t>* wdigital;
static std::vector<Watcher<float>*> wAnalogIn;
static std::vector<Watcher<float>*> wAnalogOut;

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
bool BelaArduino_setup(BelaContext* context)
{
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
	analogIn.resize(context->analogInChannels);
	analogOut.resize(context->analogOutChannels);
	digital.resize(context->digitalChannels);
	for(size_t c = 0; c < digital.size(); ++c)
	{
		digital[c].value = 0;
		digital[c].mode = kDigitalModeInput;
	}
	ArduinoSetup();
	arduinoLoopTask = Bela_createAuxiliaryTask(ArduinoLoop, 94, "ArduinoLoop");
	return true;
}

void BelaArduino_renderTop(BelaContext* context)
{
	Bela_getDefaultWatcherManager()->tick(context->audioFramesElapsed);
	for(size_t c = 0; c < context->analogInChannels && c < wAnalogIn.size(); ++c)
	{
		float value = analogRead(context, 0, c);
		wAnalogIn[c]->set(value);
		analogIn[c] = value;
	}
	wdigital->set(context->digital[0]);
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
	for(size_t c = 0; c < context->digitalChannels; ++c)
	{
		// mode is set from Arduino
		int mode = (kDigitalModeInput == digital[c].mode) ? INPUT : OUTPUT;
		pinMode(context, 0, c, mode);
		if(OUTPUT == mode) {
			digitalWrite(context, 0, c, digital[c].value);
		}
	}
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
}

void BelaArduino_cleanup(BelaContext* context)
{
	delete wdigital;
}
