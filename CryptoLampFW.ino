#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <stdio.h>
#include <ArduinoJson.h>

/* Set these to your desired credentials. */
const char *APSsid = "ethermoodlight";
const char *kUUID = "breandantestlamp";
String *uuid; 
String ssid;
String password;
bool configureMode = false;
//ESP8266WiFiMulti WiFiMulti;
int green = 512;
int red = 512;
int blue = 512;
String payload;
float change = 0;
String *checkURL;
String updateURL;

ESP8266WebServer server(80);

void handleConfig()
{
  if (server.hasArg("ssid") == true && server.hasArg("password") == true)
  {
    ssid  = server.arg("ssid");
    password = server.arg("password");
    Serial.println(server.arg("ssid"));
    Serial.println(server.arg("password"));

    File passwordFile = SPIFFS.open("/password.txt", "w");
    File ssidFile = SPIFFS.open("/ssid.txt", "w");
    passwordFile.print(password.c_str());
    passwordFile.print("\n");
    ssidFile.print(ssid.c_str());
    ssidFile.print("\n");
    ssidFile.close();
    passwordFile.close();
    server.send(200, "text/html", "SSID and Password set, device will reset in several seconds.");
    delay(1000);
    while (1 == 1);
  }
  else
  {
    server.send(200, "text/html", "SSID or Password missing, <a href=\"/\">click here</a> to try again.");

  }
}
void handleRoot() {
  //server.send(200, "text/html", "<h1>You are connected</h1>");
  if (SPIFFS.exists("/setup.html")) {
    File file = SPIFFS.open("/setup.html", "r");                 // Open it
    size_t sent = server.streamFile(file, "text/html"); // And send it to the client
    file.close();
  }
  else
  {
    Serial.println("file not found");
  }




}
void setupAP()
{
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(APSsid);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.begin();
  Serial.println("HTTP server started");
  SPIFFS.begin();
  configureMode = true;
}
void setupClient()
{
  Serial.println("Entering client mode");
  SPIFFS.begin();
  File passwordFile = SPIFFS.open("/password.txt", "r");
  File ssidFile = SPIFFS.open("/ssid.txt", "r");
  password = passwordFile.readStringUntil('\n');
  ssid = ssidFile.readStringUntil('\n');
  passwordFile.close();
  ssidFile.close();
  Serial.print("ssid: ");
  Serial.println(ssid.c_str());
  Serial.print("password: ");
  Serial.println(password.c_str());

  WiFi.begin(ssid.c_str(), password.c_str() );

  /* while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());*/

  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  digitalWrite(D7, HIGH);
  Serial.print("UUID is: ");
  Serial.println(kUUID);
  getURL();
}
void getURL()
{
  bool keepTrying = true;

  while(keepTrying)
  {
  digitalWrite(D5, HIGH);
  digitalWrite(D6, LOW);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    digitalWrite(D5, !digitalRead(D5));
    digitalWrite(D6, !digitalRead(D6));
    }
    
  HTTPClient http;

    Serial.print("[HTTP] begin...\n");

    String myURL = *checkURL + *uuid;
    
    http.begin(myURL.c_str()); //HTTP

    //  Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        updateURL = http.getString();
        Serial.println(updateURL);
        keepTrying = false;
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      delay(1000);
      digitalWrite(D5, !digitalRead(D5));
      digitalWrite(D6, !digitalRead(D6));
    }
    
    
    http.end();
  }
}
void setup() {

  checkURL = new String("http://foc-electronics.com/cryptolamps/geturl.php?UUID=");
  uuid = new String(kUUID);
  
  pinMode(D0, INPUT);
  digitalWrite(D0, HIGH);

  delay(1000);
  Serial.begin(115200);
  if (!digitalRead(D0))
  {
    setupAP();
  }
  else
  {
    setupClient();
  }
}


void loop() {
  if (configureMode)
    server.handleClient();
  else
  {
    clientLoop();
  }
}

void clientLoop()
{

  if ((WiFi.status() == WL_CONNECTED)) {

    HTTPClient http;

    //Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    //http.begin("https://foc-electronics.com:443/webservices/ethchange.php"); //HTTPS
    http.begin(updateURL.c_str()); //HTTP

    //  Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        Serial.println(payload);
      }

    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    //parse the json here

    const size_t bufferSize = JSON_OBJECT_SIZE(3) + 40 + 20;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  Serial.println(payload);
  const char* json = payload.c_str();

  JsonObject& root = jsonBuffer.parseObject(json);

  red = root["red"]; // 1023
  green = root["green"]; // 1023
  blue = root["blue"]; // 1023

    //done parsing

    Serial.println("Parsed some json, this look good?");
    Serial.println(green);
    Serial.println(red);
    Serial.println(blue);
    http.end();
    analogWrite(D5, 1023-red);
    analogWrite(D6, 1023-green);
    analogWrite(D7, 1023-blue);
    delay(30000);
  }
  else
  {
    Serial.println("can't seem to connect to wifi");
    Serial.println(green);
    Serial.println(red);
    Serial.println(blue);
    if (red == 0)
    {
      red = 1023;
      green = 0;
    }
    else
    {
      red = 0;
      green = 1023;
    }
    delay(1000);
  }
  analogWrite(D5, 1023-red);
  analogWrite(D6, 1023-green);
  analogWrite(D7, 1023-blue);

  //delay(1000);

}
