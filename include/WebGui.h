#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

#include "HardwareConfig.h"
#include "TunerController.h"

class WebGui
{
public:
    WebGui(TunerController& controller);

    void begin();
    void process();

    void broadcastStatus();

private:
    TunerController& tuner;

    ESP8266WebServer server {HTTP_PORT};
    WebSocketsServer websocket {WEBSOCKET_PORT};

    uint32_t lastStatusBroadcast = 0;

    bool lastLed1 = false;
    bool lastLed2 = false;
    bool lastAnt2 = false;

    void handleFileRequest(const String& path);

    void handleRoot();
    void handleStatus();

    void handleCup();
    void handleCdown();

    void handleLup();
    void handleLdown();

    void handleAnt();
    void handleTune();

	void handleWifiPortal();
	void handleReboot();
	void handleFwUpdate();
	void handleWebUpdate();
	void handleFwUpdatePage();
	void handleWebUpdatePage();

    void handleNotFound();

    void onWebSocketEvent(
        uint8_t clientNum,
        WStype_t type,
        uint8_t* payload,
        size_t length
    );

    String createStatusJson();
    void checkStatusChanges();

    static WebGui* instance;

    static void webSocketEventBridge(
        uint8_t clientNum,
        WStype_t type,
        uint8_t* payload,
        size_t length
    );
};
