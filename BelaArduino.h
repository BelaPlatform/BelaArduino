#include <libraries/Pipe/Pipe.h>
#include <vector>
#include <stdint.h>
#include <Bela.h>
#include <libraries/libpd/libpd.h>

enum DigitalMode {
	kDigitalModeIgnored,
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

int BelaArduino_floatHook(const char* source, float value);
int BelaArduino_listHook(const char* source, int argc, t_atom *argv);
int BelaArduino_messageHook(const char* source, const char *symbol, int argc, t_atom *argv);

bool BelaArduino_setup(BelaContext* context, void*);
void BelaArduino_renderTop(BelaContext* context, void*);
void BelaArduino_renderBottom(BelaContext* context, void*);
void BelaArduino_cleanup(BelaContext* context, void*);
