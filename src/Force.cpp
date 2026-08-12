#include <Arduino.h>

#include "Force.h"
#include "HwInterface.h"
#include "Global.h"

clForce Force;

void clForce::setup()
{
    Assembly.force.sensor.begin(FORCE_DOUT_PIN, FORCE_SCK_PIN);
    // load the persisted calibration instead of blindly tare-ing on every boot
    Assembly.force.sensor.set_scale(Assembly.cfg.scale);
    Assembly.force.sensor.set_offset(Assembly.cfg.offset);
}

void clForce::loop()
{
    if (Assembly.force.sensor.is_ready())
    {
        Assembly.force.value = Assembly.force.sensor.get_units(1);

        Assembly.force.history[Assembly.force.historyIndex] = Assembly.force.value;
        Assembly.force.historyIndex = (Assembly.force.historyIndex + 1) % FORCE_HISTORY_SIZE;
        if (Assembly.force.historyIndex == 0)
        {
            Assembly.force.historyFull = true;
        }
    }
}

void clForce::clearHistory()
{
    Assembly.force.historyIndex = 0;
    Assembly.force.historyFull = false;
}

void clForce::tare()
{
    // wait_ready() (called internally by tare()/read()) blocks forever with no timeout,
    // which trips the watchdog if the HX711 is not wired up / not powered yet
    if (Assembly.force.sensor.wait_ready_timeout(1000))
    {
        Assembly.force.sensor.tare();
        clearHistory();

        Assembly.cfg.offset = Assembly.force.sensor.get_offset();
        Assembly.saveConfig();

        Serial.print("Force.tare --> done, offset: ");
        Serial.println(Assembly.force.sensor.get_offset());
    }
    else
    {
        Serial.println("Force.tare --> HX711 not responding, tare skipped");
    }
}

void clForce::calibrate(float knownForceNewton)
{
    if (knownForceNewton == 0)
    {
        return;
    }

    if (!Assembly.force.sensor.wait_ready_timeout(1000))
    {
        Serial.println("Force.calibrate --> HX711 not responding, calibration skipped");
        return;
    }

    long rawAverage = Assembly.force.sensor.read_average(10);
    float calibrationFactor = (rawAverage - Assembly.force.sensor.get_offset()) / knownForceNewton;
    Assembly.force.sensor.set_scale(calibrationFactor);
    clearHistory();

    Assembly.cfg.scale = calibrationFactor;
    Assembly.saveConfig();

    Serial.print("Force.calibrate --> new calibration factor: ");
    Serial.println(calibrationFactor);
}

