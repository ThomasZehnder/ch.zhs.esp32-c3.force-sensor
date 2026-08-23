
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "Global.h"
#include <WiFiService.h>
#include "credentials.h"
#include "Force.h"
#include "display.h"

// global object definition
clAssembly Assembly;

// Global JSON document for configuration
JsonDocument doc;

void SerialFileOut(const char *filename)
{

    File file = LittleFS.open(filename, "r"); // Open the file
    // print out filecontent
    Serial.println(String("Assembly.setup --> configfile: ") + filename + " found... size:" + file.size());

    while (file.available())
    {
        Serial.write(file.read());
    }
    Serial.println();
    file.close(); // Close the file again
}

#define DEVICEID "FORCE-SENSOR-001"
void clAssembly::setupDevice()
{
    char filename[] = "/config_main.json";

    if (LittleFS.exists(filename))
    {
        // SerialFileOut(filename);

        File file = LittleFS.open(filename, "r"); // Open the file again

        // parse JSON
        DeserializationError error = deserializeJson(doc, file);

        // Test if parsing succeeds.
        if (error)
        {
            Serial.print(F("Assembly.setupDevice --> deserializeJson() failed: "));
            Serial.println(error.f_str());
        }

        // Serial.println(String("Assembly.setup --> configfile number of entries: ") + doc.size());

        strncpy(deviceId, doc["DEVICEID"] | DEVICEID, sizeof(deviceId));
        cfg.scale = doc["SCALE"] | 1.0f;
        cfg.offset = doc["OFFSET"] | 0L;
        cfg.taraCalibrateNewton = doc["TARA_CALIBRATE_NEWTON"] | 40.0f;

        Serial.println(String("Assembly.setupDevice --> deviceid: ") + deviceId);
        Serial.println(String("Assembly.setupDevice --> scale: ") + cfg.scale + " offset: " + cfg.offset);
        Serial.println(String("Assembly.setupDevice --> taraCalibrateNewton: ") + cfg.taraCalibrateNewton);
        file.close(); // Close the file again
    }
    else
    {
        Serial.println(String("Assembly.setupDevice --> error: NO ") + filename + " found, works with default defines.");
        strcpy(deviceId, DEVICEID);
        cfg.scale = 1.0f;
        cfg.offset = 0L;
        cfg.taraCalibrateNewton = 9.81f;
    }
}

// persist deviceId/cfg (incl. scale/offset from the latest tare/calibrate) to config_main.json
void clAssembly::saveConfig()
{
    char filename[] = "/config_main.json";

    doc.clear();
    doc["DEVICEID"] = deviceId;
    doc["SCALE"] = cfg.scale;
    doc["OFFSET"] = cfg.offset;
    doc["TARA_CALIBRATE_NEWTON"] = cfg.taraCalibrateNewton;

    File file = LittleFS.open(filename, "w");
    if (file)
    {
        serializeJson(doc, file);
        file.close();
        Serial.println("Assembly.saveConfig --> config_main.json updated");
    }
    else
    {
        Serial.println("Assembly.saveConfig --> error: could not open config_main.json for writing");
    }
}

void clAssembly::setupWifi()
{
    char filename[] = "/config_wlan.json";

    if (LittleFS.exists(filename))
    {
        // SerialFileOut(filename);

        File file = LittleFS.open(filename, "r"); // Open the file again

        // parse JSON
        DeserializationError error = deserializeJson(doc, file);

        // Test if parsing succeeds.
        if (error)
        {
            Serial.print(F("Assembly.setupWifi --> deserializeJson() failed: "));
            Serial.println(error.f_str());
        }

        // assigne values
        // get array size

        // Serial.println(String("Assembly.setup --> configfile number of entries: ") + doc.size());
        byte i = 0;
        for (JsonObject item : doc.as<JsonArray>())
        {
            if (i < (sizeof(cfg.wifi) / sizeof(cfg.wifi[0])))
            {
                strncpy(cfg.wifi[i].ssid, item["SSID"] | WIFI_SSID, sizeof(cfg.wifi[i].ssid));
                strncpy(cfg.wifi[i].pw, item["PASSWORD"] | WIFI_PASSWORD, sizeof(cfg.wifi[i].pw));

                Serial.println(String("Assembly.setupWifi --> entry: ") + i + " / " + cfg.wifi[i].ssid + " / " + cfg.wifi[i].pw);
            }

            i++;
        }

        file.close(); // Close the file again
    }
    else
    {
        Serial.println(String("Assembly.setupWifi --> error: NO ") + filename + " found, works with default defines.");
        strcpy(cfg.wifi[0].ssid, WIFI_SSID);
        strcpy(cfg.wifi[0].pw, WIFI_PASSWORD);
        strcpy(cfg.wifi[1].ssid, WIFI_SSID_1);
        strcpy(cfg.wifi[1].pw, WIFI_PASSWORD_1);
        strcpy(cfg.wifi[2].ssid, "");
        strcpy(cfg.wifi[2].pw, "");
    }
}


// read configuration from file
void clAssembly::setup()
{
    Serial.println("Assembly.setup --> begin");

    // compile date
    compileDate = String(CPP_VERSION) + __TIMESTAMP__;
    Serial.print("Assembly.setup --> compile date: ");
    Serial.println(compileDate);

    if (!LittleFS.begin())
    {
        Serial.println("Assembly.setup --> An Error has occurred while mounting LittleFS");
        delay(1000);
    }

    setupDevice();
    setupWifi();

    Serial.println("Assembly.setup --> end");
}

void clAssembly::rebootProcess()
{
    // show "rebooting" on the OLED for 1s, actual reboot fires in main loop after it elapses
    state = StateReboot;
    stateStartMillis = millis();
}

void clAssembly::wlanConnectedProcess()
{
    localIp = WiFi.localIP().toString();
    ssid = WiFi.SSID();

    // select cfg index depending on found Wifi
    for (byte i = 0; i < (sizeof(cfg.wifi) / sizeof(cfg.wifi[0])); i++)
    {
        if (WiFi.SSID() == cfg.wifi[i].ssid)
        {
            Serial.print("Assembly.wlanConnectedProcess --> use cfg index: ");
            Serial.println(i);
            cfg.index = i;
            break;
        }
    }
}

void clAssembly::processKeys()
{

        if (keys[0].edge)
        {
            // Key 0 --> show "tare now" for 1s, actual tare fires in main loop after it elapses
            state = StateTare;
            displayRenderQueued("TARA 0", "Apply no force", "", "", 1000);
            stateStartMillis = millis();
        }
        else if (keys[1].edge)
        {
            // Key 1 --> show reference force for 1s, actual calibration fires in main loop after it elapses
            state = StateCalibrate;
            displayRenderQueued("SCALE", "ApplyForce:", String(cfg.taraCalibrateNewton).c_str(), "", 1000);
            stateStartMillis = millis();
        }

        // 1s elapsed since the state was entered --> wait for key release before firing,
        // so the action doesn't run while the user is still pressing (e.g. shaking the sensor)
        bool actionDelayElapsed = (millis() - stateStartMillis) >= 1000;
        if (actionDelayElapsed)
        {
            if (state == StateTare && keys[0].pressed)
            {
                Force.tare();
                state = StateMeasure;
            }
            else if (state == StateCalibrate && keys[1].pressed)
            {
                Force.calibrate(cfg.taraCalibrateNewton); // calibrate against the configured reference force
                state = StateMeasure;
            }
            else if (state == StateReboot)
            {
                Serial.println("main.loop --> rebooting now");
                ESP.restart();
            }
        }
}
