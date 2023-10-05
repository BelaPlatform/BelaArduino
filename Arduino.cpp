#include "BelaArduino.h"
#include "Arduino.h" // has to come after BelaArduino because of the #define s it contains

uint32_t random(uint32_t max)
{
	return random(0, max);
}

void randomSeed(uint32_t s)
{
	srand(s);
}

uint32_t random(uint32_t min, uint32_t max) {
	uint32_t ran = rand();
	return map(ran, 0, (float)RAND_MAX, min, max);
}

uint32_t millis() {
	return micros() / 1000.f;
}

unsigned long micros() {
	static timespec startp;
	static bool inited;
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	if(!inited) {
		inited = true;
		startp = tp;
		return 0;
	}
	return ((tp.tv_sec - startp.tv_sec) * 1000000 + (tp.tv_nsec - startp.tv_nsec) / 1000.0);
}

void pinMode(uint32_t channel, uint32_t mode)
{
	if(channel < digital.size())
	{
		digital[channel].mode = INPUT == mode ? kDigitalModeInput : kDigitalModeOutput;
	}
}

void digitalWrite(uint32_t channel, bool value)
{
	if(channel < digital.size())
	{
		digital[channel].mode = kDigitalModeOutput;
		digital[channel].value = value;
	}
}

void pwmWrite(uint32_t channel, float value)
{
	if(channel < digital.size())
	{
		digital[channel].mode = kDigitalModePwm;
		digital[channel].value = value * kPwmPeriod;
	}
}

bool digitalRead(uint32_t channel)
{
	if(channel < digital.size())
		return digital[channel].value;
	return 0;
}

float analogRead(uint32_t channel)
{
	if(channel < analogIn.size())
		return analogIn[channel];
	return 0;
}

void analogWrite(uint32_t channel, float value)
{
	if(channel < analogOut.size())
		analogOut[channel] = value;
}

#include <unistd.h>
void delay(uint32_t t) {
	usleep(t * 1000);
}
void utoa(uint32_t num, char* dest, size_t len) {
	snprintf(dest, len, "%u", num);
}
void noInterrupts() {}
void interrupts() {}
