#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.n.h"

// UCL eduroam: enter your own credentials locally before uploading.
// Do not commit credentials to a public Git repository.
const char* ssid = WIFI_SSID;
const char* identity = WIFI_IDENTITY;
const char* username = WIFI_USERNAME;
const char* password = WIFI_PASSWORD;

// UCL broker listed in Connectivity slides, p.19.
// Port 1883 is an initial test assumption; confirm UCL broker settings.
const char* broker = "engf0001.cs.ucl.ac.uk";
const uint16_t brokerPort = 1883;

// Preserve existing two-way topics. Use group-specific topics.
const char* subscribeTopic = "uni-project-23827/arduino";
const char* publishTopic = "uni-project-23827/python";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastSend = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received on ");
  Serial.print(topic);
  Serial.print(": ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.write(payload[i]);
  }
  Serial.println();
  // TODO: Validate a setpoint JSON message, then relay it to Nano via UART.
}

void startWiFi() {
  Serial.println("Connecting to eduroam...");
  WiFi.mode(WIFI_STA);
  // This is the ESP32 Arduino-core WPA2-Enterprise PEAP API.
  // For production, validate the university RADIUS server certificate.
  WiFi.begin(ssid, WPA2_AUTH_PEAP, identity, username, password);
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  Serial.print("Connecting to UCL MQTT broker...");
  if (mqtt.connect("uni-esp32-23827")) {
    Serial.println(" connected!");
    if (!mqtt.subscribe(subscribeTopic)) {
      Serial.println("MQTT subscription failed");
    }
  } else {
    Serial.print(" failed, MQTT state: ");
    Serial.println(mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  mqtt.setServer(broker, brokerPort);
  mqtt.setCallback(callback);
  mqtt.setKeepAlive(30);
  startWiFi();
  lastWiFiAttempt = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiAttempt >= 10000) {
      lastWiFiAttempt = millis();
      Serial.println("Wi-Fi disconnected; retrying...");
      WiFi.disconnect();
      startWiFi();
    }
    delay(10);
    return;
  }

  static bool reportedWiFi = false;
  if (!reportedWiFi) {
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
    reportedWiFi = true;
  }

  if (!mqtt.connected()) {
    if (lastMQTTAttempt == 0 || millis() - lastMQTTAttempt >= 5000) {
      lastMQTTAttempt = millis();
      connectMQTT();
    }
    delay(10);
    return;
  }

  mqtt.loop();

  // Proposal slide 8: one MQTT message per second.
  // Explicitly tagged test data: NOT actual sensor readings.
  if (millis() - lastSend >= 1000) {
    lastSend = millis();
    const char* testPayload = "{\"type\":\"test\",\"source\":\"esp32-devkitc\",\"message\":\"Hello from ESP32\"}";
    if (mqtt.publish(publishTopic, testPayload)) {
      Serial.println("Test JSON sent to Python");
    } else {
      Serial.println("Publish failed");
    }
  }
}
