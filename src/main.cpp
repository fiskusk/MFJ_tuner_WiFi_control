#include <Arduino.h>

const int LED_PIN = LED_BUILTIN;

void setup()
{
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(115200);

    Serial.println();
    Serial.println("Start programu");
}

void loop()
{
    static int cnt = 0;

    digitalWrite(LED_PIN, LOW);
    delay(500);

    digitalWrite(LED_PIN, HIGH);
    delay(500);

    cnt++;

    Serial.printf(
        "Pocet bliknuti: %d\n",
        cnt
    );
}