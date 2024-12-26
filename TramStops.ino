#include <SoftwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <time.h>
#include <WebServer.h>
#include "config.h"  
WebServer server(80);

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8
// Defining pins for each matrix
#define CS_PIN_1 4  // CS pin for matrix 1
#define CS_PIN_2 16 // CS pin for matrix 2

MD_Parola Display1 = MD_Parola(HARDWARE_TYPE, CS_PIN_1, MAX_DEVICES);
MD_Parola Display2 = MD_Parola(HARDWARE_TYPE, CS_PIN_2, MAX_DEVICES);

// Wi-Fi and API details
const char* ssid = SSIDNAME;
const char* password = SSIDPASS;
const char* apiUrl = API_URL;
const char* apiKey = API_KEY;   
WiFiClientSecure client;  

// Display settings
uint8_t scrollSpeed = 45;   
textEffect_t scrollEffect  = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;

// Time details
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;  
String hslId1 = "HSL:1174410";
String hslId2 = "HSL:1174411";

// Variables to store stop info
String stopInfo1[3]; 
String stopInfo2[3]; 
int currentIndex1 = 0; 
int currentIndex2 = 0; 

void setup() {
  Serial.begin(115200);

  // Initializing both matrices
  Display1.begin();
  Display2.begin();
  Display1.setIntensity(10);  
  Display2.setIntensity(10);
  Display1.displayClear();
  Display2.displayClear();

  // Initializing Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Configure NTP
  configTime(gmtOffset_sec, 0, ntpServer);
  server.on("/setHslId", HTTP_POST, handleSetHslId);
  server.on("/tramStops", HTTP_OPTIONS, handleOptions);
  server.begin();
}

void loop() {
  server.handleClient();
   handleSetHslId();
  if (WiFi.status() == WL_CONNECTED) {
    fetchDataForMatrix1();  
    fetchDataForMatrix2();  
  }

  // Display the current tram info for both matrices
  displayOnMatrix(Display1, stopInfo1[currentIndex1]);
  displayOnMatrix(Display2, stopInfo2[currentIndex2]);

  currentIndex1 = (currentIndex1 + 1) % 3; 
  currentIndex2 = (currentIndex2 + 1) % 3; 
  delay(1000); 
}

void handleSetHslId() {
  HTTPClient http;
  client.setInsecure(); 

  String stopIdUrl = "https://hh3dlab.fi/tram/current_stop.txt";
  http.begin(client, stopIdUrl);
  int httpResponseCode = http.GET();

  //Figuring the ID for the opposite stop
  if (httpResponseCode > 0) {
    hslId1 = http.getString();  
    if(hslId1 != "HSL:1173433") {
        String prefix = hslId1.substring(0, hslId1.indexOf(':') + 1);  
        String idNumberStr = hslId1.substring(hslId1.indexOf(':') + 1);  
        int idNumber = idNumberStr.toInt();  
        int oppositeIdNumber= idNumber + 1;  
        hslId2 = prefix + String(oppositeIdNumber);  
    } else {
      hslId2 = "";
    }
    Serial.println("HSL ID1 updated to: " + hslId1);
    Serial.println("HSL ID2 updated to: " + hslId2);
  } else {
    Serial.println("Failed to retrieve stop ID. HTTP code: " + String(httpResponseCode));
  }
  http.end();  
}


void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

void fetchDataForMatrix1() {
  HTTPClient http1;
  client.setInsecure();
  String query = R"(
  {
   stop(id: ")" + hslId1 + R"(") {
      name
      stoptimesWithoutPatterns {
        scheduledArrival
        realtimeArrival
        headsign
        trip {
          route {
            shortName
          }
        }
      }
    }  
  }
  )";
  http1.setTimeout(3000);
  http1.begin(client, apiUrl);
  http1.addHeader("Content-Type", "application/graphql");
  http1.addHeader("digitransit-subscription-key", apiKey);
  int httpResponseCode = http1.POST(query);
  Serial.print("HTTP Response Code for Matrix 1: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http1.getString();
    Serial.println("Response from Matrix 1:");
    Serial.println(response);
    if (httpResponseCode == 200) {
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, response);
      if (!error) {
        JsonArray stoptimes = doc["data"]["stop"]["stoptimesWithoutPatterns"];
        int count = 0;
        for (JsonObject time : stoptimes) {
          if (count >= 3) break; 
          int realtimeArrival = time["realtimeArrival"];
          int minutesLeft = calculateMinutesLeft(realtimeArrival);
          if (minutesLeft < 0) continue; // Skip negative minutes
          String tramNo = time["trip"]["route"]["shortName"];
          String headsign = String(time["headsign"].as<const char*>());
          String headsignAfterSpace = headsign.substring(0, headsign.indexOf(' '));
          String schedule = tramNo + " " + headsignAfterSpace + " : " + String(minutesLeft) + " min";
          stopInfo1[count] = schedule;
          count++;
        }
      }
    }
  }
  http1.end();  
}


void fetchDataForMatrix2() {
  HTTPClient http2;
  client.setInsecure();
  if(hslId2 != "") {
    String query = R"(
    { 
      stop(id: ")" + hslId2 + R"(") {  
        name
        stoptimesWithoutPatterns {
          scheduledArrival
          realtimeArrival
          headsign
          trip {
            route {
              shortName
            }
          }
        }
      }  
    }
    )";
    http2.setTimeout(3000);
    http2.begin(client, apiUrl);
    http2.addHeader("Content-Type", "application/graphql");
    http2.addHeader("digitransit-subscription-key", apiKey);
    int httpResponseCode = http2.POST(query);
    Serial.print("HTTP Response Code for Matrix 2: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode > 0) {
      String response = http2.getString();
      Serial.println("Response from Matrix 2:");
      Serial.println(response);
      if (httpResponseCode == 200) {
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, response);
        if (!error) {
          JsonArray stoptimes = doc["data"]["stop"]["stoptimesWithoutPatterns"];
          int count = 0;
          for (JsonObject time : stoptimes) {
            if (count >= 3) break; 
            int realtimeArrival = time["realtimeArrival"];
            int minutesLeft = calculateMinutesLeft(realtimeArrival);
            if (minutesLeft < 0) continue; // Skip negative minutes
            String tramNo = time["trip"]["route"]["shortName"];
            String headsign = String(time["headsign"].as<const char*>());
            String headsignAfterSpace = headsign.substring(0, headsign.indexOf(' '));
            String schedule = tramNo + " " + headsignAfterSpace + " : " + String(minutesLeft) + " min";
            stopInfo2[count] = schedule; 
            count++;
          }
        }
      }
    }
    http2.end(); 
  } 
}

void displayOnMatrix(MD_Parola& display, String text) {
  display.displayText(text.c_str(), scrollAlign, scrollSpeed, 0, scrollEffect, scrollEffect);
  while (!display.displayAnimate()) {}  
  display.displayReset();
}

// Function to calculate the remaining minutes based on the current time and the API's realtimeArrival
int calculateMinutesLeft(int realtimeArrival) {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return -1;
  }
  int currentSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60;
  int realtimeArrivalHours = realtimeArrival / 3600;
  int realtimeArrivalMinutes = (realtimeArrival % 3600) / 60;
  int realtimeArrivalSeconds = realtimeArrivalHours * 3600 + realtimeArrivalMinutes * 60;
  int realfinalTime = (realtimeArrivalSeconds - currentSeconds) / 60;
  return realfinalTime; 
}
