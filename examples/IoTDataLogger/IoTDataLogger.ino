/*
  WiFiEnterprise IoT Data Logger Example
  
  This example demonstrates how to collect sensor data and send it to
  cloud services (ThingSpeak, MQTT broker) using the WiFiEnterprise library
  for WPA2-Enterprise network connectivity.
  
  Features:
  - Multiple sensor data collection
  - Cloud data transmission (ThingSpeak)
  - MQTT publishing
  - Local data buffering
  - Automatic reconnection
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - DHT22 sensor on pin 4 (optional)
  - LDR (light sensor) on pin A0 (optional)
  - Status LED on pin 2
  
  Required Libraries:
  - PubSubClient (for MQTT)
  - ArduinoJson
  - DHT sensor library (if using real sensors)
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Network credentials
const char* ssid = "YourEnterpriseNetwork";
const char* username = "your_username";
const char* password = "your_password";

// ThingSpeak configuration
const char* thingSpeakServer = "api.thingspeak.com";
const String thingSpeakAPIKey = "YOUR_THINGSPEAK_API_KEY";
const int thingSpeakChannelID = 123456; // Your channel ID

// MQTT configuration
const char* mqttServer = "your-mqtt-broker.com";
const int mqttPort = 1883;
const char* mqttUser = "your_mqtt_user";
const char* mqttPassword = "your_mqtt_password";
const char* mqttClientID = "ESP32_Enterprise_Logger";
const char* mqttTopic = "sensors/esp32/data";

// Pin definitions
const int LED_PIN = 2;
const int DHT_PIN = 4;
const int LDR_PIN = A0;

// Timing configuration
const unsigned long SENSOR_INTERVAL = 30000;    // Read sensors every 30 seconds
const unsigned long UPLOAD_INTERVAL = 300000;   // Upload to cloud every 5 minutes
const unsigned long MQTT_INTERVAL = 60000;      // Send MQTT every 1 minute

// Data structure for sensor readings
struct SensorData {
  float temperature;
  float humidity;
  int lightLevel;
  unsigned long timestamp;
};

// Global variables
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
HTTPClient httpClient;

SensorData currentReading;
SensorData dataBuffer[10]; // Buffer for storing readings
int bufferIndex = 0;
bool bufferFull = false;

unsigned long lastSensorRead = 0;
unsigned long lastCloudUpload = 0;
unsigned long lastMQTTSend = 0;
unsigned long lastReconnectAttempt = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("WiFiEnterprise IoT Data Logger");
  Serial.println("=============================");
  
  // Connect to enterprise network
  connectToNetwork();
  
  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  
  Serial.println("🚀 IoT Data Logger started!");
  Serial.println("📊 Sensor reading interval: " + String(SENSOR_INTERVAL/1000) + " seconds");
  Serial.println("☁️  Cloud upload interval: " + String(UPLOAD_INTERVAL/1000) + " seconds");
  Serial.println("📡 MQTT publish interval: " + String(MQTT_INTERVAL/1000) + " seconds");
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check WiFi connection
  if (!WiFiEnterprise.isConnected()) {
    handleDisconnection();
    return;
  }
  
  // Read sensors
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    storeReading();
    lastSensorRead = currentTime;
  }
  
  // Upload to cloud (ThingSpeak)
  if (currentTime - lastCloudUpload >= UPLOAD_INTERVAL) {
    uploadToThingSpeak();
    lastCloudUpload = currentTime;
  }
  
  // Send MQTT data
  if (currentTime - lastMQTTSend >= MQTT_INTERVAL) {
    sendMQTTData();
    lastMQTTSend = currentTime;
  }
  
  // Handle MQTT client
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Status LED (blink when active)
  digitalWrite(LED_PIN, (millis() / 1000) % 2);
  
  delay(1000);
}

void connectToNetwork() {
  Serial.println("🔗 Connecting to WPA2-Enterprise network...");
  
  if (WiFiEnterprise.begin(ssid, username, password, true)) {
    Serial.println("\n✅ Connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFiEnterprise.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n❌ Connection failed!");
    Serial.println("Please check your credentials and restart.");
    while(true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
  }
}

void handleDisconnection() {
  Serial.println("⚠️ WiFi connection lost!");
  
  // Fast blink LED to indicate disconnection
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(200);
  
  // Attempt reconnection every 30 seconds
  if (millis() - lastReconnectAttempt >= 30000) {
    Serial.println("🔄 Attempting to reconnect...");
    
    if (WiFiEnterprise.begin(ssid, username, password)) {
      Serial.println("✅ Reconnected successfully!");
      lastReconnectAttempt = 0;
    } else {
      Serial.println("❌ Reconnection failed, will retry in 30 seconds");
      lastReconnectAttempt = millis();
    }
  }
}

void readSensors() {
  // Read temperature and humidity (simulated - replace with real sensor code)
  currentReading.temperature = 20.0 + random(-50, 150) / 10.0; // 15-35°C
  currentReading.humidity = 40.0 + random(0, 400) / 10.0;      // 40-80%
  
  // Read light level
  currentReading.lightLevel = analogRead(LDR_PIN);
  
  // Store timestamp
  currentReading.timestamp = millis();
  
  // For real DHT22 sensor, use:
  // currentReading.temperature = dht.readTemperature();
  // currentReading.humidity = dht.readHumidity();
  
  Serial.println("📊 Sensor Reading:");
  Serial.println("   Temperature: " + String(currentReading.temperature, 1) + "°C");
  Serial.println("   Humidity: " + String(currentReading.humidity, 1) + "%");
  Serial.println("   Light Level: " + String(currentReading.lightLevel));
  Serial.println();
}

void storeReading() {
  // Store reading in circular buffer
  dataBuffer[bufferIndex] = currentReading;
  bufferIndex = (bufferIndex + 1) % 10;
  
  if (bufferIndex == 0) {
    bufferFull = true;
  }
  
  Serial.println("💾 Data stored in buffer (" + String(bufferFull ? 10 : bufferIndex) + "/10 readings)");
}

void uploadToThingSpeak() {
  if (!WiFiEnterprise.isConnected()) {
    Serial.println("⚠️ Cannot upload to ThingSpeak - no WiFi connection");
    return;
  }
  
  Serial.println("☁️ Uploading to ThingSpeak...");
  
  // Prepare ThingSpeak URL
  String url = "http://" + String(thingSpeakServer) + "/update?api_key=" + thingSpeakAPIKey;
  url += "&field1=" + String(currentReading.temperature, 1);
  url += "&field2=" + String(currentReading.humidity, 1);
  url += "&field3=" + String(currentReading.lightLevel);
  
  httpClient.begin(url);
  int httpResponseCode = httpClient.GET();
  
  if (httpResponseCode > 0) {
    String response = httpClient.getString();
    Serial.println("✅ ThingSpeak upload successful! Entry ID: " + response);
  } else {
    Serial.println("❌ ThingSpeak upload failed! Error: " + String(httpResponseCode));
  }
  
  httpClient.end();
}

void sendMQTTData() {
  if (!mqttClient.connected()) {
    Serial.println("⚠️ Cannot send MQTT data - not connected to broker");
    return;
  }
  
  Serial.println("📡 Sending MQTT data...");
  
  // Create JSON payload
  JsonDocument doc;
  doc["device_id"] = mqttClientID;
  doc["timestamp"] = currentReading.timestamp;
  doc["temperature"] = currentReading.temperature;
  doc["humidity"] = currentReading.humidity;
  doc["light_level"] = currentReading.lightLevel;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["free_heap"] = ESP.getFreeHeap();
  
  String payload;
  serializeJson(doc, payload);
  
  if (mqttClient.publish(mqttTopic, payload.c_str())) {
    Serial.println("✅ MQTT data sent successfully!");
    Serial.println("   Topic: " + String(mqttTopic));
    Serial.println("   Payload: " + payload);
  } else {
    Serial.println("❌ MQTT publish failed!");
  }
}

void reconnectMQTT() {
  if (!WiFiEnterprise.isConnected()) {
    return; // Don't try MQTT if WiFi is down
  }
  
  static unsigned long lastMQTTReconnect = 0;
  
  // Try to reconnect every 5 seconds
  if (millis() - lastMQTTReconnect >= 5000) {
    Serial.println("🔄 Attempting MQTT connection...");
    
    if (mqttClient.connect(mqttClientID, mqttUser, mqttPassword)) {
      Serial.println("✅ MQTT connected!");
      
      // Subscribe to control topics
      mqttClient.subscribe("sensors/esp32/control");
      
      // Send online status
      JsonDocument statusDoc;
      statusDoc["device_id"] = mqttClientID;
      statusDoc["status"] = "online";
      statusDoc["ip"] = WiFiEnterprise.localIP().toString();
      
      String statusPayload;
      serializeJson(statusDoc, statusPayload);
      mqttClient.publish("sensors/esp32/status", statusPayload.c_str());
      
    } else {
      Serial.println("❌ MQTT connection failed, rc=" + String(mqttClient.state()));
    }
    
    lastMQTTReconnect = millis();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("📨 MQTT message received:");
  Serial.println("   Topic: " + String(topic));
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("   Message: " + message);
  
  // Parse JSON command
  JsonDocument doc;
  deserializeJson(doc, message);
  
  if (doc["command"] == "get_status") {
    sendStatusUpdate();
  } else if (doc["command"] == "set_interval") {
    // Handle interval change commands
    Serial.println("💡 Command received: " + String(doc["command"].as<String>()));
  }
}

void sendStatusUpdate() {
  JsonDocument doc;
  doc["device_id"] = mqttClientID;
  doc["uptime"] = millis();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["buffer_count"] = bufferFull ? 10 : bufferIndex;
  doc["last_reading"] = currentReading.timestamp;
  
  String payload;
  serializeJson(doc, payload);
  
  mqttClient.publish("sensors/esp32/status", payload.c_str());
  Serial.println("📊 Status update sent via MQTT");
}