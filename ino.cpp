#include <Arduino.h>
#include <PdArduino.h>

void setup()
{
	Serial.println("SETUP");
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, INPUT);
	pdSendMessage("loadSound", 0, "kick.wav");
	pdSendMessage("loadSound", 1, "snare.wav");
}

void loop()
{
	pdSendMessage("playSound", 0);
	digitalWrite(0, HIGH);
	digitalWrite(1, digitalRead(2));
	delay(100);
	pdSendMessage("stopSound", 0);
	delay(400);
	pdSendMessage("playSound", 1);
	digitalWrite(0, LOW);
	digitalWrite(1, digitalRead(2));
	delay(100);
	pdSendMessage("stopSound", 1);
	delay(400);
}
