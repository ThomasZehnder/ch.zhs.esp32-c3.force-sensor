#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <LittleFS.h>

#include <ArduinoJson.h>
#include <WiFiService.h>
#include "HttpServer.h"

#include "credentials.h"



#include "Global.h"



WebServer server(80);
WiFiMulti wifiMulti;

File fsUploadFile; // a File object to temporarily store the received file

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the LittleFS
void handleFileStore();                 // store content to file

#define ACTIVITY_LED_PIN LED_BUILTIN

long triggerActivityTime = 0;

String lastDownloadFilename = "-";

void triggerActivity()
{
  digitalWrite(ACTIVITY_LED_PIN, 0);
  triggerActivityTime = millis();
}

int httpRssi(void)
{
  return WiFi.RSSI();
}


void handleJson()
{
  triggerActivity();

  // see: https://arduinojson.org/

  doc.clear();
  doc["device_id"] = Assembly.deviceId;
  doc["millis"] = millis();
  doc["rssi"] = httpRssi();
  doc["accesspoint_ip"] = WiFi.softAPIP().toString();

  // Create the "digital" array
  JsonArray digitalValues = doc.createNestedArray("digital");
  for (int pin = 0; pin < 17; pin++)
  {
    // Read the digital input
    int value = digitalRead(pin);

    // Add the value at the end of the array
    digitalValues.add(value);
    // digitalValues.add(pin);
    // digitalValues.add("pin");
  }

  // Serial.print(F("Sending: "));
  // serializeJson(doc, Serial);
  // Serial.println();

  // Lastly, you can print the resulting JSON to a String
  String output;
  serializeJson(doc, output);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", output);
}

void setAllowCors()
{
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Replace '*' with your allowed domain
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
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

void assemblyJson()
{
  triggerActivity();

  // see: https://arduinojson.org/
  doc.clear();

  doc["hostname"] = WiFi.getHostname();
  doc["deviceId"] = Assembly.deviceId;
  doc["accesspoint_enable"] = Assembly.cfg.accessPointEnabled;
  doc["localIp"] = Assembly.localIp;
  doc["ssid"] = Assembly.ssid;
  doc["compiledate"] = Assembly.compileDate;

  doc["millis"] = millis();
  doc["rssi"] = httpRssi();

  doc["wifiConnected"] = Assembly.wifiConnected;
  doc["state"] = Assembly.state;
  doc["stateText"] = state2Text(Assembly.state);

  doc["force"] = Assembly.force.value;
  doc["offset"] = Assembly.force.sensor.get_offset();
  doc["scale"] = Assembly.force.sensor.get_scale();

  // rolling history, chronological order (oldest to newest), 200ms interval covering the last 10s
  JsonArray forceHistory = doc.createNestedArray("forceHistory");
  int historyCount = Assembly.force.historyFull ? FORCE_HISTORY_SIZE : Assembly.force.historyIndex;
  int historyStart = Assembly.force.historyFull ? Assembly.force.historyIndex : 0;
  for (int i = 0; i < historyCount; i++)
  {
    forceHistory.add(Assembly.force.history[(historyStart + i) % FORCE_HISTORY_SIZE]);
  }

  doc["key_1"] = Assembly.keys[0].pressed;
  doc["key_2"] = Assembly.keys[1].pressed;

  doc["key_cnt_1"] = Assembly.keys[0].pressedCounter;
  doc["key_cnt_2"] = Assembly.keys[1].pressedCounter;

  doc["cfg_index"] = Assembly.cfg.index;

  doc["wifi_0"] = Assembly.cfg.wifi[0].ssid;
  doc["wifi_1"] = Assembly.cfg.wifi[1].ssid;
  doc["wifi_2"] = Assembly.cfg.wifi[2].ssid;


  // Lastly, you can print the resulting JSON to a String, use the "pretty" variant for readable outputs in browser
  String output;
  serializeJsonPretty(doc, output);
  setAllowCors();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", output);
}

void handleRoot()
{
  triggerActivity();
  handleFileRead("index.html");
  // server.send(200, "text/plain", "hello from esp32-c3!");
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
                                                                                    : "seeNbr";
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

void reboot(void)
{
  triggerActivity();
  server.sendHeader("Cache-Control", "no-store");

  if (!server.hasArg("bootmode"))
  {
    Serial.println("reboot argument 'bootmode' not found!!");
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"bootmode argument missing\"}");
    return;
  }

  String arg = server.arg("bootmode");
  if (arg != "espreboot")
  {
    Serial.println("reboot argument 'bootmode' unknown: " + arg);
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"unknown bootmode\"}");
    return;
  }

  Assembly.rebootProcess();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"rebooting\"}");
}

