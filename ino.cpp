#include <Arduino.h>

void setup()
{
	Serial.println("SETUP");
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, INPUT);
}

void loop()
{
	Serial.print("LOOP ");
	Serial.println(analogRead(0));
	digitalWrite(0, HIGH);
	digitalWrite(1, digitalRead(2));
	delay(500);
	digitalWrite(0, LOW);
	digitalWrite(1, digitalRead(2));
	delay(500);
}
