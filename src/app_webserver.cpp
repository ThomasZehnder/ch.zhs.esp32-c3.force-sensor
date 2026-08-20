#include "app_webserver.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>



namespace
{
WebServer server(80);
bool *filesystemMounted = nullptr;
bool *wifiConnected = nullptr;
String *selectedSound = nullptr;
SoundTriggerCallback soundTrigger = nullptr;

void logRequest(const String &path)
{
    Serial.print("HTTP request: ");
    Serial.println(path);
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
    return "text/plain";
}

bool handleFileRead(String path)
{
    logRequest(path);

    if (!filesystemMounted || !*filesystemMounted)
    {
        Serial.println("LittleFS not mounted; cannot serve file.");
        return false;
    }

    if (path.endsWith("/"))
    {
        path += "index.html";
    }

    if (!LittleFS.exists(path))
    {
        Serial.print("LittleFS file not found: ");
        Serial.println(path);
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        Serial.print("LittleFS open failed: ");
        Serial.println(path);
        return false;
    }

    const size_t fileSize = file.size();
    const String contentType = getContentType(path);
    Serial.print("Serving file: ");
    Serial.print(path);
    Serial.print(" (");
    Serial.print(fileSize);
    Serial.println(" bytes)");

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

void registerStaticRoute(const char *routePath)
{
    server.on(routePath, HTTP_GET, [routePath]()
              { serveFileOr404(String(routePath)); });
}

} // namespace

namespace
{

void assemblyJson()
{
    setAllowCors();
    server.send(200, "application/json", getAssemblyJson());
}


void handleWebServerClient()
{
    server.handleClient();
}