/*
  WiFiEnterprise MQTT Client Example
  
  This example demonstrates how to use the WiFiEnterprise library
  to connect to MQTT brokers for IoT applications over enterprise networks.
  
  Features:
  - MQTT publish/subscribe functionality
  - Automatic reconnection handling
  - JSON message formatting
  - Device status reporting
  - Remote command execution
  - Quality of Service (QoS) support
  - Last Will and Testament (LWT)
  - Secure MQTT (MQTTS) support
  - Message buffering for offline periods
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - Status LED on pin 2
  - Button on pin 0 (for manual triggers)
  - Optional sensors (DHT22, etc.)
  
  Libraries required:
  - PubSubClient
  - ArduinoJson
  
  Usage:
  1. Install required libraries
  2. Update network and MQTT broker credentials
  3. Upload sketch and open Serial Monitor
  4. Monitor MQTT messages and device status
  
  MQTT Topics:
  - device/{device_id}/status - Device status updates
  - device/{device_id}/data - Sensor data
  - device/{device_id}/command - Remote commands
  - device/{device_id}/response - Command responses
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <DHT.h>

// Network credentials
const char* ssid = "YourEnterpriseNetwork";
const char* username = "your_username";
const char* password = "your_password";

// MQTT Configuration
const char* mqtt_server = "your-mqtt-broker.com";
const int mqtt_port = 1883; // Use 8883 for MQTTS
const char* mqtt_username = "your_mqtt_username";
const char* mqtt_password = "your_mqtt_password";
const char* device_id = "esp32-enterprise-001";

// Pin definitions
#define LED_PIN 2
#define BUTTON_PIN 0
#define DHT_PIN 4
#define DHT_TYPE DHT22

// MQTT Topics
String topic_status = "device/" + String(device_id) + "/status";
String topic_data = "device/" + String(device_id) + "/data";
String topic_command = "device/" + String(device_id) + "/command";
String topic_response = "device/" + String(device_id) + "/response";
String topic_lwt = "device/" + String(device_id) + "/lwt";

// Objects
WiFiClient wifiClient;
// WiFiClientSecure wifiClient; // Use for MQTTS
PubSubClient mqttClient(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);
Preferences preferences;

// Global variables
struct DeviceStatus {
  bool connected = false;
  unsigned long uptime = 0;
  int freeHeap = 0;
  int wifiRssi = 0;
  float temperature = 0.0;
  float humidity = 0.0;
  bool buttonPressed = false;
  String lastCommand = "";
  String firmware_version = "1.0.0";
};

DeviceStatus deviceStatus;
struct MessageBuffer {
  String topic;
  String payload;
  unsigned long timestamp;
};

std::vector<MessageBuffer> offlineMessages;
const int MAX_OFFLINE_MESSAGES = 50;

unsigned long lastStatusUpdate = 0;
unsigned long lastDataUpdate = 0;
unsigned long lastMqttReconnect = 0;
unsigned long lastButtonCheck = 0;
const unsigned long STATUS_INTERVAL = 30000; // 30 seconds
const unsigned long DATA_INTERVAL = 60000;   // 60 seconds
const unsigned long MQTT_RECONNECT_INTERVAL = 5000; // 5 seconds

bool buttonState = false;
bool lastButtonState = false;
int messageCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  
  printHeader();
  
  // Initialize preferences
  preferences.begin("mqtt-client", false);
  messageCount = preferences.getInt("msg_count", 0);
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("🌡️  DHT22 sensor initialized");
  
  // Connect to enterprise network
  connectToNetwork();
  
  // Setup MQTT
  setupMQTT();
  
  Serial.println("🚀 MQTT client ready!");
  Serial.println("📡 Device ID: " + String(device_id));
  Serial.println("🏠 MQTT Broker: " + String(mqtt_server) + ":" + String(mqtt_port));
  Serial.println();
  
  // Send initial status
  publishStatus("online");
}

void loop() {
  // Handle MQTT
  if (!mqttClient.connected()) {
    reconnectMQTT();
  } else {
    mqttClient.loop();
  }
  
  // Check WiFi connection
  if (!WiFiEnterprise.isConnected()) {
    handleWiFiDisconnection();
    return;
  }
  
  // Update device status
  updateDeviceStatus();
  
  // Check button
  checkButton();
  
  // Publish status updates
  if (millis() - lastStatusUpdate >= STATUS_INTERVAL) {
    publishStatus("online");
    lastStatusUpdate = millis();
  }
  
  // Publish sensor data
  if (millis() - lastDataUpdate >= DATA_INTERVAL) {
    publishSensorData();
    lastDataUpdate = millis();
  }
  
  // Process offline messages if connected
  if (mqttClient.connected() && !offlineMessages.empty()) {
    processOfflineMessages();
  }
  
  // Status LED (slow blink = connected, fast = disconnected)
  static unsigned long lastBlink = 0;
  int blinkRate = mqttClient.connected() ? 1000 : 200;
  if (millis() - lastBlink >= blinkRate) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
  
  delay(100);
}

void printHeader() {
  Serial.println("\n" + String('=', 60));
  Serial.println("       WiFiEnterprise MQTT Client");
  Serial.println("" + String('=', 60));
  Serial.println("Device ID: " + String(device_id));
  Serial.println("ESP32 Chip: " + String(ESP.getChipModel()));
  Serial.println("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("MAC Address: " + WiFi.macAddress());
  Serial.println("Firmware: " + deviceStatus.firmware_version);
  Serial.println("" + String('=', 60) + "\n");
}

void connectToNetwork() {
  Serial.println("🔗 Connecting to WPA2-Enterprise network...");
  
  if (WiFiEnterprise.begin(ssid, username, password, true)) {
    Serial.println("\n✅ WiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFiEnterprise.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
  } else {
    Serial.println("\n❌ WiFi connection failed!");
    Serial.println("Please check credentials and restart.");
    while(true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
  }
}

void handleWiFiDisconnection() {
  static unsigned long lastReconnectAttempt = 0;
  
  // Fast blink to indicate disconnection
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(100);
  
  if (millis() - lastReconnectAttempt >= 30000) {
    Serial.println("🔄 WiFi disconnected, attempting to reconnect...");
    
    if (WiFiEnterprise.begin(ssid, username, password)) {
      Serial.println("✅ WiFi reconnected successfully!");
    } else {
      lastReconnectAttempt = millis();
    }
  }
}

void setupMQTT() {
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(30);
  
  // For MQTTS (uncomment if using secure connection)
  // wifiClient.setInsecure(); // Skip certificate verification
  // wifiClient.setCACert(ca_cert); // Set CA certificate
  
  Serial.println("📡 MQTT client configured");
  Serial.println("   Server: " + String(mqtt_server) + ":" + String(mqtt_port));
  Serial.println("   Device ID: " + String(device_id));
}

void reconnectMQTT() {
  if (millis() - lastMqttReconnect < MQTT_RECONNECT_INTERVAL) {
    return;
  }
  
  lastMqttReconnect = millis();
  
  if (!WiFiEnterprise.isConnected()) {
    return;
  }
  
  Serial.print("🔄 Attempting MQTT connection...");
  
  // Create Last Will and Testament
  String lwtPayload = createLWTPayload();
  
  // Attempt to connect
  if (mqttClient.connect(device_id, mqtt_username, mqtt_password, 
                        topic_lwt.c_str(), 1, true, lwtPayload.c_str())) {
    Serial.println(" ✅ Connected!");
    
    // Subscribe to command topic
    mqttClient.subscribe(topic_command.c_str(), 1);
    Serial.println("📥 Subscribed to: " + topic_command);
    
    // Publish online status
    publishStatus("online");
    
  } else {
    Serial.print(" ❌ Failed, rc=");
    Serial.println(mqttClient.state());
    
    switch (mqttClient.state()) {
      case -4: Serial.println("   Connection timeout"); break;
      case -3: Serial.println("   Connection lost"); break;
      case -2: Serial.println("   Connect failed"); break;
      case -1: Serial.println("   Disconnected"); break;
      case 1: Serial.println("   Bad protocol version"); break;
      case 2: Serial.println("   Bad client ID"); break;
      case 3: Serial.println("   Unavailable"); break;
      case 4: Serial.println("   Bad credentials"); break;
      case 5: Serial.println("   Unauthorized"); break;
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("📨 MQTT message received:");
  Serial.println("   Topic: " + String(topic));
  Serial.println("   Payload: " + message);
  
  // Process command
  if (String(topic) == topic_command) {
    processCommand(message);
  }
}

void processCommand(String command) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, command);
  
  if (error) {
    Serial.println("❌ Invalid JSON command: " + String(error.c_str()));
    publishResponse("error", "Invalid JSON format");
    return;
  }
  
  String cmd = doc["command"];
  deviceStatus.lastCommand = cmd;
  
  Serial.println("⚡ Processing command: " + cmd);
  
  if (cmd == "status") {
    publishStatus("online");
    publishResponse("success", "Status sent");
    
  } else if (cmd == "data") {
    publishSensorData();
    publishResponse("success", "Sensor data sent");
    
  } else if (cmd == "reboot") {
    publishResponse("success", "Rebooting device");
    delay(1000);
    ESP.restart();
    
  } else if (cmd == "led_on") {
    digitalWrite(LED_PIN, HIGH);
    publishResponse("success", "LED turned on");
    
  } else if (cmd == "led_off") {
    digitalWrite(LED_PIN, LOW);
    publishResponse("success", "LED turned off");
    
  } else if (cmd == "get_info") {
    publishDeviceInfo();
    publishResponse("success", "Device info sent");
    
  } else if (cmd == "ping") {
    publishResponse("success", "pong");
    
  } else {
    Serial.println("❓ Unknown command: " + cmd);
    publishResponse("error", "Unknown command: " + cmd);
  }
}

void updateDeviceStatus() {
  deviceStatus.connected = mqttClient.connected();
  deviceStatus.uptime = millis() / 1000;
  deviceStatus.freeHeap = ESP.getFreeHeap();
  deviceStatus.wifiRssi = WiFi.RSSI();
  
  // Read sensors
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (!isnan(temp)) deviceStatus.temperature = temp;
  if (!isnan(hum)) deviceStatus.humidity = hum;
}

void checkButton() {
  if (millis() - lastButtonCheck >= 50) { // Debounce
    buttonState = !digitalRead(BUTTON_PIN);
    
    if (buttonState && !lastButtonState) {
      Serial.println("🔘 Button pressed - sending manual data update");
      deviceStatus.buttonPressed = true;
      publishSensorData();
      publishResponse("info", "Manual data update triggered");
    }
    
    lastButtonState = buttonState;
    lastButtonCheck = millis();
  }
}

void publishStatus(String status) {
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["status"] = status;
  doc["timestamp"] = millis();
  doc["uptime"] = deviceStatus.uptime;
  doc["free_heap"] = deviceStatus.freeHeap;
  doc["wifi_rssi"] = deviceStatus.wifiRssi;
  doc["ip_address"] = WiFiEnterprise.localIP().toString();
  doc["firmware_version"] = deviceStatus.firmware_version;
  doc["last_command"] = deviceStatus.lastCommand;
  
  String payload;
  serializeJson(doc, payload);
  
  if (publishMessage(topic_status, payload, true)) {
    Serial.println("📤 Status published: " + status);
  }
}

void publishSensorData() {
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["timestamp"] = millis();
  doc["temperature"] = deviceStatus.temperature;
  doc["humidity"] = deviceStatus.humidity;
  doc["button_pressed"] = deviceStatus.buttonPressed;
  doc["message_count"] = ++messageCount;
  
  // Reset button flag
  deviceStatus.buttonPressed = false;
  
  String payload;
  serializeJson(doc, payload);
  
  if (publishMessage(topic_data, payload, false)) {
    Serial.println("📤 Sensor data published (msg #" + String(messageCount) + ")");
    Serial.println("   Temperature: " + String(deviceStatus.temperature, 1) + "°C");
    Serial.println("   Humidity: " + String(deviceStatus.humidity, 1) + "%");
    
    // Save message count
    preferences.putInt("msg_count", messageCount);
  }
}

void publishDeviceInfo() {
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["chip_model"] = ESP.getChipModel();
  doc["cpu_freq"] = ESP.getCpuFreqMHz();
  doc["flash_size"] = ESP.getFlashChipSize();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["mac_address"] = WiFi.macAddress();
  doc["firmware_version"] = deviceStatus.firmware_version;
  doc["sdk_version"] = ESP.getSdkVersion();
  doc["uptime"] = deviceStatus.uptime;
  doc["wifi_ssid"] = WiFi.SSID();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["ip_address"] = WiFiEnterprise.localIP().toString();
  
  String payload;
  serializeJson(doc, payload);
  
  String infoTopic = "device/" + String(device_id) + "/info";
  if (publishMessage(infoTopic, payload, false)) {
    Serial.println("📤 Device info published");
  }
}

void publishResponse(String status, String message) {
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["timestamp"] = millis();
  doc["status"] = status;
  doc["message"] = message;
  doc["command"] = deviceStatus.lastCommand;
  
  String payload;
  serializeJson(doc, payload);
  
  if (publishMessage(topic_response, payload, false)) {
    Serial.println("📤 Response sent: " + status + " - " + message);
  }
}

bool publishMessage(String topic, String payload, bool retained) {
  if (mqttClient.connected()) {
    bool result = mqttClient.publish(topic.c_str(), payload.c_str(), retained);
    if (!result) {
      Serial.println("❌ Failed to publish to: " + topic);
    }
    return result;
  } else {
    // Store message for later if offline
    if (offlineMessages.size() < MAX_OFFLINE_MESSAGES) {
      MessageBuffer msg;
      msg.topic = topic;
      msg.payload = payload;
      msg.timestamp = millis();
      offlineMessages.push_back(msg);
      Serial.println("💾 Message queued for offline delivery: " + topic);
    } else {
      Serial.println("⚠️  Offline message buffer full, dropping message");
    }
    return false;
  }
}

void processOfflineMessages() {
  if (offlineMessages.empty()) return;
  
  Serial.println("📤 Processing " + String(offlineMessages.size()) + " offline messages...");
  
  auto it = offlineMessages.begin();
  while (it != offlineMessages.end()) {
    if (mqttClient.publish(it->topic.c_str(), it->payload.c_str())) {
      Serial.println("✅ Offline message sent: " + it->topic);
      it = offlineMessages.erase(it);
    } else {
      Serial.println("❌ Failed to send offline message: " + it->topic);
      break; // Stop trying if one fails
    }
    delay(100); // Small delay between messages
  }
  
  if (offlineMessages.empty()) {
    Serial.println("✅ All offline messages processed");
  }
}

String createLWTPayload() {
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["status"] = "offline";
  doc["timestamp"] = millis();
  doc["reason"] = "unexpected_disconnect";
  
  String payload;
  serializeJson(doc, payload);
  return payload;
}