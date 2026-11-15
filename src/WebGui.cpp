#include "WebGui.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <Updater.h>

#include "TimeUtils.h"
#include "LedController.h"

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif

WebGui* WebGui::instance = nullptr;

extern LedController led;

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
	server.on("/api/wifiportal", HTTP_POST, [this]() { handleWifiPortal(); });
	server.on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
	server.on("/api/fwupdate", HTTP_POST, [this]() {handleFwUpdate();} );
	server.on("/api/webupdate", HTTP_POST, [this]() {handleWebUpdate(); });
	server.on("/fwupdate", HTTP_GET, [this]() { handleFwUpdatePage(); });
	server.on("/update", HTTP_POST,[this]()
    {
        server.send(
			200,
			"text/html",
			R"(
		<!DOCTYPE html>
		<html>
		<head>
		<meta charset="utf-8">
		<meta name="viewport"
			content="width=device-width,initial-scale=1">

		<style>

		body {

			background:#121212;

			color:white;

			font-family:Arial;

			display:flex;

			justify-content:center;

			align-items:center;

			height:100vh;

			margin:0;
		}

		.card {

			text-align:center;
		}

		</style>

		</head>

		<body>

		<div class="card">

			<h2>
				Firmware uploaded
			</h2>

			<p>
				Device is rebooting...
			</p>

			<p>
				Waiting for reconnect
			</p>

		</div>

		<script>

		const waitForEsp =
			setInterval(async () =>
		{
			try
			{
				const response =
					await fetch(
						"/api/status"
					);

				if (response.ok)
				{
					clearInterval(
						waitForEsp
					);

					location.href =
						"/";
				}
			}
			catch(e)
			{
			}

		}, 2000);

		</script>

		</body>
		</html>
		)"
		);

        delay(1000);

        ESP.restart();
    },
    [this]()
    {
        HTTPUpload& upload = server.upload();

        if (upload.status == UPLOAD_FILE_START)
        {
            String filename = upload.filename;

            Serial.print( "Upload start: " );

            Serial.println(filename);

            // kontrola přípony

            if (!filename.endsWith(".bin"))
            {
                // Update.abort();
                return;
            }

            if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
            {
                Update.printError(Serial);
            }
        }
		else if (upload.status == UPLOAD_FILE_WRITE)
        {
            // první blok
            // kontrola ESP8266 firmware

            if (upload.totalSize == 0 &&  upload.currentSize > 0)
            {
                if (upload.buf[0] != 0xE9)
                {
                    Serial.println( "Not ESP firmware" );

                    // Update.abort();

                    return;
                }
            }

            if ( Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            {
                Update.printError(Serial);
            }
        }
		else if ( upload.status == UPLOAD_FILE_END)
        {
            if (Update.end(true))
            {
                Serial.println("Update success");
            }
            else
            {
                Update.printError(Serial);
            }
        }
    });


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
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;  
    doc["time"] = getFormattedTime();
	doc["version"] = GIT_VERSION;
	doc["build"] = __DATE__ " " __TIME__;

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
		led.blink(50, 50, 50);
        server.send(429, "text/plain", "Too Many Requests");
        return false;
    }
    return true;
}

void WebGui::handleCup()
{
    if (!enqueueOrReply(tuner,server, CommandType::C_UP))
    {
        return;
    }

    led.blink(50);

    server.send(
        200,
        "text/plain",
        "OK"
    );
}

void WebGui::handleCdown()
{
    if (!enqueueOrReply(tuner,server, CommandType::C_DN))
    {
        return;
    }

    led.blink(50);

    server.send(
        200,
        "text/plain",
        "OK"
    );
}

void WebGui::handleLup()
{
    if (!enqueueOrReply(tuner,server, CommandType::L_UP))
    {
        return;
    }

    led.blink(50);

    server.send(
        200,
        "text/plain",
        "OK"
    );
}

void WebGui::handleLdown()
{
    if (!enqueueOrReply(tuner,server, CommandType::L_DN))
    {
        return;
    }

    led.blink(50);

    server.send(
        200,
        "text/plain",
        "OK"
    );
}

void WebGui::handleAnt()
{
    if (!enqueueOrReply(tuner,server, CommandType::ANT_TOGGLE))
    {
        return;
    }

    led.blink(50);

    server.send(
        200,
        "text/plain",
        "OK"
    );
}

void WebGui::handleTune()
{
    if (!enqueueOrReply(tuner,server, CommandType::TUNE))
    {
        return;
    }

    led.blink(50);

    server.send(
        200,
        "text/plain",
        "OK"
    );
}

void WebGui::handleReboot()
{
    server.send(
		200,
		"application/json",
		"{\"status\":\"rebooting\"}"
	);

	delay(500);

	ESP.restart();
}

void WebGui::handleFwUpdatePage()
{
    server.send(
		200,
		"text/html",
		R"(
	<!DOCTYPE html>
	<html lang="cs">

	<head>

	<meta charset="utf-8">

	<meta name="viewport"
		content="width=device-width,initial-scale=1">

	<title>FW Update</title>

	<style>

	body {
		margin: 0;
		background: #121212;
		color: white;
		font-family: Arial, sans-serif;
	}

	.container {
		max-width: 700px;
		margin: auto;
		padding: 30px;
	}

	h2 {
		text-align: center;
	}

	.card {

		background: #303030;

		border-radius: 10px;

		padding: 20px;

		text-align: center;
	}
	
	.card form {

		display: flex;

		flex-direction: column;

		align-items: center;

		gap: 15px;
	}

	input[type=file] {

		max-width: 350px;
	}

	input[type=submit] {

		width: auto;

		min-width: 180px;

		font-size: 18px;

		padding: 15px 25px;

		border: none;

		border-radius: 10px;

		background: #1976d2;

		color: white;

		cursor: pointer;
	}

	</style>

	</head>

	<body>

	<div class="container">

	<h2>FW Update</h2>

	<div class="card">

	<form method="POST"
		action="/update"
		enctype="multipart/form-data">

	<input type="file"
       name="update"
       accept=".bin">

	<input type="submit"
		value="Upload Firmware">

	</form>

	</div>

	</div>

	</body>

	</html>
	)"
	);
}

void WebGui::handleFwUpdate()
{
    server.send(
        200,
        "text/plain",
        "Not implemented"
    );
}

void WebGui::handleWebUpdate()
{
    server.send(
        200,
        "text/plain",
        "Not implemented"
    );
}

void WebGui::handleWifiPortal()
{
    server.send(
        200,
        "text/plain",
        "WIFI"
    );

    delay(250);

}

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

