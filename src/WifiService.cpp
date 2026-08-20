#include <WiFi.h>

#include "credentials.h"

#include "Global.h"

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
  Serial.println("WifiSetup --> End");
}


void wifiLoop()
{
  // nothing, all "event based"
}
