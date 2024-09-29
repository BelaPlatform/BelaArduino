#include <libraries/BelaLibpd/BelaLibpd.h>
#include <BelaArduino.h>
#include "Watcher.h"
Watcher<float> myvar("myvar"); // this is the user-define watcher

bool setup(BelaContext* context, void* userData)
{
	Bela_getDefaultWatcherManager()->getGui().setup(std::string("/projects/") + context->projectName + "/upstream.js", 1234);
	BelaLibpdSettings settings;
	settings.useIoThreaded = false;
	settings.useGui = false;
	settings.messageHook = BelaArduino_messageHook;
	settings.listHook = BelaArduino_listHook;
	settings.floatHook = BelaArduino_floatHook;
	if(!BelaLibpd_setup(context, userData, settings))
		return false;
	BelaArduinoSettings arduinoSettings;
	if(!BelaArduino_setup(context, userData, arduinoSettings))
		return false;
	return true;
}

void render(BelaContext* context, void* userData)
{
	if(Bela_getDefaultWatcherManager()->getGui().numConnections())
		Bela_getDefaultWatcherManager()->tick(context->audioFramesElapsed);
	myvar = myvar + 0.001;
	if(myvar >= 1)
		myvar = 0;
	BelaArduino_renderTop(context, userData);
	BelaLibpd_render(context, userData);
	BelaArduino_renderBottom(context, userData);
}

void cleanup(BelaContext* context, void* userData)
{
	BelaArduino_cleanup(context, userData);
	BelaLibpd_cleanup(context, userData);
}
