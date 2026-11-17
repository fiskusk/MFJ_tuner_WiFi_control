// TODO udělat možnost režimu AP, kdy to poběží standalone a něco se může připojit k tomu bez nutnosti externí wifi sítě.

#include <Arduino.h>
#include "LedController.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiManager.h>

#include "TunerController.h"
#include "WebGui.h"
#include "TimeUtils.h"

const int LED_PIN = LED_BUILTIN;

LedController led;

TunerController controller;
WebGui webGui(controller);

WiFiManager wm;

String wifiSsid;
String wifiPassword;

const long  gmtOffset_sec = 0; // 1 * 60 * 60;
const int   daylightOffset_sec = 0; // 3600;

bool wifiConnected = false;
bool timeObtained = false;

bool wifiPortalRunning = false;

struct WifiCredentials
{
    uint32_t magic;
    
    char ssid[32];
    char password[64];
};

constexpr uint32_t WIFI_MAGIC = 0x4D464A31;

static_assert(sizeof(WifiCredentials) < 128, "WifiCredentials too large");

bool loadCredentials()
{
    WifiCredentials cfg;

    EEPROM.get(
        0,
        cfg
    );

    if (cfg.magic != WIFI_MAGIC)
    {
        return false;
    }

    wifiSsid =
        String(cfg.ssid);

    wifiPassword =
        String(cfg.password);

    return true;
}

bool saveCredentials(const String& ssid, const String& password)
{
    WifiCredentials cfg {};
 
   cfg.magic = WIFI_MAGIC;     

    Serial.print("Saving SSID: ");
    Serial.println(ssid);    

    strncpy(
        cfg.ssid,
        ssid.c_str(),
        sizeof(cfg.ssid) - 1
    );

    strncpy(
        cfg.password,
        password.c_str(),
        sizeof(cfg.password) - 1
    );

    EEPROM.put(
        0,
        cfg
    );

    EEPROM.commit();

    wifiSsid = ssid;
    wifiPassword = password;

    return true;
}


String removeSpecialCharacters(String s)
{
    for (int i = 0; i < s.length(); i++) {
        if (s[i] < 32 || s[i] > 126) {
            // erase function to erase
            // the character
            s.remove(i);
            i--;
        }
    }
    return s;
}

void commandNewCreditials()
{
    String receivedSsid;
    String receivedPassword;

    Serial.println("Enter new WiFi SSID:");

    while (!Serial.available())
    {
        yield();    // na ESP8266 místo esp_task_wdt_reset()
    }

    receivedSsid = removeSpecialCharacters(Serial.readString());

    Serial.print("Enter new WiFi Password for \"");
    Serial.print(receivedSsid);
    Serial.println("\":");

    while (!Serial.available())
    {
        yield();
    }

    receivedPassword = removeSpecialCharacters(Serial.readString());

    if (saveCredentials(receivedSsid, receivedPassword))
    {
        Serial.println("New credentials saved.");
    }
    else
    {
        Serial.println("Save failed!");
    }

    Serial.println("Restarting...");    

    delay(1000);
    ESP.restart();
}

bool timeInit(int maxTryNr = 5)
{
    configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER_1, NTP_SERVER_2);
    setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);
    tzset();
    for (int tryNr = 0; tryNr < maxTryNr; tryNr++) {
        delay(100);
        if (timeValid())
        {
            Serial.println(getFormattedDateTime());
            return true;
        }
    }
    Serial.println("The maximum number of attempts to get time has been reached");
    return false;
}

void startWifiPortal()
{
        Serial.println("Starting WiFi portal...");

		wm.setHostname("MFJ-Tuner-Control");

		wm.setConfigPortalTimeout(300);
		
		led.repeatBlink(50, 50, 50, 1000);

		wm.setConfigPortalBlocking(
			false
		);

		wifiPortalRunning = true;

		wm.startConfigPortal("MFJ-Tuner-Setup");
}


