#include <Arduino.h>
#include "LedController.h"

const int LED_PIN = LED_BUILTIN;

LedController led;

void setup()
{
	led.init(LED_PIN, true);

    // Indicate that MCU is initializing
    led.on();

    Serial.begin(115200);

    Serial.println();
    Serial.println("Start programu");

	led.repeatBlink(50, 2000);
}

void loop()
{
	unsigned long time = millis();
    static int cnt = 0;

	led.process(time);

    cnt++;

    Serial.printf(
        "Pocet bliknuti: %d\n",
        cnt
    );
}