void success(void)
{
  triggerActivity();
  String msg = "<h1>Upload Result</h1>";
  msg += "Last uploaded file: " + lastDownloadFilename;
  msg += "<br><a href=\"/a-upload.html\">Upload next file.</a>";
  msg += "<br><a href=\"/dir\">Pure directory</a>";
  msg += "<br><a href=\"/\">Back to main page.</a>";
  server.send(200, "text/html", msg);
}

void dir(void)
{
  triggerActivity();

  String msg = "directory root: <table><tr><th>FILE</th><th>SIZE</th></tr>";
  const char dirname[] = "/";
  Serial.printf("Listing directory: %s\n", dirname);

  File root = LittleFS.open(dirname);
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
void subscriptionList(void)
{
  triggerActivity();

  String msg = "<pre> NO Subscriptions";

  msg += "</pre>";
  server.send(200, "text/html", msg);
}

void _getKeys()
{
  triggerActivity();

  String output;

  // build manualy
  output += "{";
  output += "\"key_1\":" + String(Assembly.keys[0].pressed) + ",";
  output += "\"key_2\":" + String(Assembly.keys[1].pressed);
  output += "}";

  setAllowCors();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", output);
}

void httpServerSetup(void)
{

  Serial.println("HttpSetup --> Start");
  // SSID = DEVICEID + last two hex digits of the AP MAC address, to tell devices apart
  String apMac = WiFi.softAPmacAddress();
  Assembly.apSsid = String(Assembly.deviceId) + apMac.substring(apMac.length() - 2);

  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(ACTIVITY_LED_PIN, 1);

  for (byte i = 0; i < (sizeof(Assembly.cfg.wifi) / sizeof(Assembly.cfg.wifi[0])); i++)
  {
    wifiMulti.addAP(Assembly.cfg.wifi[i].ssid, Assembly.cfg.wifi[i].pw); // add Wi-Fi networks you want to connect to, see credentials.h
  }

  Serial.println("HttpSetup --> Connecting Wifi Multi part...");
  bool wifiConnectError = true;
  if (wifiMulti.run(2000) == WL_CONNECTED)
  {
    Serial.println("HttpSetup --> connected Wifi ...");
    wifiConnectError = false;
    // get Wifi Informations
    Serial.print("HttpSetup  --> IP address: ");
    Serial.println(WiFi.localIP());
    Assembly.wifiConnected = true;

    Assembly.wlanConnectedProcess();
  }

  if (Assembly.cfg.accessPointEnabled || wifiConnectError)
  {
    Serial.println("HttpSetup --> Setting the AP Mode with SSID, NO Password...");

    // Setting the AP Mode with SSID, Password, and Max Connection Limit
    Serial.print("HttpSetup  --> AP ACCESS SSID: ");
    Serial.println(Assembly.apSsid);
    if (WiFi.softAP(Assembly.apSsid.c_str(), "", 1, false, 1) == true)
    {
      Assembly.apIp = WiFi.softAPIP().toString();
      Serial.print("Access Point is Created with MAC ADDR: ");
      Serial.println(apMac);
      Serial.print("Access Point IP: ");
      Serial.println(Assembly.apIp);
    }
    else
    {
      Serial.println("Unable to Create Access Point");
    }
  }

  // no configured WiFi found at boot -> commit to Access Point only mode, never retry (see httpServerLoop())
  Assembly.apOnlyMode = wifiConnectError;

  Serial.println("HttpSetup --> Mound File System: LittleFS");
  // Start the SPI Flash Files System
  if (!LittleFS.begin())
  {
    Serial.println("HttpSetup --> An Error has occurred while mounting LittleFS");
    delay(1000);
  }

  // Setup Http Server
  server.on("/", handleRoot);
  server.on("/json", handleJson); // test only
  server.on("/assembly", assemblyJson);

  server.on("/inline", []()
            { server.send(200, "text/plain", "this works as well"); });

  server.on("/reboot", reboot);

  server.on("/success", success);
  server.on("/dir", dir);

  server.on("/subscriptionList", subscriptionList);

  server.on("/getkeys", _getKeys);

  server.on("/upload", HTTP_GET, []() { // if the client requests the upload page
    triggerActivity();
    if (!handleFileRead("/a-upload.html"))              // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on(
      "/upload", HTTP_POST, // if the client posts to the upload page
      []()
      { server.send(200); }, // Send status 200 (OK) to tell the client we are ready to receive
      handleFileUpload       // Receive and save the file
  );

  server.on("/store", HTTP_POST, handleFileStore);

  // server.onNotFound(handleNotFound);
  server.onNotFound([]() {             // If the client requests any URI
    if (!handleFileRead(server.uri())) // send it if it exists
      handleNotFound();                // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();
  Serial.println("HttpSetup --> HTTP server started...");

  Serial.println("HttpSetup --> End");
}

void httpServerLoop(void)
{
  // connect to strongest wifi, and reconnect.
  // wifiMulti.run() returns immediately while connected, but while disconnected it blocks for
  // several seconds (WiFi scan + a connect attempt per configured network) -
  // calling it every loop() iteration would then stall everything else (force sampling, OLED, HTTP)
  // almost permanently, so retries are throttled while there is no connection.
  // If no configured WiFi was found already at boot, Assembly.apOnlyMode is set and no further
  // attempts are made at all - the device stays in Access Point only mode until rebooted.
  static unsigned long nextWifiRetryMillis = 0;
  if (!Assembly.apOnlyMode && (WiFi.status() == WL_CONNECTED || (long)(millis() - nextWifiRetryMillis) >= 0))
  {
    nextWifiRetryMillis = millis() + 60000;
    wifiMulti.run();
  }

  server.handleClient();

  if (triggerActivityTime != 0)
  {
    if ((long)millis() - triggerActivityTime - 200 > 0)
    {
      digitalWrite(ACTIVITY_LED_PIN, 1);
      triggerActivityTime = 0;
    }
  }
}

String getContentType(String filename)
{ // convert the file extension to the MIME type
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".pdf"))
    return "application/pdf";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".svg"))
    return "image/svg+xml";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".json"))
    return "application/json";
  return "text/plain";
}

