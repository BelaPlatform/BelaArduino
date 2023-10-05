#include <Arduino.h>
#include <PdArduino.h>

void setup()
{
	Serial.println("SETUP");
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, INPUT);
}

void loop()
{
	pdSendMessage("playSound", "one", "two", 123, 123.f);
	pdSendMessage("stopSound", 1, "uops", 3);
	Serial.println(analogRead(0));
	digitalWrite(0, HIGH);
	digitalWrite(1, digitalRead(2));
	delay(500);
	digitalWrite(0, LOW);
	digitalWrite(1, digitalRead(2));
	delay(500);
}
