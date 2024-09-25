#include <Arduino.h>
#include <PdArduino.h>

// This function is called when a message is sent from a Pd
// patch. You can use it to communicate from Pd back to your
// Arduino-style code.
void pdReceiveMsg(const char* symbol, float* data, size_t length)
{
	Serial.print("Received ");
	Serial.print(symbol);
	Serial.print("[");
	Serial.print(length);
	Serial.print("]:");
	for(size_t n = 0; n < length; ++n)
	{
		Serial.print(" ");
		Serial.print(data[n]);
	}
	Serial.println();
}

void setup()
{
	Serial.println("Arduino setup\n");
	// Set up the digital pins
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, INPUT);
	pinMode(3, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);

	// Load sound
	pdSendMessage("loadSound", 0, "kick.wav");
	pdSendMessage("loadSound", 1, "snare.wav");
}

void loop()
{
	// Example of using a shift register:
	static unsigned int count = 0;
	shiftOut(4, 5, 6, LSBFIRST, 16, count);

	// To use a custom GUI, you need to #define ENABLE_GUI in BelaArduino.cpp
	float myArray[2];
	myArray[1] = count & 1;
	myArray[0] = count++;
	gui.sendBuffer(0, myArray);

	// Example of playing sounds (see _main.pd) and blinking LEDs
	pdSendMessage("playSound", 0);
	digitalWrite(0, HIGH);
	digitalWrite(1, digitalRead(2));
	pwmWrite(3, 0.05);
	delay(100);
	pdSendMessage("stopSound", 0);
	delay(400);

	pdSendMessage("playSound", 1);
	digitalWrite(0, LOW);
	digitalWrite(1, digitalRead(2));
	pwmWrite(3, 0.3);
	delay(100);
	pdSendMessage("stopSound", 1);
	delay(400);
}
