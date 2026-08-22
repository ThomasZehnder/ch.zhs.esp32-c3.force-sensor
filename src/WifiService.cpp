#include <WiFi.h>

#include "credentials.h"

#include "Global.h"
#include "display.h"

void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  switch (event)
  {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    {
      Assembly.wifiConnected = true;
      Assembly.localIp = WiFi.localIP().toString();
      Assembly.ssid = WiFi.SSID();

      Serial.println("[WIFI] ===== CONNECTED =====");
      Serial.print("[WIFI] SSID: ");
      Serial.println(Assembly.ssid);
      Serial.print("[WIFI] IP Address: ");
      Serial.println(Assembly.localIp);
      int rssiValue = WiFi.RSSI();
      Serial.print("[WIFI] Signal Strength: ");
      Serial.print(rssiValue);
      Serial.println(" dBm");

      // Show WiFi info on OLED for 2 seconds via queue
      char rssiStr[16];
      snprintf(rssiStr, sizeof(rssiStr), "%d dBm", rssiValue);
      renderDisplayQueued("WiFi Connected", Assembly.localIp, Assembly.ssid, rssiStr, 5000);

      byte cfgIndex = 0;
      for (cfgIndex = 0; cfgIndex < (sizeof(Assembly.cfg.wifi) / sizeof(Assembly.cfg.wifi[0])); cfgIndex++)
      {
        if (WiFi.SSID() == Assembly.cfg.wifi[cfgIndex].ssid)
        {
          Serial.print("[WIFI] Config index: ");
          Serial.println(cfgIndex);
          Assembly.cfg.index = cfgIndex;
          break;
        }
      }
      Serial.println("[WIFI] ======================");
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    {
      Assembly.wifiConnected = false;
      Serial.println("[WIFI] [!] DISCONNECTED from WiFi");
      break;
    }

    default:
      break;
  }
}

void wifiSetup()
{
  Serial.println("WifiSetup --> Start");

  WiFi.setAutoReconnect(false);
  WiFi.onEvent(onWifiEvent);

  Serial.println("[WIFI] Event handlers registered");
  Serial.print("[WIFI] WiFi mode: ");
  Serial.println(WiFi.getMode());

  // Start WiFi connection with first configured network
  if (Assembly.cfg.wifi[0].ssid[0] != '\0')
  {
    Serial.print("[WIFI] Starting WiFi.begin() with SSID: ");
    Serial.println(Assembly.cfg.wifi[0].ssid);

    // Show on OLED immediately
    renderDisplayQueued("[WIFI] Searching", Assembly.cfg.wifi[0].ssid, "Attempt:", "(1)/1/2", 2000);

    WiFi.begin(Assembly.cfg.wifi[0].ssid, Assembly.cfg.wifi[0].pw);
  }
  else
  {
    Serial.println("[WIFI] No WiFi SSID configured!");
    renderDisplayQueued("[WIFI] Error", "No SSID", "configured", "", 3000);
    
  }

  Serial.println("WifiSetup --> End");
}


void wifiLoop()
{
  static unsigned long lastWifiAttemptTime = 0;
  static int currentWifiIndex = 0;
  static int wifiAttemptRound = 0;  // 0-2 for 3 attempts through all networks
  static bool apModeActivated = false;
  static unsigned long lastStatusCheck = 0;

  const unsigned long WIFI_ATTEMPT_TIMEOUT_MS = 15000;  // 15 seconds per network
  const int MAX_WIFI_ROUNDS = 2;

  // If already connected, nothing to do
  if (WiFi.status() == WL_CONNECTED)
  {
    lastWifiAttemptTime = 0;
    return;
  }

  // Check if we've exhausted all attempts
  if (wifiAttemptRound >= MAX_WIFI_ROUNDS && !apModeActivated)
  {
    Serial.println("[WIFI] All WiFi attempts exhausted, activating Access Point mode");
    apModeActivated = true;

    // Setup Access Point
    String apSsid = String("force_sensor_") + Assembly.deviceId;
    if (WiFi.softAP(apSsid.c_str(), "", 1, false, 1))
    {
      Serial.print("[WIFI] Access Point created: ");
      Serial.println(apSsid);
      Assembly.apIp = WiFi.softAPIP().toString();
      renderDisplayQueued("AP Mode Active", Assembly.apIp, apSsid,  "", 5000);
    }
    return;
  }

  // Try next WiFi network if timeout elapsed
  if ((long)(millis() - lastWifiAttemptTime) >= WIFI_ATTEMPT_TIMEOUT_MS)
  {
    lastWifiAttemptTime = millis();

    // Move to next network
    currentWifiIndex++;
    if (currentWifiIndex >= 3)
    {
      currentWifiIndex = 0;
      wifiAttemptRound++;
    }

    // Check if this network is configured
    if (Assembly.cfg.wifi[currentWifiIndex].ssid[0] != '\0')
    {
      Serial.print("[WIFI] Attempting network ");
      Serial.print(wifiAttemptRound + 1);
      Serial.print("/");
      Serial.print(MAX_WIFI_ROUNDS);
      Serial.print(" - ");
      Serial.println(Assembly.cfg.wifi[currentWifiIndex].ssid);

      // Show on OLED
      char attemptStr[16];
      snprintf(attemptStr, sizeof(attemptStr), "(%d)/%d/%d", currentWifiIndex +1, wifiAttemptRound + 1, MAX_WIFI_ROUNDS);
      renderDisplayQueued("[WIFI] Searching", Assembly.cfg.wifi[currentWifiIndex].ssid, "Attempt:", attemptStr, 2000);

      // Try to connect
      WiFi.begin(Assembly.cfg.wifi[currentWifiIndex].ssid, Assembly.cfg.wifi[currentWifiIndex].pw);
    }
  }

  // Periodic status debug output
  if ((long)(millis() - lastStatusCheck) >= 10000)
  {
    lastStatusCheck = millis();
    Serial.print("[WIFI] Status check - WiFi status: ");
    Serial.print(WiFi.status());
    Serial.print(" | Round: ");
    Serial.print(wifiAttemptRound + 1);
    Serial.print("/");
    Serial.print(MAX_WIFI_ROUNDS);
    Serial.print(" | Index: ");
    Serial.println(currentWifiIndex);
  }
}
