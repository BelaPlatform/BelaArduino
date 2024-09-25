#include <libraries/BelaLibpd/BelaLibpd.h>
#include <BelaArduino.h>

bool setup(BelaContext* context, void* userData)
{
	BelaLibpdSettings settings;
	settings.useIoThreaded = false;
	settings.useGui = false;
	settings.messageHook = BelaArduino_messageHook;
	settings.listHook = BelaArduino_listHook;
	settings.floatHook = BelaArduino_floatHook;
	if(!BelaLibpd_setup(context, userData, settings))
		return false;
	if(!BelaArduino_setup(context, userData))
		return false;
	return true;
}

void render(BelaContext* context, void* userData)
{
	BelaArduino_renderTop(context, userData);
	BelaLibpd_render(context, userData);
	BelaArduino_renderBottom(context, userData);
}

void cleanup(BelaContext* context, void* userData)
{
	BelaArduino_cleanup(context, userData);
	BelaLibpd_cleanup(context, userData);
}