void checkWifiStateAndReconnectItIfDown(unsigned long currentMillis, uint32_t interval = 10000)
{
    static volatile unsigned long wifiReconnectionStartTime = 0;
    static volatile unsigned long obtainLocalTimeStartTime = 0;
    static uint8_t reconnectAttempts = 0;
    
    // if WiFi is down, try reconnecting
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - wifiReconnectionStartTime >= interval))
    {
        wifiConnected = false;
        
        led.repeatBlink(100, 100);
        
        Serial.print("Reconnecting to WiFi... ");
        
        WiFi.disconnect();
        delay(100);

		WiFi.begin(
			wifiSsid.c_str(),
			wifiPassword.c_str()
		);
        
        reconnectAttempts++;

        Serial.print(
            "Reconnect attempt: "
        );
        
        Serial.println(
            reconnectAttempts
        );
               
        wifiReconnectionStartTime = currentMillis;
    }
    
    if (reconnectAttempts >= 10)
    {
        reconnectAttempts = 0;
    
        startWifiPortal();
    }    
    // WiFi connected, check time and if is correct calculate new feeding times
    if (WiFi.status() == WL_CONNECTED && wifiConnected == false)
    {
        reconnectAttempts = 0;    
        wifiConnected = true;
        // Init and get the time
        Serial.println("Get new time info...");
        timeObtained = timeInit();
        Serial.println();
        Serial.println("WiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    
        if (timeObtained == true)
        {
            led.repeatBlink(50, 2000);
        }
    }
    // WiFi connected, but time failed to obtain from NTP
    if (WiFi.status() == WL_CONNECTED && timeObtained == false && (currentMillis - obtainLocalTimeStartTime >= interval))
    {
        Serial.println(getFormattedDateTime());
        if (timeValid())
        {
            timeObtained = true;
        
            Serial.println(getFormattedDateTime());
        
            led.repeatBlink(50, 2000);
        }
        else
        {
            led.repeatBlink(100, 100);
        }
        obtainLocalTimeStartTime = currentMillis;
    }
}

void scanWifi()
{
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    if (n == WIFI_SCAN_FAILED) {
        Serial.println("Scan failed");
        return;
    }
    Serial.println("Scan done");
    if (n == 0) {
        Serial.println("No networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
        delay(10);
        }
    }
}

void unknownCommand(String received)
{
    Serial.print("Received command \"");
    Serial.print(received);
    Serial.println("\" unknown!");
}

void checkSerialCommand(String received)
{
    if (received == "W\n") {
        Serial.println("Press \"N\" for enter new creditials; \"S\" for scan WiFi Networks");
        while (Serial.available() == false) { yield();}
        received = Serial.readString();
        if (received == "N\n")
            commandNewCreditials();
        else if (received == "S\n")
            scanWifi();
        else 
            unknownCommand(received);
    } 
    else if (received == "R\n") {
        ESP.restart();
    }
    else if (received == "\n" ) {
        Serial.println("For configure WiFi connection enter \"W\"; For ESP restart enter \"R\"");
    }
    else {
        unknownCommand(received);
    }
}


void wifiCreditialsInit()
{
    Serial.println("Loading credentials from EEPROM");
    
    if (!loadCredentials())
    {
        Serial.println("No stored credentials");
        return;
    }

    Serial.print("Stored ssid: ");
    Serial.println(wifiSsid);
}


void serialInit()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println();
    Serial.println("========================================");
    Serial.println("========================================");
}

bool wifiInit(unsigned long connectingStartingTime, uint32_t timeoutMs = 10000)
{
    WiFi.hostname("MFJ-Tuner-Control");

    if (wifiSsid.length() == 0)
    {
		Serial.println("No WiFi credentials stored");
		startWifiPortal();
		return false;
    }

    WiFi.begin(
        wifiSsid.c_str(),
        wifiPassword.c_str()
    );

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED &&
           millis() < connectingStartingTime + timeoutMs)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    WiFi.disconnect();

	Serial.println();
	Serial.println("WiFi connection timeout");

	startWifiPortal();

	return false;
}

void setup()
{
    led.init(LED_PIN, true);

    // Indicate that MCU is initializing
    led.on();

    // initialize serial port for debug
    serialInit();
 
     EEPROM.begin(sizeof(WifiCredentials));     

    if (!LittleFS.begin())
    {
        Serial.println(
            "LittleFS mount failed"
        );
    }    

    // initialize WiFi connection
    wifiCreditialsInit();
    wifiConnected = wifiInit(millis(), 15000);

    if (wifiConnected) {
        // Init and get the time
        timeObtained = timeInit();
        Serial.println();
        Serial.println("WiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

		led.repeatBlink(50, 2000);
    }

    controller.begin();
    webGui.begin();
 
}

void loop()
{
    unsigned long time = millis();

    led.process(time);

	if (wifiPortalRunning)
	{
		wm.process();

		if (WiFi.status() == WL_CONNECTED)
		{
			saveCredentials(
				WiFi.SSID(),
				WiFi.psk()
			);

			Serial.println("New creditials from portal saved");
			Serial.println();
			Serial.println("WiFi connected");
			Serial.print("IP: ");
			Serial.println(WiFi.localIP());

			wifiPortalRunning = false;

			wifiConnected = true;

			timeObtained =
				timeInit();

			led.repeatBlink(
				50,
				2000
			);
		}
	}

    // if WiFi is down, try reconnecting
    if (wifiSsid.length() > 0)
    {
        checkWifiStateAndReconnectItIfDown(time);
    }

    if (Serial.available() == true)
        checkSerialCommand(Serial.readString());

    controller.process();
    webGui.process();
}
