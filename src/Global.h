#ifndef GLOBAL_H
#define GLOBAL_H
// global object definition
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HX711.h>

#define CPP_VERSION "V1.00.00 "

struct tstCfgWifi
{
    char ssid[32];
    char pw[32];
};

#define NBRCONNECTIONS 3
struct tstCfg
{
    tstCfgWifi wifi[NBRCONNECTIONS];
    bool accessPointEnabled = true;
    float scale = 1.0;         // HX711 calibration factor (counts per Newton), persisted via config_main.json, SCALE
    long offset = 0;           // HX711 raw offset, persisted via config_main.json, OFFSET
    float taraCalibrateKg = 1.0; // reference weight used for calibration, configurable via config_main.json, TARA_CALIBRATE_KG
    byte index = 0; // index depends on found WIFI Network
};

#define NBRKEYS 2
struct tstKey
{
    int pin;
    bool edge;
    bool pressed;
    int pressedCounter;
};

// rolling history at 200ms sampling interval, covering the last 10 seconds
#define FORCE_HISTORY_SIZE 50
struct tstForce
{
    float value; // measured force in Newton
    HX711 sensor;

    float history[FORCE_HISTORY_SIZE] = {0}; // circular buffer, oldest to newest via historyIndex/historyFull
    int historyIndex = 0;                    // next slot to write
    bool historyFull = false;                // true once the buffer has wrapped around at least once
};

enum enMainState
{
    StateSetup,
    StateMeasure,
    StateTare,
    StateCalibrate,
    StateReboot
};

class clAssembly
{
public:
    String compileDate;
    char deviceId[32];

    bool wifiConnected = false;

    String localIp;
    String ssid;
    String apSsid;
    String apIp;

    // true once no configured WiFi was found at boot - the device commits to Access Point
    // only mode and stops retrying, see httpServerSetup()/httpServerLoop()
    bool apOnlyMode = false;

    struct tstKey keys[NBRKEYS];

    tstCfg cfg;

    tstForce force;

    enMainState state = StateSetup;
    unsigned long stateStartMillis = 0; // millis() timestamp when the current state was entered

public:
    void setup();
    void saveConfig(); // persist deviceId/cfg (incl. scale/offset) to config_main.json

    void rebootProcess();
    void wlanConnectedProcess();

    void processKeys();

private:
    void setupDevice();
    void setupWifi();
};

extern clAssembly Assembly;

// Global JSON document for configuration
extern JsonDocument doc;

#endif // GLOBAL_H