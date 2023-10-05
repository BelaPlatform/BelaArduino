#include <BelaArduino.h>

bool setup(BelaContext *context, void *userData)
{
	BelaArduino_setup(context);
	return true;
}

void render(BelaContext *context, void *userData) {
	BelaArduino_renderTop(context);
	BelaArduino_renderBottom(context);
}

void cleanup(BelaContext *context, void *userData)
{
	BelaArduino_cleanup(context);
}
