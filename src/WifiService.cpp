#include <WiFi.h>

#include "credentials.h"

#include "Global.h"

void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  switch (event)
  {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    {
      Serial.println("onWifiEvent --> Connected to WiFi: " + WiFi.SSID());
      Serial.print("onWifiEvent --> IP address: ");
      Serial.println(WiFi.localIP());
      Assembly.wifiConnected = true;
      Assembly.localIp = WiFi.localIP().toString();
      Assembly.ssid = WiFi.SSID();

      byte cfgIndex = 0;
      for (cfgIndex = 0; cfgIndex < (sizeof(Assembly.cfg.wifi) / sizeof(Assembly.cfg.wifi[0])); cfgIndex++)
      {
        if (WiFi.SSID() == Assembly.cfg.wifi[cfgIndex].ssid)
        {
          Serial.print("onWifiEvent --> use cfg index: ");
          Serial.println(cfgIndex);
          Assembly.cfg.index = cfgIndex;
          break;
        }
      }
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    {
      Serial.println("onWifiEvent --> Disconnected from WiFi.");
      Assembly.wifiConnected = false;
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
