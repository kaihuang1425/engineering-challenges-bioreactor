
#include <WiFi.h>
#include <PubSubClient.h>

// EDUROAM
const char* ssid = "eduroam";
const char* email = "";
const char* password = "";

// MQTT
const char* broker = "broker.hivemq.com";
const char* subscribeTopic = "uni-project-23827/arduino";
const char* publishTopic = "uni-project-23827/python";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastSend = 0;

// Receive messages from Python
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received from Python: ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
}

// Connect to eduroam
void connectWiFi() {
  Serial.println("Connecting to eduroam...");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid,
    WPA2_AUTH_PEAP,
    email,
    email,
    password
  );

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nWi-Fi connected!");
  Serial.println(WiFi.localIP());
}

// Connect to HiveMQ
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.println("Connecting to HiveMQ...");

    if (mqtt.connect("uni-esp32-23827")) {
      Serial.println("MQTT connected!");
      mqtt.subscribe(subscribeTopic);
    } else {
      Serial.print("MQTT failed. State: ");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  mqtt.setServer(broker, 1883);
  mqtt.setCallback(callback);

  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();

  // Send test message every 5 seconds
  if (millis() - lastSend >= 5000) {
    lastSend = millis();

    if (mqtt.publish(publishTopic, "Hello from Arduino!")) {
      Serial.println("Sent to Python!");
    } else {
      Serial.println("Publish failed!");
    }
  }
}
