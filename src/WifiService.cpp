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
      Serial.print("[WIFI] Signal Strength: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");

      // Show WiFi info on OLED for 2 seconds via queue
      renderDisplayQueued("WiFi Connected", Assembly.ssid, Assembly.localIp, "", 2000);

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
    WiFi.begin(Assembly.cfg.wifi[0].ssid, Assembly.cfg.wifi[0].pw);
  }
  else
  {
    Serial.println("[WIFI] No WiFi SSID configured!");
  }

  Serial.println("WifiSetup --> End");
}


void wifiLoop()
{
  // Check WiFi status periodically for debug
  static unsigned long lastStatusCheck = 0;
  if ((long)(millis() - lastStatusCheck) >= 10000)
  {
    lastStatusCheck = millis();
    Serial.print("[WIFI] Status check - WiFi status: ");
    Serial.print(WiFi.status());
    Serial.print(" | Connected: ");
    Serial.println(Assembly.wifiConnected);
  }
}
