#include "WebGui.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include "TimeUtils.h"

WebGui* WebGui::instance = nullptr;

WebGui::WebGui(TunerController& controller)
    : tuner(controller)
{
}

void WebGui::begin()
{
    instance = this;

    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/index.html", HTTP_GET, [this]() { handleFileRequest("/index.html"); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/cup", HTTP_POST, [this]() { handleCup(); });
    server.on("/api/cdown", HTTP_POST, [this]() { handleCdown(); });
    server.on("/api/lup", HTTP_POST, [this]() { handleLup(); });
    server.on("/api/ldown", HTTP_POST, [this]() { handleLdown(); });
    server.on("/api/ant", HTTP_POST, [this]() { handleAnt(); });
    server.on("/api/tune", HTTP_POST, [this]() { handleTune(); });
    server.on("/style.css", HTTP_GET, [this]() { handleFileRequest("/style.css"); });
    server.on("/app.js", HTTP_GET, [this]() { handleFileRequest("/app.js"); });
    server.onNotFound([this]() { handleNotFound(); });

    server.begin();

    websocket.begin();
    websocket.onEvent(webSocketEventBridge);

    lastLed1 = tuner.led1();
    lastLed2 = tuner.led2();
    lastAnt2 = tuner.antenna2();
}

void WebGui::process()
{
    server.handleClient();
    websocket.loop();

    checkStatusChanges();

    if ((millis() - lastStatusBroadcast) > STATUS_BROADCAST_MS)
    {
        lastStatusBroadcast = millis();
        broadcastStatus();
    }
}

void WebGui::broadcastStatus()
{
	String json = createStatusJson();

    websocket.broadcastTXT(json);
}

String WebGui::createStatusJson()
{
    JsonDocument doc;

    doc["type"] = "status";
    doc["ant"] = tuner.antenna2() ? 2 : 1;
    doc["led1"] = tuner.led1();
    doc["led2"] = tuner.led2();
    doc["queue"] = tuner.queuedCommands();
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["time"] = getFormattedTime();

    String json;
    serializeJson(doc, json);
    return json;
}

void WebGui::checkStatusChanges()
{
    bool led1 = tuner.led1();
    bool led2 = tuner.led2();
    bool ant2 = tuner.antenna2();

    if (led1 != lastLed1 || led2 != lastLed2 || ant2 != lastAnt2)
    {
        lastLed1 = led1;
        lastLed2 = led2;
        lastAnt2 = ant2;
        broadcastStatus();
    }
}

void WebGui::handleRoot(){ handleFileRequest("/index.html"); }

void WebGui::handleStatus()
{
    server.send(200, "application/json", createStatusJson());
}

static bool enqueueOrReply(TunerController& tuner, ESP8266WebServer& server, CommandType cmd)
{
    if (!tuner.enqueue(cmd))
    {
        server.send(429, "text/plain", "Too Many Requests");
        return false;
    }
    return true;
}

void WebGui::handleCup(){ if(enqueueOrReply(tuner,server,CommandType::C_UP)) server.send(200,"text/plain","OK"); }
void WebGui::handleCdown(){ if(enqueueOrReply(tuner,server,CommandType::C_DN)) server.send(200,"text/plain","OK"); }
void WebGui::handleLup(){ if(enqueueOrReply(tuner,server,CommandType::L_UP)) server.send(200,"text/plain","OK"); }
void WebGui::handleLdown(){ if(enqueueOrReply(tuner,server,CommandType::L_DN)) server.send(200,"text/plain","OK"); }
void WebGui::handleAnt(){ if(enqueueOrReply(tuner,server,CommandType::ANT_TOGGLE)) server.send(200,"text/plain","OK"); }
void WebGui::handleTune(){ if(enqueueOrReply(tuner,server,CommandType::TUNE)) server.send(200,"text/plain","OK"); }

void WebGui::handleFileRequest(const String& path)
{
    if (!LittleFS.exists(path))
    {
        server.send(404, "text/plain", "File not found");
        return;
    }

    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";

    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
}

void WebGui::handleNotFound(){ server.send(404, "text/plain", "404 Not Found"); }

void WebGui::onWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length)
{
    switch (type)
    {
        case WStype_CONNECTED:
		{
            String json = createStatusJson();

			websocket.sendTXT(clientNum, json);
            break;
		}

        case WStype_TEXT:
        {
            String msg((char*)payload);
            if (msg == "status")
			{
				String json = createStatusJson();

				websocket.sendTXT(clientNum, json);
			}
            break;
        }

        default:
            break;
    }
}

void WebGui::webSocketEventBridge(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length)
{
    if (instance != nullptr)
        instance->onWebSocketEvent(clientNum, type, payload, length);
}
