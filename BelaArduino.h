#include <libraries/Pipe/Pipe.h>
#include <vector>
#include <stdint.h>
#include <Bela.h>
#include <libraries/libpd/libpd.h>

enum DigitalMode {
	kDigitalModeInput,
	kDigitalModeOutput,
	kDigitalModePwm,
};
struct DigitalChannel {
	uint32_t value;
	uint32_t period;
	uint32_t count;
	DigitalMode mode;
};

extern std::vector<DigitalChannel> digital;
extern Pipe belaArduinoPipe;
extern std::vector<float> analogIn;
extern std::vector<float> analogOut;

void BelaArduino_floatHook(float value);
void BelaArduino_listHook(int argc, t_atom *argv);
void BelaArduino_messageHook(const char *symbol, int argc, t_atom *argv);

bool BelaArduino_setup(BelaContext* context, void*);
void BelaArduino_renderTop(BelaContext* context, void*);
void BelaArduino_renderBottom(BelaContext* context, void*);
void BelaArduino_cleanup(BelaContext* context, void*);
