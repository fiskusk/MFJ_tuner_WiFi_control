// =====================================================
// TunerController.h
// =====================================================

#pragma once

#include "HardwareConfig.h"
#include <Arduino.h>

enum class CommandType
{
    C_UP,
    C_DN,
    L_UP,
    L_DN,
    ANT_TOGGLE,
    TUNE
};

class TunerController
{
  public:
    void begin();
    void process();

    bool enqueue(CommandType cmd);

    bool antenna2() const;

    bool led1() const;
    bool led2() const;

    uint8_t queuedCommands() const;

    void startContinuous(CommandType cmd);

    void stopContinuous();

  private:
    CommandType queue[COMMAND_QUEUE_SIZE];

    uint8_t queueHead = 0;
    uint8_t queueTail = 0;
    uint8_t queueCount = 0;

    bool enqueueInternal(CommandType cmd);
    bool dequeue(CommandType &cmd);

    bool pulseActive = false;
    uint8_t activePin = 0;
    uint32_t pulseStartTime = 0;

    bool continuousActive = false;

    uint8_t continuousPin = 0;

    void startPulse(uint8_t pin);
    void processPulse();

    bool ant2State = false;

    void loadConfig();
    void saveConfig();

    void executeCommand(CommandType cmd);
};
