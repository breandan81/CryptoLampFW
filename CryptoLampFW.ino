/*
   Based on example arduino ESP8266 Access point, copyright follows:
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.
   ?)(O
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Modifications copyright 2018 Breandan O'Shaughnessy All rights reserved

  This code allows connection as an access point to set wifi SSID and password settings for future connections


*/

/* Create a WiFi access point and provide a web server on it. */

/*todo
   boot in two modes depending on whether a button was held, button will also be used to enter OTA update mode
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <stdio.h>

/* Set these to your desired credentials. */
const char *APSsid = "ethermoodlight";
const char *APPassword = "thereisnospoon";
String ssid;
String password;
bool configureMode = false;
//ESP8266WiFiMulti WiFiMulti;
int green = 512;
int red = 512;
String payload;
float change = 0;


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
}
void setup() {
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

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    //http.begin("https://foc-electronics.com:443/webservices/ethchange.php"); //HTTPS
    http.begin("http://foc-electronics.com/webservices/ethchange.php"); //HTTP

    //  Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        Serial.println(payload);
      }

    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    change = payload.toFloat() / 10;
    Serial.println(change);
    if (change < -1)
    {
      red = 0;
      green = 1023;
    }
    else if (change > 1)
    {
      red = 1023;
      green = 0;
    }
    else
    {
      float result = 1023.0 * ((change + 1.0) / 2);
      red = result;
      green = 1023 - result;
    }
    Serial.println(green);

    Serial.println(red);
    delay(1000);
  }
  else
  {
    Serial.println("can't seem to connect to wifi");
    Serial.println(green);
    Serial.println(red);
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
  analogWrite(D6, red);
  analogWrite(D5, green);

  //delay(1000);

}
