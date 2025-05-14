#include "arduino_secrets.h"
#include "DHT.h"
#include "HTTPClient.h"
#include "thingProperties.h"

const int trig = 2;
const int echo = 4;
#define motor 5
#define dhtpin 18
#define DHTTYPE DHT22
#define fan 19
#define ledpin 13
#define pirPin 21
#define flamePin 22
#define buzzer 23
#define ledpir 14


float distance;
float duration;

unsigned long lastTempCheck = 0;
unsigned long lastWaterCheck = 0;
unsigned long lastFireCheck = 0;
unsigned long lastMotionCheck = 0;
unsigned long motionDetectedAt = 0;
unsigned long lastFireDetectTime = 0;

const unsigned long tempInterval = 2000;
const unsigned long waterInterval = 1500;
const unsigned long fireInterval = 1000;
const unsigned long motionInterval = 100;
const unsigned long motionDuration = 1000;
const unsigned long fireDelay = 15000;

bool fireDetected = false;
bool fireStateSent = false;
bool fireClearedSent = false;

// Button debounce
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

const char* server = "https://app.notify.lk/api/v1/send?";
const String apiKey = "Zz3YDV7ECdBnAL7ah3fg";
const String userID = "28999";
const String mobileNumber = "94750611344";

DHT dht(dhtpin, DHTTYPE);

unsigned long lastMotionTime = 0;
const unsigned long motionDelay = 1000;

void sendNotification(String message);

void setup() {
  Serial.begin(9600);
  delay(1500);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(motor, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(ledpin, OUTPUT);
  pinMode(pirPin, INPUT);
  pinMode(flamePin, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(ledpir, OUTPUT);

  WiFi.begin();

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  dht.begin();
}

void loop() {
  ArduinoCloud.update();
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastTempCheck >= tempInterval) {
    lastTempCheck = currentMillis;
    checkTemperature();
  }

  if (currentMillis - lastWaterCheck >= waterInterval) {
    lastWaterCheck = currentMillis;
    checkWaterLevel();
  }

  if (currentMillis - lastMotionCheck >= motionInterval) {
    lastMotionCheck = currentMillis;
    checkMotion();
  }

  if (currentMillis - lastFireCheck >= fireInterval) {
    lastFireCheck = currentMillis;
    checkFire();
  }
  


}

void checkTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Failed to read temperature");
    return;
  }

  temp = t;
  Serial.println(t);

  if (t > 30) {
    digitalWrite(fan, HIGH);
  } else if (t < 29) {
    digitalWrite(fan, LOW);
  }
}

void checkWaterLevel() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  float duration = pulseIn(echo, HIGH, 10000);
  float distance = 0.034 * duration / 2;

  waterLevel = distance;
  Serial.println(distance);

  if (distance > 20) {
    Serial.println("low water in tank!");
    digitalWrite(motor, HIGH);
  } else if (distance < 10) {
    Serial.println("tank full!");
    digitalWrite(motor, LOW);
  }
}

void checkMotion() {
  int pirState = digitalRead(pirPin);

  if (pirState == HIGH) {
    if (millis() - lastMotionTime > motionDelay) {
      if (!motion) {
        Serial.println("Motion detected");
      }
      motion = true;
      digitalWrite(ledpir, HIGH);
      motionDetectedAt = millis();
      lastMotionTime = millis();
    }
  } else {
    if (motion && millis() - motionDetectedAt <= motionDuration) {
      digitalWrite(ledpir, HIGH);
    } else {
      if (motion) {
        Serial.println("Motion ended");
        motion = false;
      }
      digitalWrite(ledpir, LOW);
    }
  }
}

void checkFire() {
  int fireState = digitalRead(flamePin);

  if (fireState == LOW && !fireDetected && !fireStateSent) {
    if (millis() - lastFireDetectTime > fireDelay) {
      Serial.println("Fire Detected! Alarm ON!");
      firealarm = true;
      digitalWrite(buzzer, HIGH);
      fireDetected = true;
      fireStateSent = true;
      fireClearedSent = false;
      sendNotification("Fire detected!");
      lastFireDetectTime = millis();
    }
  } else if (fireState == HIGH && fireDetected && !fireClearedSent) {
    Serial.println("No Fire, Alarm OFF!");
    firealarm = false;
    digitalWrite(buzzer, LOW);
    fireDetected = false;
    fireStateSent = false;
    fireClearedSent = true;
    sendNotification("No fire detected!");
  }
}


  
void sendNotification(String message) {
  HTTPClient http;
  http.begin(server);

  String postData = "user_id=" + userID + "&api_key=" + apiKey +
                    "&sender_id=NotifyDEMO&to=" + mobileNumber + "&message=" + message;

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST(postData);

  Serial.println(httpResponseCode > 0 ? "Message sent!" : "Error: " + String(httpResponseCode));
  http.end();
}

// Cloud callback functions
void onWaterLevelChange() {}

void onTempChange() {}

void onAlarmChange() {}

void onLedChange() {
  digitalWrite(led, ledpin ? HIGH : LOW);
}

void onMotionChange() {}

void onFirealarmChange() {}
