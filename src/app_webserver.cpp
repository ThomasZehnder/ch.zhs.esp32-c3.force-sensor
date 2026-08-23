#include "app_webserver.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

#include "Global.h"
#include "display.h"
#include "HwInterface.h"

namespace
{
  WebServer server(80);

  long triggerActivityTime = 0;
  String lastDownloadFilename = "-";

#define ACTIVITY_LED_PIN LED_PIN

  void triggerActivity()
  {
    digitalWrite(ACTIVITY_LED_PIN, 0);
/*
    triggerActivityTime = millis();

    if (triggerActivityTime < 10000) //only during first 10s
    {
      Serial.print("[LED ACTIVITY] ");
      Serial.print(ACTIVITY_LED_PIN);
      Serial.print(" ");
      Serial.println(triggerActivityTime);
    }
*/
  }

  void logRequest(const String &path, const String &method = "GET")
  {
    Serial.print("[HTTP] ");
    Serial.print(method);
    Serial.print(" ");
    Serial.println(path);

    // Show on OLED for 500ms with uptime
    String displayPath = path;
    if (displayPath.length() > 16)
    {
      displayPath = displayPath.substring(0, 13) + "...";
    }

    // Format uptime as MM:SS
    unsigned long uptimeMs = millis();
    unsigned long mils = uptimeMs % 1000;
    unsigned long uptimeSec = uptimeMs / 1000;
    unsigned int minutes = (uptimeSec / 60) % 60;
    unsigned int seconds = uptimeSec % 60;

    char uptimeStr[16];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02u:%02u:%02u", minutes, seconds, mils);

    displayRenderQueued("[HTTP] " + method, displayPath, "", uptimeStr, 500);
  }

  void setAllowCors()
  {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  }

  String getContentType(const String &path)
  {
    if (path.endsWith(".html"))
      return "text/html";
    if (path.endsWith(".css"))
      return "text/css";
    if (path.endsWith(".js"))
      return "application/javascript";
    if (path.endsWith(".json"))
      return "application/json";
    if (path.endsWith(".png"))
      return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
      return "image/jpeg";
    if (path.endsWith(".gif"))
      return "image/gif";
    if (path.endsWith(".svg"))
      return "image/svg+xml";
    if (path.endsWith(".ico"))
      return "image/x-icon";
    if (path.endsWith(".pdf"))
      return "application/pdf";
    if (path.endsWith(".gz"))
      return "application/x-gzip";
    return "text/plain";
  }

  bool handleFileRead(String path)
  {
    logRequest(path);

    if (!path.startsWith("/"))
      path = "/" + path;

    if (path.endsWith("/"))
    {
      path += "index.html";
    }

    String pathWithGz = path + ".gz";
    String fileToOpen = path;

    if (LittleFS.exists(pathWithGz))
    {
      fileToOpen = pathWithGz;
    }
    else if (!LittleFS.exists(path))
    {
      Serial.print("LittleFS file not found: ");
      Serial.println(path);
      return false;
    }

    File file = LittleFS.open(fileToOpen, "r");
    if (!file)
    {
      Serial.print("LittleFS open failed: ");
      Serial.println(fileToOpen);
      return false;
    }

    const size_t fileSize = file.size();
    const String contentType = getContentType(path);
    Serial.print("Serving file: ");
    Serial.print(fileToOpen);
    Serial.print(" (");
    Serial.print(fileSize);
    Serial.println(" bytes)");

    if (path.endsWith(".json"))
      server.sendHeader("Cache-Control", "no-store");
    else
      server.sendHeader("Cache-Control", "public, max-age=432000");

    const size_t bytesSent = server.streamFile(file, contentType);
    file.close();

    Serial.print("Sent bytes: ");
    Serial.println(bytesSent);

    if (bytesSent != fileSize)
    {
      Serial.println("Stream incomplete.");
      return false;
    }

    return true;
  }

  void serveFileOr404(const String &path)
  {
    if (!handleFileRead(path))
    {
      server.send(404, "text/plain", "404: " + path + " not found");
    }
  }

  int httpRssi()
  {
    return WiFi.RSSI();
  }

  String state2Text(enMainState state)
  {
    switch (state)
    {
    case StateSetup:
      return "Setup";
    case StateMeasure:
      return "Measure";
    case StateTare:
      return "Tare";
    case StateCalibrate:
      return "Calibrate";
    case StateReboot:
      return "Reboot";
    default:
      return "Unknown";
    }
  }

