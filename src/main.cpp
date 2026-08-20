#include <Arduino.h>

#include "app_webserver.h"

#include "Force.h"
#include "HwInterface.h"

#include <WiFiService.h>

#include "Global.h"
#include "display.h"

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("[DEBUG] === SETUP START ===");

    Serial.println("[DEBUG] Assembly.setup()...");
    Assembly.setup(); // read config file

    Serial.println("[DEBUG] initDisplay()...");
    initDisplay();
    renderDisplay("ESP32-C3 OLED", "DeviceId", Assembly.deviceId, "...");

    Serial.println("[DEBUG] wifiSetup()...");
    wifiSetup();

    Serial.println("[DEBUG] setupWebServer()...");
    setupWebServer();

    Serial.println("[DEBUG] hwSetup()...");
    hwSetup();

    Serial.println("[DEBUG] Force.setup()...");
    Force.setup();

    Serial.println("[DEBUG] === SETUP END ===");
}

void loop()
{
    // Update display queue
    displayUpdate();

    static unsigned long lastDebugMillis = 0;
    if ((long)(millis() - lastDebugMillis) >= 1000)
    {
        lastDebugMillis = millis();
        Serial.print("[DEBUG] loop tick - state: ");
        Serial.print(Assembly.state);
        Serial.print(" wifi: ");
        Serial.print(Assembly.wifiConnected);
        Serial.print(" millis: ");
        Serial.println(millis());
    }

    hwLoop();

    if (hwSecoundTick())
    {
        Serial.println("[DEBUG] Second tick");
        if (Assembly.state == StateSetup)
        {
            Serial.println("[STATE CHANGE ] StateSetup --> StateMeasure");
            Assembly.state = StateMeasure;
        }
    }

    // 200ms tick
    if (hwForceSampleTick())
    {
        Serial.println("[DEBUG] Force sample tick");
        //Force.loop();
    }

    // 50ms tick
    if (hwCentiSecoundTick())
    {
        Serial.println("[DEBUG] START Centi-second tick");
        pollKeyPressed();
        Assembly.processKeys();

        // 1s elapsed since the state was entered --> wait for key release before firing,
        // so the action doesn't run while the user is still pressing (e.g. shaking the sensor)
        bool actionDelayElapsed = (millis() - Assembly.stateStartMillis) >= 1000;
        if (actionDelayElapsed)
        {
            if (Assembly.state == StateTare && Assembly.keys[0].pressed)
            {
                Force.tare();
                Assembly.state = StateMeasure;
            }
            else if (Assembly.state == StateCalibrate && Assembly.keys[1].pressed)
            {
                Force.calibrate(Assembly.cfg.taraCalibrateKg * EARTH_GRAVITY_MPS2); // calibrate against the configured reference weight
                Assembly.state = StateMeasure;
            }
            else if (Assembly.state == StateReboot)
            {
                Serial.println("main.loop --> rebooting now");
                ESP.restart();
            }
        }
        Serial.println("[DEBUG] END Centi-second tick");
    }

    wifiLoop();

    handleWebServerClient();
}