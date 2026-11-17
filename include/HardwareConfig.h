// =====================================================
// HardwareConfig.h
// =====================================================

#pragma once

#include <Arduino.h>

// -----------------------------------------------------
// GPIO mapping
// -----------------------------------------------------

constexpr uint8_t PIN_C_UP = D1;      // GPIO5
constexpr uint8_t PIN_C_DN = D2;      // GPIO4

constexpr uint8_t PIN_L_UP = D5;      // GPIO14
constexpr uint8_t PIN_L_DN = D6;      // GPIO12

constexpr uint8_t PIN_ANT  = D8;      // GPIO15

constexpr uint8_t PIN_TUNE = D3;      // GPIO0

constexpr uint8_t PIN_LED1 = D0;      // GPIO16
constexpr uint8_t PIN_LED2 = D7;      // GPIO13

constexpr uint32_t BUTTON_PRESS_MS = 100;
constexpr uint32_t LONG_PRESS_MS = 500;
constexpr uint8_t BUTTON_IDLE_LEVEL   = LOW;
constexpr uint8_t BUTTON_ACTIVE_LEVEL = HIGH;
constexpr uint8_t ANT1_SELECT_LEVEL = HIGH;
constexpr uint8_t COMMAND_QUEUE_SIZE = 30;
constexpr char CONFIG_FILE[] = "/config.json";
constexpr uint16_t CONFIG_JSON_SIZE = 128;
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.google.com";
constexpr long GMT_OFFSET_SEC = 3600;
constexpr long DST_OFFSET_SEC = 3600;
constexpr uint16_t HTTP_PORT = 80;
constexpr uint16_t WEBSOCKET_PORT = 81;
constexpr uint32_t STATUS_BROADCAST_MS = 2000;
