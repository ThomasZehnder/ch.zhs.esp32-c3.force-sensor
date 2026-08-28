#include <Arduino.h>

#include "app_webserver.h"

#include "Force.h"
#include "HwInterface.h"

#include <WiFiService.h>

#include "Global.h"
#include "display.h"
#include "TftDisplay.h"

void setup()
{
    Serial.begin(115200);
    displayRender("ESP32-C3 OLED", "**", "****", "****");
    delay(300); // to enable serial out

    Serial.println();
    Serial.println("[DEBUG] === SETUP START ===");

    Serial.println("[DEBUG] Assembly.setup()...");
    Assembly.setup(); // read config file

    Serial.println("[DEBUG] initDisplay()...");
    initDisplay();
    displayRender("FORCE SENSOR", Assembly.deviceId, "DeviceId", "...");

    Serial.println("[DEBUG] hwSetup()...");
    hwSetup(); // configures the button pins - needed before the hold-check below

    Serial.println("[DEBUG] tftDisplaySetup()...");
    tftDisplaySetup(Assembly.cfg.theme);
    tftDisplayTest(Assembly.compileDate.c_str());
    delay(1000); // minimum time the startup screen stays visible
    while (digitalRead(KEY1_PIN) == LOW || digitalRead(KEY2_PIN) == LOW)
    {
        delay(50); // keep showing the startup screen while a button is held
    }

    Serial.println("[DEBUG] wifiSetup()...");
    wifiSetup();

    Serial.println("[DEBUG] setupWebServer()...");
    setupWebServer();

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
        if (Assembly.state == StateMeasure)
        {
            String ipText = Assembly.wifiConnected ? Assembly.localIp
                            : (Assembly.apIp != "") ? Assembly.apIp
                                                     : "not WIFI...";

            // unwrap the circular history buffer into chronological (oldest to newest) order
            float orderedHistory[FORCE_HISTORY_SIZE];
            int historyCount = Assembly.force.historyFull ? FORCE_HISTORY_SIZE : Assembly.force.historyIndex;
            int historyStart = Assembly.force.historyFull ? Assembly.force.historyIndex : 0;
            for (int i = 0; i < historyCount; i++)
            {
                orderedHistory[i] = Assembly.force.history[(historyStart + i) % FORCE_HISTORY_SIZE];
            }
            tftDisplayShowForce(Assembly.force.value, orderedHistory, historyCount, ipText.c_str());

            if (isDisplayQueueEmpty())
            {
                String sForce = String(Assembly.force.value, 1) + "N";
                displayRenderQueued("#Force", sForce.c_str(), "", ipText, 10);
            }
        }
    }

    // 50ms tick
    if (hwCentiSecoundTick())
    {
        // Serial.println("[DEBUG] START Centi-second tick");
        pollKeyPressed();

        Assembly.processKeys(); // transition do calibrate and tara force sensor

        // Serial.println("[DEBUG] END Centi-second tick");
    }

    wifiLoop();

    handleWebServerClient();
}