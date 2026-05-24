#include <Arduino.h>
#include <WiFi.h>
#include <ESP32_FTPClient.h>

#include "ChatGPTClient.h"
#include "config.h"

ChatGPTClient gpt(OPENAI_API_KEY);
ESP32_FTPClient ftp(FTP_HOST, FTP_USER, FTP_PASSWORD, FTP_PORT, 2);

unsigned long lastDecisionMs = 0;
unsigned long lastRequirementsUploadMs = 0;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(" connected");
}

int readDistanceCm() {
  // TODO: Replace with your real distance sensor read.
  return random(10, 150);
}

void motorForward() { Serial.println("ACTION: forward"); }
void motorLeft() { Serial.println("ACTION: left"); }
void motorRight() { Serial.println("ACTION: right"); }
void motorStop() { Serial.println("ACTION: stop"); }

void applyAction(const String& action) {
  if (action == "forward") {
    motorForward();
  } else if (action == "left") {
    motorLeft();
  } else if (action == "right") {
    motorRight();
  } else {
    motorStop();
  }
}

String robotStateJson() {
  int distance = readDistanceCm();
  bool wifi = WiFi.status() == WL_CONNECTED;

  String s = "{";
  s += "\\\"robot_id\\\":\\\"" + String(ROBOT_ID) + "\\\",";
  s += "\\\"distance_cm\\\":" + String(distance) + ",";
  s += "\\\"wifi\\\":" + String(wifi ? "true" : "false") + ",";
  s += "\\\"budget_aud\\\":" + String(UPGRADE_BUDGET_AUD, 2);
  s += "}";
  return s;
}

bool uploadRequirementsToFtp(const String& requirementsText) {
  if (WiFi.status() != WL_CONNECTED) return false;

  ftp.OpenConnection();
  ftp.ChangeWorkDir(FTP_BASE_PATH);

  String filename = String(ROBOT_ID) + "_requirements.txt";
  ftp.InitFile("Type A");
  ftp.NewFile(filename.c_str());
  ftp.Write((unsigned char*)requirementsText.c_str(), requirementsText.length());
  ftp.CloseFile();
  ftp.CloseConnection();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  connectWifi();
  randomSeed(micros());
  Serial.println("Explorer ESP32-S3 robot booted.");
}

void loop() {
  const unsigned long now = millis();

  if (now - lastDecisionMs >= DECISION_INTERVAL_MS) {
    lastDecisionMs = now;

    String state = robotStateJson();
    Serial.println("STATE: " + state);

    String action = gpt.suggestAction(state, OPENAI_MODEL, OPENAI_ENDPOINT);
    Serial.println("GPT SUGGESTED ACTION: " + action);

    // Local safety guard: never move forward if too close.
    if (readDistanceCm() < 20 && action == "forward") action = "stop";

    applyAction(action);
  }

  if (now - lastRequirementsUploadMs >= REQUIREMENTS_UPLOAD_MS) {
    lastRequirementsUploadMs = now;

    String state = robotStateJson();
    String requirements = gpt.suggestRequirements(state, UPGRADE_BUDGET_AUD,
                                                  OPENAI_MODEL, OPENAI_ENDPOINT);

    Serial.println("UPGRADE PLAN (<=30 AUD):\n" + requirements);

    if (uploadRequirementsToFtp(requirements)) {
      Serial.println("Requirements uploaded to FTP.");
    } else {
      Serial.println("FTP upload skipped/failed.");
    }
  }

  delay(50);
}