bool handleFileRead(String path)
{ // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (!path.startsWith("/"))
    path = "/" + path;                        // Ensure path starts with /
  if (path.endsWith("/"))
    path += "index.html";                    // If a folder is requested, send the index file
  String contentType = getContentType(path); // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path))
  {                                                     // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                    // If there's a compressed version available
      path += ".gz";                                    // Use the compressed verion
    File file = LittleFS.open(path, "r");               // Open the file
    if (path.endsWith(".json"))
      server.sendHeader("Cache-Control", "no-store"); // editable via the config UI, never cache
    else
      server.sendHeader("Cache-Control", "public, max-age=432000"); // 5 days
    size_t sent = server.streamFile(file, contentType); // Send it to the client
    file.close();                                       // Close the file again
    Serial.println(String("\tSent file: ") + path + " size: " + sent);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path); // If the file doesn't exist, return false
  return false;
}

// multipart from Form
void handleFileUpload()
{ // upload a new file to the LittleFS
  triggerActivity();
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    Serial.print("handleFileUpload Name: ");
    Serial.println(filename);
    lastDownloadFilename = filename;
    fsUploadFile = LittleFS.open(filename, "w"); // Open the file for writing in LittleFS (create if it doesn't exist)
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {                       // If the file was successfully created
      fsUploadFile.close(); // Close the file again
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success"); // Redirect the client to the success page
      server.send(303);
    }
    else
    {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

// content in body
void handleFileStore()
{ // upload a new file to the LittleFS
  Serial.println("handleFileStore(): ");
  /*
    String message = "";
    message += "Args Count: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    Serial.println(message);
  */

  // arg - get request argument value, use arg("plain") to get POST body
  /*
  String c = server.arg("plain");
  Serial.println("body: " + c);
  */

  if (!server.hasArg("name"))
  {
    Serial.println("handleFileUpload argument 'name' not found!!");
    server.send(500, "text/plain", "500: filename not defined ");
    return;
  }

  String filename = server.arg("name");
  if (!filename.startsWith("/"))
    filename = "/" + filename;
  Serial.print("handleFileUpload Name: ");
  Serial.println(filename);
  fsUploadFile = LittleFS.open(filename, "w"); // Open the file for writing in LittleFS (create if it doesn't exist)
  filename = String();
  if (fsUploadFile)
  {
    fsUploadFile.print(server.arg("plain")); // Write the received body to the file
    fsUploadFile.close();                    // Close the file again
    server.send(200, "text/plain", "handleFileStore() " + filename + " successful");
  }
  else
  {
    server.send(500, "text/plain", "500: couldn't create file " + filename);
  }
}