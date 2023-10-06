#include <libraries/Pipe/Pipe.h>
#include <vector>
#include <stdint.h>
#include <Bela.h>

enum DigitalMode {
	kDigitalModeInput,
	kDigitalModeOutput,
	kDigitalModePwm,
};
struct DigitalChannel {
	DigitalMode mode;
	uint16_t value;
};
// assumed to be power of 2
static constexpr size_t kPwmPeriod = 256;

extern std::vector<DigitalChannel> digital;
extern Pipe belaArduinoPipe;
extern std::vector<float> analogIn;
extern std::vector<float> analogOut;

bool BelaArduino_setup(BelaContext* context);
void BelaArduino_renderTop(BelaContext* context);
void BelaArduino_renderBottom(BelaContext* context);
void BelaArduino_cleanup(BelaContext* context);
