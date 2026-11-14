#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <time.h>

#include "TunerController.h"
#include "WebGui.h"
#include "TimeUtils.h"

TunerController controller;
WebGui webGui(controller);

void setup()
{
    Serial.begin(115200);

    // WiFi připojení

    // NTP

    controller.begin();

    webGui.begin();
}

void loop()
{
    controller.process();
    webGui.process();
}
