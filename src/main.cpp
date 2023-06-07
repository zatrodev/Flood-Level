/*
  Rui Santos
  Complete project details at our blog: https://RandomNerdTutorials.com/esp8266-data-logging-firebase-realtime-database/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"


#define WIFI_SSID "Galaxy A31FAB2"
#define WIFI_PASSWORD "Blake3.0"

#define API_KEY "AIzaSyC2nDYTS_CxsI3L2_DZmdy8XvE7uFTU1Ms"
#define USER_EMAIL "furio.renz.0189@gmail.com"
#define USER_PASSWORD "Riecoe200412"
#define DATABASE_URL "https://flood-level-database-default-rtdb.asia-southeast1.firebasedatabase.app/Z"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;

String parentPath;

String floodLevelPath = "/floodLevel";
String S_A = "/levelA";
String S_B = "/levelB";
String S_C = "/levelC";
String S_D = "/levelD";
String S_E = "/levelE";

String timePath = "/timestamp";

int timestamp;

FirebaseJson json;

//Delay before sending new data
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 500;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


//WEB API KEY: AIzaSyC2nDYTS_CxsI3L2_DZmdy8XvE7uFTU1Ms
//ADDRESS: https://flood-level-database-default-rtdb.asia-southeast1.firebasedatabase.app/Z

typedef const uint8_t pint;

//Switches are assigned to pins D1, D2, D5, D6, and D7 (GPIO5, GPIO4, GPIO14, GPIO12, and GPIO13 respectively)
pint SWITCH_A = 5;
pint SWITCH_B = 4; 
pint SWITCH_C = 14;
pint SWITCH_D = 12;
pint SWITCH_E = 13;

pint SIREN_V = 0;
pint SIREN_G = 2;

pint SWITCH_PIN[5] = {SWITCH_A, SWITCH_B, SWITCH_C, SWITCH_D, SWITCH_E};
bool IS_TRIGGERED[5];

uint G_LEVEL;
uint PREV_LEVEL = 6;

String uintToString(uint INT)
{
  String intS[6] = {"0", "1", "2", "3", "4", "5"};
  return intS[INT];
}

uint FLOOD_LEVEL()
{
  uint LEVEL = 0;
  for (int i = 0; i < 5; i++)
  {
    IS_TRIGGERED[i] = (digitalRead(SWITCH_PIN[i]) ? true : false);
  }
  for (int i = 0; i < 5; i++)
  {
    LEVEL += (IS_TRIGGERED[i] ? 1 : 0);
  }
  G_LEVEL = LEVEL;
  // fireSiren(); 
  return LEVEL;
}
/*
bool CHANGE_LEVEL()
{
  bool CHANGE_DETECTED = (PREV_LEVEL != G_LEVEL? true : false);
  return CHANGE_DETECTED;
}*/

void fireSiren()
{
  if (G_LEVEL >= 2)
  {
    digitalWrite(SIREN_V, LOW);

    delay(10000);

    digitalWrite(SIREN_V, HIGH);
  }
  else
  {
    digitalWrite(SIREN_V, HIGH);
  }
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(WiFi.localIP());
  Serial.println();
}

unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void setup() {
  Serial.begin(115200);

  initWiFi();
  
  for (int i = 0; i < 5; i++)
  {
    pinMode(SWITCH_PIN[i], OUTPUT);
  }

  pinMode(SIREN_V, INPUT);
  pinMode(SIREN_V, INPUT);

  digitalWrite(SIREN_G, HIGH);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task 
  config.token_status_callback = tokenStatusCallback;

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/";
}

void loop() {
  //Initializes the function every start of the loop 
  FLOOD_LEVEL();
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    //Get current timestamp
    timestamp = getTime();
    Serial.print ("Levels Flipped: ");
    Serial.println (G_LEVEL);

    parentPath = databasePath;

    json.set(floodLevelPath.c_str(), uintToString(G_LEVEL));

    json.set(S_A.c_str(), uintToString(digitalRead(SWITCH_A)));
    json.set(S_B.c_str(), uintToString(digitalRead(SWITCH_B)));
    json.set(S_C.c_str(), uintToString(digitalRead(SWITCH_C)));
    json.set(S_D.c_str(), uintToString(digitalRead(SWITCH_D)));
    json.set(S_E.c_str(), uintToString(digitalRead(SWITCH_E)));

    json.set(timePath, String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "OK" : fbdo.errorReason().c_str());
    PREV_LEVEL = G_LEVEL; 
  }
}