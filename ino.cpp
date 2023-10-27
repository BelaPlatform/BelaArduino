#include <Arduino.h>
#include <PdArduino.h>

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
	Serial.println("SETUP");
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, INPUT);
	pinMode(3, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);
	pdSendMessage("loadSound", 0, "kick.wav");
	pdSendMessage("loadSound", 1, "snare.wav");
}

void loop()
{
	static unsigned int count = 0;
	shiftOut(4, 5, 6, LSBFIRST, 16, count);
	float myArray[2];
	myArray[1] = count & 1;
	myArray[0] = count++;
	gui.sendBuffer(0, myArray);
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
