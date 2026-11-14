#include <Arduino.h>
#include "LedController.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>

const int LED_PIN = LED_BUILTIN;

LedController led;

String wifiSsid;
String wifiPassword;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0; // 1 * 60 * 60;
const int   daylightOffset_sec = 0; // 3600;

bool wifiConnected = false;
bool timeObtained = false;

bool loadCredentials()
{
    File f = LittleFS.open("/config.txt", "r");
    if (!f)
	{
        return false;
	}

    while (f.available())
    {
        String line = f.readStringUntil('\n');
        line.trim();

        if (line.startsWith("SSID="))
            wifiSsid = line.substring(5);

        if (line.startsWith("PASSWORD="))
            wifiPassword = line.substring(9);
    }

    f.close();
    return true;
}

bool saveCredentials(const String& ssid, const String& password)
{
    File f = LittleFS.open("/config.txt", "w");
    if (!f)
        return false;

    f.println("SSID=" + ssid);
    f.println("PASSWORD=" + password);

    f.close();

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

    delay(1000);
    ESP.restart();
}

bool printLocalTime()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return false;
    }

    char buffer[64];

    strftime(
        buffer,
        sizeof(buffer),
        "%A, %d.%m.%Y %H:%M:%S",
        &timeinfo
    );

    Serial.println(buffer);

    return true;
}

bool timeInit(int maxTryNr = 5)
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);
    tzset();
    for (int tryNr = 0; tryNr < maxTryNr; tryNr++) {
        yield();
        if (printLocalTime())
            return true;
    }
    Serial.println("The maximum number of attempts to get time has been reached");
    return false;
}

void checkWifiStateAndReconnectItIfDown(unsigned long currentMillis, uint32_t interval = 10000)
{
    static volatile unsigned long wifiReconnectionStartTime = 0;
    static volatile unsigned long obtainLocalTimeStartTime = 0;
    // if WiFi is down, try reconnecting
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - wifiReconnectionStartTime >= interval))
    {
        wifiConnected = false;
        led.repeatBlink(100, 100);
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        wifiReconnectionStartTime = currentMillis;
    }
    // WiFi connected, check time and if is correct calculate new feeding times
    if (WiFi.status() == WL_CONNECTED && wifiConnected == false)
    {
        wifiConnected = true;
        // Init and get the time
        Serial.println("Get new time info...");
        timeObtained = timeInit();
        // start camera server
        // TODO startWeberver();
        if (timeObtained == true)
        {
            led.repeatBlink(50, 1500);
        }
    }
    // WiFi connected, but time failed to obtain from NTP
    if (WiFi.status() == WL_CONNECTED && timeObtained == false && (currentMillis - obtainLocalTimeStartTime >= interval))
    {
        timeObtained = printLocalTime();
        if (timeObtained == true)
        {
            led.repeatBlink(50, 1500);
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
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS mount failed");
        return;
    }

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
        Serial.println("SSID not configured");
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
        Serial.println();
        Serial.println("WiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        return true;
    }

    WiFi.disconnect();

    Serial.println();
    Serial.println("WiFi connection timeout");

    return false;
}

void setup()
{
	led.init(LED_PIN, true);

    // Indicate that MCU is initializing
    led.on();

	// initialize serial port for debug
    serialInit();

	// initialize WiFi connection
	wifiCreditialsInit();
	wifiConnected = wifiInit(millis(), 15000);

    if (wifiConnected) {
        // Init and get the time
        timeObtained = timeInit();
        // start camera server
        //startWebServer();
    }


	// WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

	led.repeatBlink(50, 2000);
}

void loop()
{
	unsigned long time = millis();

	led.process(time);
	// if WiFi is down, try reconnecting
	if (wifiSsid.length() > 0)
    {
    	checkWifiStateAndReconnectItIfDown(time);
	}

	if (Serial.available() == true)
        checkSerialCommand(Serial.readString());
}