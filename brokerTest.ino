#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.n.h"

const char* BROKER = "engf0001.cs.ucl.ac.uk";
const uint16_t PORT = 1883;
const char* TOPIC_TELEMETRY = "group31/bioreactor/telemetry";
const char* TOPIC_SETPOINTS = "group31/bioreactor/setpoints/set";

WiFiClient wifi;
PubSubClient mqtt(wifi);

float spTemp = 30.0f;
float spPh = 5.0f;
float spRpm = 1000.0f;
float temp = 27.0f;
float ph = 5.4f;
float rpm = 600.0f;
unsigned long lastPublish = 0;
unsigned long lastWifiTry = 0;
unsigned long lastMqttTry = 0;

bool allowed(JsonVariantConst value, float low, float high) {
  if (!value.is<float>() && !value.is<double>() && !value.is<int>() && !value.is<long>()) return false;
  float n = value.as<float>();
  return isfinite(n) && n >= low && n <= high;
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_SETPOINTS) != 0) return;

  JsonDocument data;
  if (deserializeJson(data, payload, length) || !data.is<JsonObject>()) {
    Serial.println("Invalid setpoints JSON");
    return;
  }

  JsonObjectConst obj = data.as<JsonObjectConst>();
  float nextTemp = spTemp, nextPh = spPh, nextRpm = spRpm;
  bool changed = false;
  for (JsonPairConst pair : obj) {
    const char* key = pair.key().c_str();
    JsonVariantConst value = pair.value();
    if (strcmp(key, "temp") == 0 && allowed(value, 25, 35)) {
      nextTemp = value.as<float>();
      changed = true;
    } else if (strcmp(key, "ph") == 0 && allowed(value, 3, 7)) {
      nextPh = value.as<float>();
      changed = true;
    } else if (strcmp(key, "rpm") == 0 && allowed(value, 500, 1500)) {
      nextRpm = value.as<float>();
      changed = true;
    } else {
      Serial.println("Setpoint request rejected");
      return;
    }
  }

  if (changed) {
    spTemp = nextTemp;
    spPh = nextPh;
    spRpm = nextRpm;
    Serial.printf("Setpoints: %.1f C, pH %.2f, %.0f rpm\n", spTemp, spPh, spRpm);
    lastPublish = millis() - 1000;
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WPA2_AUTH_PEAP, WIFI_IDENTITY, WIFI_USERNAME, WIFI_PASSWORD);
}

void connectMqtt() {
  String id = "group31_nano_" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqtt.connect(id.c_str())) {
    mqtt.subscribe(TOPIC_SETPOINTS, 1);
    Serial.println("MQTT connected");
  } else {
    Serial.printf("MQTT error: %d\n", mqtt.state());
  }
}

void publishTelemetry() {
  JsonDocument data;
  data["temp"] = roundf(temp * 100) / 100.0f;
  data["ph"] = roundf(ph * 100) / 100.0f;
  data["rpm"] = (int)roundf(rpm);
  data["heater"] = constrain((spTemp - temp) * 0.12f, 0.0f, 0.6f);
  data["motor"] = constrain(rpm / 1500.0f, 0.0f, 1.0f);
  data["acid"] = ph > spPh + 0.1f ? 0.5f : 0.0f;
  data["base"] = ph < spPh - 0.1f ? 0.5f : 0.0f;
  data["sp_temp"] = spTemp;
  data["sp_ph"] = spPh;
  data["sp_rpm"] = (int)roundf(spRpm);

  char buffer[384];
  size_t n = serializeJson(data, buffer, sizeof(buffer));
  if (n > 0 && n < sizeof(buffer)) {
    mqtt.publish(TOPIC_TELEMETRY, (const uint8_t*)buffer, n, false);
    Serial.println(buffer);
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);
  mqtt.setServer(BROKER, PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);
  connectWiFi();
  lastWifiTry = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiTry >= 10000) {
      lastWifiTry = millis();
      WiFi.disconnect();
      connectWiFi();
    }
    delay(10);
    return;
  }

  if (!mqtt.connected()) {
    if (lastMqttTry == 0 || millis() - lastMqttTry >= 5000) {
      lastMqttTry = millis();
      connectMqtt();
    }
    delay(10);
    return;
  }

  mqtt.loop();
  if (millis() - lastPublish >= 1000) {
    lastPublish = millis();
    temp += constrain((spTemp - temp) * 0.05f, -0.3f, 0.3f);
    ph += constrain((spPh - ph) * 0.05f, -0.05f, 0.05f);
    rpm += constrain((spRpm - rpm) * 0.05f, -40.0f, 40.0f);
    publishTelemetry();
  }
}
