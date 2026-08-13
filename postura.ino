#include <WiFi.h>
#include "ThingSpeak.h"
#include <Wire.h>

// Wi-Fi credentials 
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ThingSpeak configuration
unsigned long channelID = YOUR_CHANNEL_ID;
const char* writeAPIKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

#define BUZZER_PIN 2
WiFiClient client;

const float GOOD_POSTURE_MIN = 60.0;
const float GOOD_POSTURE_MAX = 87.0;

bool isSlouching = false;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 15000;

unsigned long lastBuzzerBeep = 0;
const unsigned long BUZZER_INTERVAL = 1500;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected");

  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  Serial.println("MPU6050 Ready");

  ThingSpeak.begin(client);
}

void loop() {
  float pitch = getPitch();
  bool goodPosture =
      (pitch >= GOOD_POSTURE_MIN && pitch <= GOOD_POSTURE_MAX);

  if (!goodPosture) {
    if (millis() - lastBuzzerBeep > BUZZER_INTERVAL) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);

      Serial.println("BAD POSTURE!");

      lastBuzzerBeep = millis();
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);

    if (isSlouching) {
      Serial.println("Posture corrected!");
      isSlouching = false;
    }
  }

  isSlouching = !goodPosture;

  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print("° | Status: ");
  Serial.println(
      goodPosture ? "GOOD (60-87°)" : "BAD (<60°) - BUZZING"
  );

  if (millis() - lastUpdate > UPDATE_INTERVAL) {
    ThingSpeak.setField(1, pitch);
    ThingSpeak.setField(2, goodPosture ? 0 : 1);

    int x = ThingSpeak.writeFields(channelID, writeAPIKey);

    Serial.println(
        x == 200 ? "ThingSpeak OK" : "ThingSpeak FAIL"
    );

    lastUpdate = millis();
  }

  delay(3000);
}

float getPitch() {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  Wire.requestFrom(0x68, 6);

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();

  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;

  return atan2(
      accelX,
      sqrt(accelY * accelY + accelZ * accelZ)
  ) * 180 / PI;
}
