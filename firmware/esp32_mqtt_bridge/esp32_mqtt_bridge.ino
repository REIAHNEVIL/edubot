#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>  // Include the ArduinoJson library

// Define your Wi-Fi and MQTT settings
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqttServer = "YOUR_MQTT_BROKER_IP";
const int mqttPort = 1883;  // Default MQTT port, change if needed

// Define serial communication pins for ESP32 if using hardware UART
#define RX_PIN 16  // RX pin of ESP32 connected to TX pin of Arduino
#define TX_PIN 17  // TX pin of ESP32 connected to RX pin of Arduino

WiFiClient espClient;
PubSubClient client(espClient);

// Timer variables to track the phone being removed
unsigned long lastUpdateTime = 0;  // Tracks last valid data timestamp
unsigned long lastCounterUpdateTime = 0;  // Tracks the last counter increment
const unsigned long timeoutInterval = 5000;  // 5 seconds timeout
const unsigned long counterUpdateInterval = 1000;  // Update counter every 1 second

// Last known pomodoro state
int lastMinutes = 0;
int lastSeconds = 0;
int lastCounter = 0;
bool phoneRemoved = false;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("Raw MQTT message: " + message);

  // Parse the received JSON message
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  if (!doc.containsKey("minutes") || !doc.containsKey("seconds") || !doc.containsKey("counter")) {
    Serial.println("Missing keys in JSON");
    return;
  }

  // Extract timer values
  int minutes = doc["minutes"];
  int seconds = doc["seconds"];
  int counter = doc["counter"];

  Serial.printf("Parsed JSON: minutes=%d, seconds=%d, counter=%d\n", minutes, seconds, counter);

  // Update last known values
  lastMinutes = minutes;
  lastSeconds = seconds;
  lastCounter = counter;

  // Reset timeout
  phoneRemoved = false;
  lastUpdateTime = millis();
  lastCounterUpdateTime = millis();
}

void setup() {
  Serial.begin(115200);  // Debug serial connection
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);  // Serial1 for Arduino communication

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    if (client.connect("ESP32_Client")) {
      Serial.println("Connected to MQTT broker");
      client.subscribe("pomodoro/data");
    } else {
      Serial.println("Failed to connect to MQTT. Retrying...");
      delay(5000);
    }
  }

  // Initialize last update time
  lastUpdateTime = millis();
  lastCounterUpdateTime = millis();
}

void loop() {
  client.loop();

  // Check for data from Arduino
  if (Serial1.available()) {
    String payload = Serial1.readStringUntil('\n');
    Serial.println("Received from Arduino: " + payload);  // Debug incoming payload

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON parse error from Arduino: ");
      Serial.println(error.c_str());
      return;
    }

    if (!doc.containsKey("minutes") || !doc.containsKey("seconds") || !doc.containsKey("counter")) {
      Serial.println("Arduino JSON missing keys");
      return;
    }

    // Extract data
    int minutes = doc["minutes"];
    int seconds = doc["seconds"];
    int counter = doc["counter"];

    // Publish data to MQTT
    if (client.connected()) {
      client.publish("pomodoro/data", payload.c_str());
      Serial.println("Published to MQTT: " + payload);

      // Update last known values
      lastMinutes = minutes;
      lastSeconds = seconds;
      lastCounter = counter;

      // Reset timeout
      phoneRemoved = false;
      lastUpdateTime = millis();
      lastCounterUpdateTime = millis();
    } else {
      Serial.println("MQTT client not connected. Could not publish.");
    }
  }

  // Increment the counter periodically like seconds/minutes
  if (millis() - lastCounterUpdateTime >= counterUpdateInterval) {
    lastCounter++;
    lastCounterUpdateTime = millis();

    // Print updated counter
    // Serial.printf("Time Distracted: %d seconds\n", lastCounter);
  }
  // Check for phone removal or timeout
  if (millis() - lastUpdateTime > timeoutInterval) {
    if (!phoneRemoved) {
      Serial.println("Phone removed or no updates from Arduino.");
      phoneRemoved = true;

      // Publish a "PAUSED" state to indicate the phone is removed
      DynamicJsonDocument doc(2048);
      doc["status"] = "PAUSED";
      doc["minutes"] = lastMinutes;
      doc["seconds"] = lastSeconds;
      doc["counter"] = lastCounter;

      String pausedPayload;
      serializeJson(doc, pausedPayload);

      if (client.connected()) {
        client.publish("pomodoro/status", pausedPayload.c_str());
        Serial.println("Published PAUSED state to MQTT.");
      }
    }
  }
}
