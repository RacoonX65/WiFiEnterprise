/*
  WiFiEnterprise Simple Sensor Example
  
  This example shows how to read a simple sensor and display
  the data on a web page over an enterprise WiFi network.
  
  Perfect for learning basic IoT sensor monitoring.
  
  Features:
  - Temperature sensor reading (built-in ESP32 sensor)
  - Simple web display
  - Auto-refresh data
  - Serial output for debugging
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board only (uses built-in temperature sensor)
  - Optional: External temperature sensor on pin A0
  
  Usage:
  1. Update your network credentials below
  2. Upload to ESP32
  3. Open Serial Monitor to see readings and IP address
  4. Open web browser and go to the IP address
  5. Watch live sensor data
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <WebServer.h>

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

// ========================================
// CHANGE THESE TO YOUR NETWORK SETTINGS
// ========================================
const char* ssid = "YourEnterpriseNetwork";     // Your enterprise WiFi name
const char* username = "your_username";          // Your username
const char* password = "your_password";          // Your password

// Pin definitions
const int STATUS_LED = 2;     // Built-in LED for status
const int ANALOG_PIN = A0;    // Optional external sensor pin

// Create web server on port 80
WebServer server(80);

// Sensor data
float temperature = 0.0;
int analogValue = 0;
unsigned long lastReading = 0;
const unsigned long READING_INTERVAL = 2000; // Read every 2 seconds

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Setup pins
  pinMode(STATUS_LED, OUTPUT);
  pinMode(ANALOG_PIN, INPUT);
  digitalWrite(STATUS_LED, LOW);
  
  // Print startup message
  Serial.println();
  Serial.println("====================================");
  Serial.println("  WiFiEnterprise Simple Sensor");
  Serial.println("====================================");
  Serial.println();
  
  // Connect to WiFi
  Serial.println("Connecting to enterprise network...");
  Serial.println("Network: " + String(ssid));
  Serial.println();
  
  if (WiFiEnterprise.begin(ssid, username, password)) {
    Serial.println("✅ WiFi Connected!");
    Serial.println("IP Address: " + WiFiEnterprise.localIP().toString());
    Serial.println();
    
    // Turn on status LED
    digitalWrite(STATUS_LED, HIGH);
    
    // Setup web server
    setupWebServer();
    
    Serial.println("🌐 Web server started!");
    Serial.println("📊 Sensor monitoring active!");
    Serial.println("Open your browser and go to: http://" + WiFiEnterprise.localIP().toString());
    Serial.println();
    
    // Take initial reading
    readSensors();
    
  } else {
    Serial.println("❌ WiFi connection failed!");
    Serial.println("Please check your credentials and try again.");
    
    // Blink status LED to show error
    while(true) {
      digitalWrite(STATUS_LED, HIGH);
      delay(200);
      digitalWrite(STATUS_LED, LOW);
      delay(200);
    }
  }
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Read sensors periodically
  if (millis() - lastReading >= READING_INTERVAL) {
    readSensors();
    lastReading = millis();
  }
  
  // Check WiFi connection
  if (!WiFiEnterprise.isConnected()) {
    Serial.println("WiFi connection lost!");
    digitalWrite(STATUS_LED, LOW);
    
    // Try to reconnect
    if (WiFiEnterprise.begin(ssid, username, password)) {
      Serial.println("WiFi reconnected!");
      digitalWrite(STATUS_LED, HIGH);
    }
  }
  
  delay(10);
}

void readSensors() {
  // Read built-in temperature sensor (ESP32)
  temperature = (temprature_sens_read() - 32) / 1.8;
  
  // Read analog value (0-4095 on ESP32)
  analogValue = analogRead(ANALOG_PIN);
  
  // Print to serial
  Serial.println("📊 Sensor Reading:");
  Serial.println("   Temperature: " + String(temperature, 1) + "°C");
  Serial.println("   Analog Value: " + String(analogValue) + " (" + String((analogValue/4095.0)*100, 1) + "%)");
  Serial.println();
}

void setupWebServer() {
  // Main page
  server.on("/", handleRoot);
  
  // Data API endpoint
  server.on("/data", handleData);
  
  // Start server
  server.begin();
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>ESP32 Sensor Monitor</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; margin: 50px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; }";
  html += ".container { max-width: 500px; margin: 0 auto; background: rgba(255,255,255,0.1); padding: 30px; border-radius: 15px; backdrop-filter: blur(10px); box-shadow: 0 8px 32px rgba(0,0,0,0.3); }";
  html += "h1 { margin-bottom: 30px; font-size: 28px; }";
  html += ".sensor-card { background: rgba(255,255,255,0.2); margin: 20px 0; padding: 25px; border-radius: 12px; border: 1px solid rgba(255,255,255,0.3); }";
  html += ".sensor-title { font-size: 18px; margin-bottom: 10px; opacity: 0.9; }";
  html += ".sensor-value { font-size: 36px; font-weight: bold; margin: 10px 0; }";
  html += ".sensor-unit { font-size: 16px; opacity: 0.8; }";
  html += ".progress-bar { width: 100%; height: 20px; background: rgba(255,255,255,0.2); border-radius: 10px; overflow: hidden; margin: 15px 0; }";
  html += ".progress-fill { height: 100%; background: linear-gradient(90deg, #00ff88, #00ccff); transition: width 0.5s ease; }";
  html += ".info { margin-top: 30px; padding: 20px; background: rgba(255,255,255,0.1); border-radius: 10px; font-size: 14px; }";
  html += ".timestamp { font-size: 12px; opacity: 0.7; margin-top: 15px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>🌡️ ESP32 Sensor Monitor</h1>";
  
  // Temperature card
  html += "<div class='sensor-card'>";
  html += "<div class='sensor-title'>🌡️ Internal Temperature</div>";
  html += "<div class='sensor-value'>" + String(temperature, 1) + "</div>";
  html += "<div class='sensor-unit'>degrees Celsius</div>";
  
  // Temperature progress bar (assuming 0-50°C range)
  float tempPercent = constrain((temperature / 50.0) * 100, 0, 100);
  html += "<div class='progress-bar'>";
  html += "<div class='progress-fill' style='width: " + String(tempPercent) + "%;'></div>";
  html += "</div>";
  html += "</div>";
  
  // Analog sensor card
  html += "<div class='sensor-card'>";
  html += "<div class='sensor-title'>📊 Analog Sensor</div>";
  html += "<div class='sensor-value'>" + String(analogValue) + "</div>";
  html += "<div class='sensor-unit'>raw value (0-4095)</div>";
  
  // Analog progress bar
  float analogPercent = (analogValue / 4095.0) * 100;
  html += "<div class='progress-bar'>";
  html += "<div class='progress-fill' style='width: " + String(analogPercent) + "%;'></div>";
  html += "</div>";
  html += "<div class='sensor-unit'>" + String(analogPercent, 1) + "% of full scale</div>";
  html += "</div>";
  
  // Device info
  html += "<div class='info'>";
  html += "<strong>📡 Device Information</strong><br>";
  html += "IP Address: " + WiFiEnterprise.localIP().toString() + "<br>";
  html += "Uptime: " + formatUptime(millis()) + "<br>";
  html += "WiFi Signal: " + String(WiFi.RSSI()) + " dBm<br>";
  html += "Free Memory: " + String(ESP.getFreeHeap()) + " bytes";
  html += "</div>";
  
  html += "<div class='timestamp'>";
  html += "Last updated: " + String(millis()/1000) + " seconds ago<br>";
  html += "Auto-refresh every 5 seconds";
  html += "</div>";
  
  html += "</div>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"temperature\": " + String(temperature, 2) + ",";
  json += "\"analog_value\": " + String(analogValue) + ",";
  json += "\"analog_percent\": " + String((analogValue/4095.0)*100, 2) + ",";
  json += "\"uptime\": " + String(millis()/1000) + ",";
  json += "\"ip_address\": \"" + WiFiEnterprise.localIP().toString() + "\",";
  json += "\"wifi_rssi\": " + String(WiFi.RSSI()) + ",";
  json += "\"free_heap\": " + String(ESP.getFreeHeap()) + ",";
  json += "\"timestamp\": " + String(millis());
  json += "}";
  
  server.send(200, "application/json", json);
}

String formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  
  String uptime = "";
  if (days > 0) uptime += String(days) + "d ";
  if (hours > 0) uptime += String(hours) + "h ";
  if (minutes > 0) uptime += String(minutes) + "m ";
  uptime += String(seconds) + "s";
  
  return uptime;
}