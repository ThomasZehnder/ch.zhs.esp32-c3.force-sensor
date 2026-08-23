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
    renderDisplay("ESP32-C3 OLED", "**", "****", "****");
    delay(300); // to enable serial out

    Serial.println();
    Serial.println("[DEBUG] === SETUP START ===");

    Serial.println("[DEBUG] Assembly.setup()...");
    Assembly.setup(); // read config file

    Serial.println("[DEBUG] initDisplay()...");
    initDisplay();
    renderDisplay("FORCE SENSOR",  Assembly.deviceId, "DeviceId", "...");

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

    hwLoop();

    if (hwSecoundTick())
    {
        // Serial.println("[DEBUG] Second tick");
        if (Assembly.state == StateSetup)
        {
            Serial.println("[STATE CHANGE ] StateSetup --> StateMeasure");
            Assembly.state = StateMeasure;
        }
    }

    // 200ms tick
    if (hwForceSampleTick())
    {
        // Serial.println("[DEBUG] Force sample tick");
        Force.loop();
        if ((Assembly.state==StateMeasure)&&(Assembly.wifiConnected)) {
            String sForce = String(Assembly.force.value, 1) + "N";
            renderDisplayQueued("#Force", sForce.c_str(), "", Assembly.localIp, 10);
        }
    }

    // 50ms tick
    if (hwCentiSecoundTick())
    {
        // Serial.println("[DEBUG] START Centi-second tick");
        pollKeyPressed();

        Assembly.processKeys(); //transition do calibrate and tara force sensor

        // Serial.println("[DEBUG] END Centi-second tick");
    }

    wifiLoop();

    handleWebServerClient();
}