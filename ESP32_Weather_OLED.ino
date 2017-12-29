#include <WiFi.h>
#include <ArduinoJson.h>
#include "SSD1306.h"

const char* ssid     = "";
const char* password = "";

const char* host = "api.openweathermap.org";
// City ID
const char *id = "3526617";
// API Key
const char* privateKey = "";

IPAddress local_IP(192, 168, 0, 249);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

SSD1306  display(0x3c, 5, 4);

void setup(){
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void loop(){
  delay(10000);
  display.clear();
  
  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // Create a URI for the request
  String url = "/data/2.5/weather?id=";
  url += id;
  url += "&lang=es&units=metric&APPID=";
  url += privateKey;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
               
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  parseJson(client);
}

void parseJson(WiFiClient jsonString){
  const size_t bufferSize =
      JSON_ARRAY_SIZE(1)
      + JSON_OBJECT_SIZE(1)
      + 2*JSON_OBJECT_SIZE(2)
      + 2*JSON_OBJECT_SIZE(4)
      + JSON_OBJECT_SIZE(7)
      + JSON_OBJECT_SIZE(11) //raiz
      + 430;
      
  //Reserva memoria
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  if (!root.success()){
    Serial.println("ParseObject() failed");
    return;
  }

  JsonObject& main = root["main"];
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, root["name"].as<char*>());

  JsonObject& weather = root["weather"][0];
  String weather_description = weather["description"];
  byte temperatura = (byte)main["temp"];
  byte humedad = main["humidity"];

  display.drawString(0, 10, weather_description);
  display.drawString(0, 20, "Temperatura: " + String(temperatura) + " C");
  display.drawString(0, 30, "Humedad: " + String(humedad) + " %");
  display.display();
}

