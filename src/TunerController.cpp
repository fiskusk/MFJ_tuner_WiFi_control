#include "TunerController.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

void TunerController::begin()
{
    pinMode(PIN_C_UP, OUTPUT);
    pinMode(PIN_C_DN, OUTPUT);

    pinMode(PIN_L_UP, OUTPUT);
    pinMode(PIN_L_DN, OUTPUT);

    pinMode(PIN_ANT, OUTPUT);

    pinMode(PIN_TUNE, OUTPUT);

    pinMode(PIN_LED1, INPUT);
    pinMode(PIN_LED2, INPUT);

    digitalWrite(PIN_C_UP, BUTTON_IDLE_LEVEL);
    digitalWrite(PIN_C_DN, BUTTON_IDLE_LEVEL);

    digitalWrite(PIN_L_UP, BUTTON_IDLE_LEVEL);
    digitalWrite(PIN_L_DN, BUTTON_IDLE_LEVEL);

    digitalWrite(PIN_TUNE, BUTTON_IDLE_LEVEL);

    if (!LittleFS.begin())
    {
        Serial.println("LittleFS mount failed");
    }

    loadConfig();

    digitalWrite(PIN_ANT, ant2State ? !ANT1_SELECT_LEVEL : ANT1_SELECT_LEVEL);
}

void TunerController::process()
{
    processPulse();

    if (pulseActive)
    {
        return;
    }

    CommandType cmd;

    if (dequeue(cmd))
    {
        executeCommand(cmd);
    }
}

bool TunerController::enqueue(CommandType cmd)
{
    return enqueueInternal(cmd);
}

bool TunerController::enqueueInternal(CommandType cmd)
{
    if (queueCount >= COMMAND_QUEUE_SIZE)
    {
        Serial.println("Command queue full");
        return false;
    }

    queue[queueTail] = cmd;

    queueTail++;

    if (queueTail >= COMMAND_QUEUE_SIZE)
    {
        queueTail = 0;
    }

    queueCount++;

    return true;
}

bool TunerController::dequeue(CommandType &cmd)
{
    if (queueCount == 0)
    {
        return false;
    }

    cmd = queue[queueHead];

    queueHead++;

    if (queueHead >= COMMAND_QUEUE_SIZE)
    {
        queueHead = 0;
    }

    queueCount--;

    return true;
}

void TunerController::startPulse(uint8_t pin)
{
    activePin = pin;

    pulseStartTime = millis();

    pulseActive = true;

    digitalWrite(activePin, BUTTON_ACTIVE_LEVEL);
}

void TunerController::processPulse()
{
    if (!pulseActive)
    {
        return;
    }

    if ((millis() - pulseStartTime) < BUTTON_PRESS_MS)
    {
        return;
    }

    digitalWrite(activePin, BUTTON_IDLE_LEVEL);

    pulseActive = false;
}

void TunerController::executeCommand(CommandType cmd)
{
    switch (cmd)
    {
    case CommandType::C_UP:
        startPulse(PIN_C_UP);
        break;

    case CommandType::C_DN:
        startPulse(PIN_C_DN);
        break;

    case CommandType::L_UP:
        startPulse(PIN_L_UP);
        break;

    case CommandType::L_DN:
        startPulse(PIN_L_DN);
        break;

    case CommandType::TUNE:
        startPulse(PIN_TUNE);
        break;

    case CommandType::ANT_TOGGLE:
        ant2State = !ant2State;

        digitalWrite(PIN_ANT, ant2State ? !ANT1_SELECT_LEVEL : ANT1_SELECT_LEVEL);

        saveConfig();
        break;
    }
}

bool TunerController::antenna2() const
{
    return ant2State;
}

bool TunerController::led1() const
{
    return digitalRead(PIN_LED1);
}

bool TunerController::led2() const
{
    return digitalRead(PIN_LED2);
}

void TunerController::loadConfig()
{
    if (!LittleFS.exists(CONFIG_FILE))
    {
        ant2State = false;
        return;
    }

    File file = LittleFS.open(CONFIG_FILE, "r");

    if (!file)
    {
        ant2State = false;
        return;
    }

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, file);

    file.close();

    if (error)
    {
        ant2State = false;
        return;
    }

    ant2State = doc["ant"] == 2;
}

void TunerController::saveConfig()
{
    JsonDocument doc;

    doc["ant"] = ant2State ? 2 : 1;

    File file = LittleFS.open(CONFIG_FILE, "w");

    if (!file)
    {
        Serial.println("Config write failed");
        return;
    }

    serializeJson(doc, file);

    file.close();
}

uint8_t TunerController::queuedCommands() const
{
    return queueCount;
}
