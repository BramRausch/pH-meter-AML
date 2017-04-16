#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define API_key "Bearer <API key>"

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

int buttonState;
int lastButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

const char* ssid     = "<SSID>";
const char* password = "<Passphrase>";

String url = "<Machine learning URL>";

void AzureMLPost(String payload){
  HTTPClient http;
  http.begin(url, "FC E4 6C E7 4F DA AF 0F B5 03 66 3A 8D 6D 3F 21 1A 40 82 6E"); // SHA1 HTTPS fingerprint
  http.addHeader("Authorization", API_key);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(payload.length()));

  int httpCode = http.POST(payload);
  
  if(httpCode == HTTP_CODE_OK){
    Serial.print("HTTP response code: ");
    Serial.println(httpCode);
    String response = http.getString();

    StaticJsonBuffer<400> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(response);
    float ph = root["Results"]["output1"]["value"]["Values"][0][6];
    Serial.print("Ph: "); Serial.println(ph);
  }else {
    Serial.println("Error in HTTP request");
  }
  http.end();
}

String ReadSensorToJSON(){
  uint16_t r, g, b, c, colorTemp, lux;
  
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);
  
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& Inputs = root.createNestedObject("Inputs");
  JsonObject& input1 = Inputs.createNestedObject("input1");
  JsonArray&  ColumnNames  = input1.createNestedArray("ColumnNames");
  ColumnNames.add("Color Temp");
  ColumnNames.add("Lux");
  ColumnNames.add("R");
  ColumnNames.add("G");
  ColumnNames.add("B");
  ColumnNames.add("C");
  JsonArray&  Values  = input1.createNestedArray("Values");
  JsonArray&  in1  = Values.createNestedArray();
  in1.add(colorTemp);
  in1.add(lux);
  in1.add(r);
  in1.add(g);
  in1.add(b);
  in1.add(c);
  JsonObject& GlobalParameters = root.createNestedObject("GlobalParameters");

  char buffer[256];
  root.printTo(buffer, sizeof(buffer));

  return buffer;
}

void setup(void) {
  Serial.begin(9600);
  tcs.begin();
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  } 
  Serial.println("Connected");  
    
  pinMode(D4, INPUT_PULLUP);
}

void loop(void) {
  int reading = digitalRead(D4);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        AzureMLPost(ReadSensorToJSON());        
      }
    }
  }
  
  lastButtonState = reading;
}