  String getAssemblyJsonImpl()
  {
    doc.clear();
    doc["hostname"] = WiFi.getHostname();
    doc["deviceId"] = Assembly.deviceId;
    doc["localIp"] = Assembly.localIp;
    doc["ssid"] = Assembly.ssid;
    doc["compiledate"] = Assembly.compileDate;
    doc["millis"] = millis();
    doc["rssi"] = httpRssi();
    doc["wifiConnected"] = Assembly.wifiConnected;
    
    doc["key_1"] = Assembly.keys[0].pressed;
    doc["key_2"] = Assembly.keys[1].pressed;
    doc["key_cnt_1"] = Assembly.keys[0].pressedCounter;
    doc["key_cnt_2"] = Assembly.keys[1].pressedCounter;
    doc["cfg_index"] = Assembly.cfg.index;
    doc["wifi_0"] = Assembly.cfg.wifi[0].ssid;
    doc["wifi_1"] = Assembly.cfg.wifi[1].ssid;
    doc["wifi_2"] = Assembly.cfg.wifi[2].ssid;

    doc["state"] = Assembly.state;
    doc["stateText"] = state2Text(Assembly.state);
    doc["force"] = Assembly.force.value;
    doc["offset"] = Assembly.force.sensor.get_offset();
    doc["scale"] = Assembly.force.sensor.get_scale();

    JsonArray forceHistory = doc["forceHistory"].to<JsonArray>();
    int historyCount = Assembly.force.historyFull ? FORCE_HISTORY_SIZE : Assembly.force.historyIndex;
    int historyStart = Assembly.force.historyFull ? Assembly.force.historyIndex : 0;
    for (int i = 0; i < historyCount; i++)
    {
      forceHistory.add(Assembly.force.history[(historyStart + i) % FORCE_HISTORY_SIZE]);
    }

    String output;
    serializeJsonPretty(doc, output);
    return output;
  }

  void handleRoot()
  {
    triggerActivity();
    logRequest("/", "GET");
    serveFileOr404("/");
  }

  void handleJson()
  {
    triggerActivity();
    logRequest("/json", "GET");
    setAllowCors();
    server.send(200, "application/json", getAssemblyJsonImpl());
  }

  void handleAssembly()
  {
    triggerActivity();
    // logRequest("/assembly", "GET");
    setAllowCors();
    server.send(200, "application/json", getAssemblyJsonImpl());
  }

  void handleGetKeys()
  {
    triggerActivity();
    logRequest("/getkeys", "GET");
    String output = "{";
    output += "\"key_1\":" + String(Assembly.keys[0].pressed) + ",";
    output += "\"key_2\":" + String(Assembly.keys[1].pressed);
    output += "}";
    setAllowCors();
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", output);
  }

  void handleReboot()
  {
    triggerActivity();
    logRequest("/reboot", "GET");
    server.sendHeader("Cache-Control", "no-store");

    if (!server.hasArg("bootmode"))
    {
      Serial.println("[HTTP] reboot argument 'bootmode' not found!!");
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"bootmode argument missing\"}");
      return;
    }

    String arg = server.arg("bootmode");
    if (arg != "espreboot")
    {
      Serial.println("[HTTP] reboot argument 'bootmode' unknown: " + arg);
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"unknown bootmode\"}");
      return;
    }

    Assembly.rebootProcess();
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"rebooting\"}");
  }

  void handleFileUpload()
  {
    triggerActivity();
    HTTPUpload &upload = server.upload();

    static File fsUploadFile;

    if (upload.status == UPLOAD_FILE_START)
    {
      String filename = upload.filename;
      if (!filename.startsWith("/"))
        filename = "/" + filename;
      logRequest("/upload", "POST");
      Serial.print("[HTTP] Upload filename: ");
      Serial.println(filename);
      lastDownloadFilename = filename;
      fsUploadFile = LittleFS.open(filename, "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      if (fsUploadFile)
        fsUploadFile.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      if (fsUploadFile)
      {
        fsUploadFile.close();
        Serial.print("[HTTP] Upload complete. Size: ");
        Serial.println(upload.totalSize);
        displayRenderQueued("[HTTP] Upload OK", lastDownloadFilename, "", "", 500);
        server.sendHeader("Location", "/success");
        server.send(303);
      }
      else
      {
        server.send(500, "text/plain", "500: couldn't create file");
      }
    }
  }

  void handleSuccess()
  {
    triggerActivity();
    logRequest("/success", "GET");
    String msg = "<h1>Upload Result</h1>";
    msg += "Last uploaded file: " + lastDownloadFilename;
    msg += "<br><a href=\"/a-upload.html\">Upload next file.</a>";
    msg += "<br><a href=\"/dir\">Pure directory</a>";
    msg += "<br><a href=\"/\">Back to main page.</a>";
    server.send(200, "text/html", msg);
  }

  void handleDir()
  {
    triggerActivity();
    logRequest("/dir", "GET");
    String msg = "directory root: <table><tr><th>FILE</th><th>SIZE</th></tr>";
    Serial.println("[HTTP] Listing directory: /");

    File root = LittleFS.open("/");
    if (!root || !root.isDirectory())
    {
      server.send(500, "text/html", "Cannot open directory");
      return;
    }

    File file = root.openNextFile();
    while (file)
    {
      if (!file.isDirectory())
      {
        Serial.print(" FILE: ");
        Serial.print(file.name());
        Serial.print(" SIZE: ");
        char sz[200];
        ltoa(file.size(), sz, 10);
        msg += "<tr><td><a href=\"" + String(file.name()) + "\">" + String(file.name()) + "</a> </td><td>" + sz + "</td></tr>";
        Serial.println(sz);
      }
      file = root.openNextFile();
    }
    Serial.println("");

    msg += "</table>";
    server.send(200, "text/html", msg);
  }

  void handleFileStore()
  {
    triggerActivity();
    logRequest("/store", "POST");

    if (!server.hasArg("plain"))
    {
      Serial.println("[HTTP] Store: No content in body");
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No content in body\"}");
      return;
    }

    if (!server.hasArg("name"))
    {
      Serial.println("[HTTP] Store: filename argument missing");
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"filename argument missing\"}");
      return;
    }

    String filename = server.arg("name");
    if (!filename.startsWith("/"))
      filename = "/" + filename;

    String content = server.arg("plain");

    Serial.print("[HTTP] Store: Writing ");
    Serial.print(content.length());
    Serial.print(" bytes to ");
    Serial.println(filename);

    File file = LittleFS.open(filename, "w");
    if (!file)
    {
      Serial.print("[HTTP] Store: Failed to open ");
      Serial.println(filename);
      server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to open file\"}");
      return;
    }

    size_t written = file.print(content);
    file.close();

    if (written == content.length())
    {
      Serial.print("[HTTP] Store: Successfully wrote ");
      Serial.print(written);
      Serial.print(" bytes to ");
      Serial.println(filename);
      displayRenderQueued("[HTTP] Store OK", filename, String(written) + " bytes", "", 500);
      server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"File stored\"}");
    }
    else
    {
      Serial.print("[HTTP] Store: Write mismatch - expected ");
      Serial.print(content.length());
      Serial.print(" but wrote ");
      Serial.println(written);
      server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Write incomplete\"}");
    }
  }

  void handleNotFound()
  {
    triggerActivity();
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += server.method();
    message += " : ";
    message += (server.method() == HTTP_GET) ? "GET" : (server.method() == HTTP_POST) ? "POST"
                                                   : (server.method() == HTTP_PATCH)  ? "PATCH"
                                                                                      : "OTHER";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    Serial.println(message);
  }

} // namespace

String getAssemblyJson()
{
  return getAssemblyJsonImpl();
}

void setupWebServer()
{
  Serial.println("setupWebServer --> Start");

  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(ACTIVITY_LED_PIN, 1);

  // WiFi is managed by WiFiService.cpp via event handlers
  // Just ensure we're in station mode
  WiFi.mode(WIFI_STA);

  // Setup HTTP routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/json", HTTP_GET, handleJson);
  server.on("/assembly", HTTP_GET, handleAssembly);
  server.on("/getkeys", HTTP_GET, handleGetKeys);
  server.on("/reboot", HTTP_GET, handleReboot);
  server.on("/success", HTTP_GET, handleSuccess);
  server.on("/dir", HTTP_GET, handleDir);

  server.on("/upload", HTTP_GET, []()
            {
    triggerActivity();
    if (!handleFileRead("/a-upload.html"))
      server.send(404, "text/plain", "404: Not Found"); });

  server.on("/upload", HTTP_POST, []()
            { server.send(200); }, handleFileUpload);

  server.on("/store", HTTP_POST, handleFileStore);

  server.onNotFound([]()
                    {
    if (!handleFileRead(server.uri()))
      handleNotFound(); });

  server.begin();
  Serial.println("setupWebServer --> HTTP server started...");
  Serial.println("setupWebServer --> End");
}

void handleWebServerClient()
{
  // WiFi is managed by WiFiService.cpp via event handlers
  server.handleClient();

  // Activity LED management
  if (triggerActivityTime != 0)
  {
    if ((long)millis() - triggerActivityTime - 50 > 0)
    {
      digitalWrite(ACTIVITY_LED_PIN, 1);
      triggerActivityTime = 0;
    }
  }
}
