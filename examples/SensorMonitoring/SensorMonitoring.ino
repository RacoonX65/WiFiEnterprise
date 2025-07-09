/*
  WiFiEnterprise Sensor Monitoring Example
  
  This example demonstrates how to create a comprehensive sensor monitoring
  system using the WiFiEnterprise library for WPA2-Enterprise connectivity.
  
  Features:
  - Multiple sensor support (DHT22, BMP280, LDR, PIR)
  - Real-time data collection
  - Web dashboard interface
  - Data logging to SD card
  - Alert system with thresholds
  - JSON API endpoints
  - Automatic data transmission
  - Low power mode support
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - DHT22 sensor on pin 4 (temperature/humidity)
  - BMP280 sensor on I2C (pressure/altitude)
  - LDR (light sensor) on pin 34
  - PIR motion sensor on pin 5
  - Status LED on pin 2
  - Buzzer on pin 18 (optional)
  - SD card module on SPI (optional)
  
  Libraries required:
  - DHT sensor library
  - Adafruit BMP280 library
  - ArduinoJson
  - SD library (if using SD card)
  
  Usage:
  1. Install required libraries
  2. Connect sensors according to circuit diagram
  3. Update network credentials
  4. Upload sketch and open Serial Monitor
  5. Access web dashboard at displayed IP address
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>

// Network credentials
const char* ssid = "YourEnterpriseNetwork";
const char* username = "your_username";
const char* password = "your_password";

// Pin definitions
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define LDR_PIN 34
#define PIR_PIN 5
#define LED_PIN 2
#define BUZZER_PIN 18
#define SD_CS_PIN 5

// Sensor objects
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;
WebServer server(80);
Preferences preferences;

// Sensor data structure
struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  int lightLevel;
  bool motionDetected;
  unsigned long timestamp;
  bool valid;
};

// Configuration structure
struct Config {
  float tempMin = 18.0;
  float tempMax = 28.0;
  float humidityMin = 30.0;
  float humidityMax = 70.0;
  float pressureMin = 950.0;
  float pressureMax = 1050.0;
  int lightMin = 100;
  int lightMax = 800;
  bool alertsEnabled = true;
  int readingInterval = 5000; // 5 seconds
  bool sdLogging = false;
  bool webDashboard = true;
};

// Global variables
SensorData currentReading;
SensorData lastReading;
Config config;
std::vector<SensorData> dataHistory;
const int MAX_HISTORY = 100;
unsigned long lastSensorRead = 0;
unsigned long lastWebUpdate = 0;
unsigned long lastAlert = 0;
bool sensorsInitialized = false;
String alertMessage = "";
int alertLevel = 0; // 0=none, 1=warning, 2=critical

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  printHeader();
  
  // Initialize preferences
  preferences.begin("sensor-monitor", false);
  loadConfiguration();
  
  // Initialize sensors
  initializeSensors();
  
  // Connect to enterprise network
  connectToNetwork();
  
  // Setup web server
  setupWebServer();
  
  Serial.println("🚀 Sensor monitoring system ready!");
  Serial.print("🌐 Web dashboard: http://");
  Serial.println(WiFiEnterprise.localIP());
  Serial.println("📊 Starting data collection...");
  Serial.println();
}

void loop() {
  // Handle web server
  server.handleClient();
  
  // Check connection status
  if (!WiFiEnterprise.isConnected()) {
    handleDisconnection();
    return;
  }
  
  // Read sensors at configured interval
  if (millis() - lastSensorRead >= config.readingInterval) {
    readSensors();
    checkAlerts();
    updateHistory();
    
    if (config.sdLogging) {
      logToSD();
    }
    
    lastSensorRead = millis();
  }
  
  // Handle alerts
  handleAlerts();
  
  // Status LED (slow blink = normal, fast = alert)
  static unsigned long lastBlink = 0;
  int blinkRate = (alertLevel > 0) ? 200 : 1000;
  if (millis() - lastBlink >= blinkRate) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
  
  delay(100);
}

void printHeader() {
  Serial.println("\n" + String('=', 60));
  Serial.println("      WiFiEnterprise Sensor Monitoring");
  Serial.println("" + String('=', 60));
  Serial.println("ESP32 Chip: " + String(ESP.getChipModel()));
  Serial.println("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
  Serial.println("" + String('=', 60) + "\n");
}

void loadConfiguration() {
  config.tempMin = preferences.getFloat("tempMin", config.tempMin);
  config.tempMax = preferences.getFloat("tempMax", config.tempMax);
  config.humidityMin = preferences.getFloat("humMin", config.humidityMin);
  config.humidityMax = preferences.getFloat("humMax", config.humidityMax);
  config.pressureMin = preferences.getFloat("presMin", config.pressureMin);
  config.pressureMax = preferences.getFloat("presMax", config.pressureMax);
  config.lightMin = preferences.getInt("lightMin", config.lightMin);
  config.lightMax = preferences.getInt("lightMax", config.lightMax);
  config.alertsEnabled = preferences.getBool("alerts", config.alertsEnabled);
  config.readingInterval = preferences.getInt("interval", config.readingInterval);
  config.sdLogging = preferences.getBool("sdLog", config.sdLogging);
  
  Serial.println("📋 Configuration loaded:");
  Serial.println("   Temperature: " + String(config.tempMin) + "°C - " + String(config.tempMax) + "°C");
  Serial.println("   Humidity: " + String(config.humidityMin) + "% - " + String(config.humidityMax) + "%");
  Serial.println("   Pressure: " + String(config.pressureMin) + " - " + String(config.pressureMax) + " hPa");
  Serial.println("   Light: " + String(config.lightMin) + " - " + String(config.lightMax));
  Serial.println("   Reading interval: " + String(config.readingInterval) + " ms");
  Serial.println("   Alerts: " + String(config.alertsEnabled ? "Enabled" : "Disabled"));
  Serial.println();
}

void initializeSensors() {
  Serial.println("🔧 Initializing sensors...");
  
  // Initialize DHT22
  dht.begin();
  Serial.println("   ✅ DHT22 (Temperature/Humidity) initialized");
  
  // Initialize BMP280
  if (bmp.begin(0x76)) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    Serial.println("   ✅ BMP280 (Pressure/Altitude) initialized");
  } else {
    Serial.println("   ⚠️  BMP280 not found, pressure readings disabled");
  }
  
  // Initialize SD card (optional)
  if (config.sdLogging) {
    if (SD.begin(SD_CS_PIN)) {
      Serial.println("   ✅ SD card initialized for logging");
      
      // Create header if file doesn't exist
      if (!SD.exists("/sensor_data.csv")) {
        File file = SD.open("/sensor_data.csv", FILE_WRITE);
        if (file) {
          file.println("Timestamp,Temperature,Humidity,Pressure,Altitude,Light,Motion");
          file.close();
        }
      }
    } else {
      Serial.println("   ⚠️  SD card initialization failed");
      config.sdLogging = false;
    }
  }
  
  Serial.println("   ✅ LDR (Light sensor) ready");
  Serial.println("   ✅ PIR (Motion sensor) ready");
  
  sensorsInitialized = true;
  Serial.println("✅ All sensors initialized\n");
}

void connectToNetwork() {
  Serial.println("🔗 Connecting to WPA2-Enterprise network...");
  
  if (WiFiEnterprise.begin(ssid, username, password, true)) {
    Serial.println("\n✅ Connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFiEnterprise.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
  } else {
    Serial.println("\n❌ Connection failed!");
    Serial.println("Please check credentials and restart.");
    while(true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
  }
}

void handleDisconnection() {
  static unsigned long lastReconnectAttempt = 0;
  
  // Fast blink to indicate disconnection
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(100);
  
  if (millis() - lastReconnectAttempt >= 30000) {
    Serial.println("🔄 WiFi disconnected, attempting to reconnect...");
    
    if (WiFiEnterprise.begin(ssid, username, password)) {
      Serial.println("✅ Reconnected successfully!");
    } else {
      lastReconnectAttempt = millis();
    }
  }
}

void readSensors() {
  if (!sensorsInitialized) return;
  
  SensorData reading;
  reading.timestamp = millis();
  reading.valid = true;
  
  // Read DHT22
  reading.temperature = dht.readTemperature();
  reading.humidity = dht.readHumidity();
  
  if (isnan(reading.temperature) || isnan(reading.humidity)) {
    Serial.println("⚠️  DHT22 reading failed");
    reading.temperature = lastReading.temperature;
    reading.humidity = lastReading.humidity;
  }
  
  // Read BMP280
  reading.pressure = bmp.readPressure() / 100.0F; // Convert to hPa
  reading.altitude = bmp.readAltitude(1013.25); // Sea level pressure
  
  if (isnan(reading.pressure)) {
    reading.pressure = lastReading.pressure;
    reading.altitude = lastReading.altitude;
  }
  
  // Read light sensor
  reading.lightLevel = analogRead(LDR_PIN);
  
  // Read motion sensor
  reading.motionDetected = digitalRead(PIR_PIN);
  
  // Store current reading
  lastReading = currentReading;
  currentReading = reading;
  
  // Print to serial
  printSensorData();
}

void printSensorData() {
  Serial.println("📊 Sensor Reading [" + String(millis()/1000) + "s]:");
  Serial.println("   🌡️  Temperature: " + String(currentReading.temperature, 1) + "°C");
  Serial.println("   💧 Humidity: " + String(currentReading.humidity, 1) + "%");
  Serial.println("   🌪️  Pressure: " + String(currentReading.pressure, 1) + " hPa");
  Serial.println("   🏔️  Altitude: " + String(currentReading.altitude, 1) + " m");
  Serial.println("   💡 Light: " + String(currentReading.lightLevel));
  Serial.println("   🚶 Motion: " + String(currentReading.motionDetected ? "Detected" : "None"));
  
  if (alertLevel > 0) {
    Serial.println("   ⚠️  Alert: " + alertMessage);
  }
  
  Serial.println();
}

void checkAlerts() {
  if (!config.alertsEnabled) {
    alertLevel = 0;
    alertMessage = "";
    return;
  }
  
  alertLevel = 0;
  alertMessage = "";
  
  // Temperature alerts
  if (currentReading.temperature < config.tempMin) {
    alertLevel = 2;
    alertMessage = "Temperature too low: " + String(currentReading.temperature, 1) + "°C";
  } else if (currentReading.temperature > config.tempMax) {
    alertLevel = 2;
    alertMessage = "Temperature too high: " + String(currentReading.temperature, 1) + "°C";
  }
  
  // Humidity alerts
  else if (currentReading.humidity < config.humidityMin) {
    alertLevel = 1;
    alertMessage = "Humidity too low: " + String(currentReading.humidity, 1) + "%";
  } else if (currentReading.humidity > config.humidityMax) {
    alertLevel = 1;
    alertMessage = "Humidity too high: " + String(currentReading.humidity, 1) + "%";
  }
  
  // Pressure alerts
  else if (currentReading.pressure < config.pressureMin) {
    alertLevel = 1;
    alertMessage = "Pressure too low: " + String(currentReading.pressure, 1) + " hPa";
  } else if (currentReading.pressure > config.pressureMax) {
    alertLevel = 1;
    alertMessage = "Pressure too high: " + String(currentReading.pressure, 1) + " hPa";
  }
  
  // Light alerts
  else if (currentReading.lightLevel < config.lightMin) {
    alertLevel = 1;
    alertMessage = "Light level too low: " + String(currentReading.lightLevel);
  } else if (currentReading.lightLevel > config.lightMax) {
    alertLevel = 1;
    alertMessage = "Light level too high: " + String(currentReading.lightLevel);
  }
}

void handleAlerts() {
  if (alertLevel == 0) return;
  
  // Sound buzzer for critical alerts
  if (alertLevel == 2 && millis() - lastAlert >= 5000) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
    }
    lastAlert = millis();
  }
}

void updateHistory() {
  dataHistory.push_back(currentReading);
  
  // Keep only last MAX_HISTORY readings
  if (dataHistory.size() > MAX_HISTORY) {
    dataHistory.erase(dataHistory.begin());
  }
}

void logToSD() {
  if (!config.sdLogging) return;
  
  File file = SD.open("/sensor_data.csv", FILE_APPEND);
  if (file) {
    file.print(currentReading.timestamp);
    file.print(",");
    file.print(currentReading.temperature, 2);
    file.print(",");
    file.print(currentReading.humidity, 2);
    file.print(",");
    file.print(currentReading.pressure, 2);
    file.print(",");
    file.print(currentReading.altitude, 2);
    file.print(",");
    file.print(currentReading.lightLevel);
    file.print(",");
    file.println(currentReading.motionDetected ? 1 : 0);
    file.close();
  }
}

void setupWebServer() {
  // Main dashboard
  server.on("/", handleDashboard);
  
  // API endpoints
  server.on("/api/current", handleCurrentData);
  server.on("/api/history", handleHistoryData);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSetConfig);
  server.on("/api/alerts", handleAlerts);
  
  // Static files
  server.on("/style.css", handleCSS);
  
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("🌐 Web server started");
}

void handleDashboard() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Sensor Monitor Dashboard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>🌡️ Sensor Monitoring Dashboard</h1>
            <p>Real-time environmental monitoring</p>
        </header>
        
        <div class="alert-banner" id="alertBanner" style="display: none;">
            <span id="alertText"></span>
        </div>
        
        <div class="sensor-grid">
            <div class="sensor-card">
                <h3>🌡️ Temperature</h3>
                <div class="value" id="temperature">--°C</div>
                <div class="range">Range: )" + String(config.tempMin) + R"(°C - )" + String(config.tempMax) + R"(°C</div>
            </div>
            
            <div class="sensor-card">
                <h3>💧 Humidity</h3>
                <div class="value" id="humidity">--%</div>
                <div class="range">Range: )" + String(config.humidityMin) + R"(% - )" + String(config.humidityMax) + R"(%</div>
            </div>
            
            <div class="sensor-card">
                <h3>🌪️ Pressure</h3>
                <div class="value" id="pressure">-- hPa</div>
                <div class="range">Range: )" + String(config.pressureMin) + R"( - )" + String(config.pressureMax) + R"( hPa</div>
            </div>
            
            <div class="sensor-card">
                <h3>🏔️ Altitude</h3>
                <div class="value" id="altitude">-- m</div>
                <div class="range">Sea level reference</div>
            </div>
            
            <div class="sensor-card">
                <h3>💡 Light Level</h3>
                <div class="value" id="light">--</div>
                <div class="range">Range: )" + String(config.lightMin) + R"( - )" + String(config.lightMax) + R"(</div>
            </div>
            
            <div class="sensor-card">
                <h3>🚶 Motion</h3>
                <div class="value" id="motion">--</div>
                <div class="range">PIR sensor</div>
            </div>
        </div>
        
        <div class="controls">
            <button onclick="toggleAlerts()" id="alertBtn">🔔 Alerts: ON</button>
            <button onclick="downloadData()">📥 Download Data</button>
            <button onclick="showConfig()">⚙️ Settings</button>
        </div>
        
        <div class="chart-container">
            <h3>📊 Recent History</h3>
            <canvas id="chart" width="800" height="300"></canvas>
        </div>
    </div>
    
    <script>
        let alertsEnabled = true;
        
        function updateData() {
            fetch('/api/current')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temperature.toFixed(1) + '°C';
                    document.getElementById('humidity').textContent = data.humidity.toFixed(1) + '%';
                    document.getElementById('pressure').textContent = data.pressure.toFixed(1) + ' hPa';
                    document.getElementById('altitude').textContent = data.altitude.toFixed(1) + ' m';
                    document.getElementById('light').textContent = data.light_level;
                    document.getElementById('motion').textContent = data.motion ? 'Detected' : 'None';
                    
                    // Update alert banner
                    if (data.alert_level > 0) {
                        document.getElementById('alertBanner').style.display = 'block';
                        document.getElementById('alertText').textContent = data.alert_message;
                        document.getElementById('alertBanner').className = 'alert-banner ' + 
                            (data.alert_level === 2 ? 'critical' : 'warning');
                    } else {
                        document.getElementById('alertBanner').style.display = 'none';
                    }
                });
        }
        
        function toggleAlerts() {
            alertsEnabled = !alertsEnabled;
            document.getElementById('alertBtn').textContent = '🔔 Alerts: ' + (alertsEnabled ? 'ON' : 'OFF');
            // Send to server
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({alerts_enabled: alertsEnabled})
            });
        }
        
        function downloadData() {
            fetch('/api/history')
                .then(response => response.blob())
                .then(blob => {
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = 'sensor_data.csv';
                    a.click();
                });
        }
        
        function showConfig() {
            alert('Configuration panel coming soon!');
        }
        
        // Update every 5 seconds
        setInterval(updateData, 5000);
        updateData(); // Initial load
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

void handleCSS() {
  String css = R"(
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    margin: 0;
    padding: 20px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    background: white;
    border-radius: 15px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.2);
    overflow: hidden;
}

header {
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    color: white;
    padding: 30px;
    text-align: center;
}

header h1 {
    margin: 0;
    font-size: 2.5em;
}

.alert-banner {
    padding: 15px;
    text-align: center;
    font-weight: bold;
    margin: 0;
}

.alert-banner.warning {
    background: #fff3cd;
    color: #856404;
    border-bottom: 3px solid #ffc107;
}

.alert-banner.critical {
    background: #f8d7da;
    color: #721c24;
    border-bottom: 3px solid #dc3545;
}

.sensor-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 20px;
    padding: 30px;
}

.sensor-card {
    background: #f8f9fa;
    border-radius: 10px;
    padding: 25px;
    text-align: center;
    border-left: 5px solid #007bff;
    transition: transform 0.3s ease;
}

.sensor-card:hover {
    transform: translateY(-5px);
    box-shadow: 0 5px 15px rgba(0,0,0,0.1);
}

.sensor-card h3 {
    margin: 0 0 15px 0;
    color: #495057;
    font-size: 1.2em;
}

.sensor-card .value {
    font-size: 2.5em;
    font-weight: bold;
    color: #007bff;
    margin: 10px 0;
}

.sensor-card .range {
    color: #6c757d;
    font-size: 0.9em;
}

.controls {
    padding: 20px 30px;
    text-align: center;
    background: #f8f9fa;
    border-top: 1px solid #dee2e6;
}

.controls button {
    background: #007bff;
    color: white;
    border: none;
    padding: 12px 24px;
    margin: 0 10px;
    border-radius: 25px;
    cursor: pointer;
    font-size: 1em;
    transition: background 0.3s ease;
}

.controls button:hover {
    background: #0056b3;
}

.chart-container {
    padding: 30px;
    text-align: center;
}

.chart-container h3 {
    margin-bottom: 20px;
    color: #495057;
}

#chart {
    border: 1px solid #dee2e6;
    border-radius: 10px;
    max-width: 100%;
    height: auto;
}

@media (max-width: 768px) {
    .sensor-grid {
        grid-template-columns: 1fr;
        padding: 20px;
    }
    
    header h1 {
        font-size: 2em;
    }
    
    .sensor-card .value {
        font-size: 2em;
    }
}
)";
  
  server.send(200, "text/css", css);
}

void handleCurrentData() {
  JsonDocument doc;
  doc["temperature"] = currentReading.temperature;
  doc["humidity"] = currentReading.humidity;
  doc["pressure"] = currentReading.pressure;
  doc["altitude"] = currentReading.altitude;
  doc["light_level"] = currentReading.lightLevel;
  doc["motion"] = currentReading.motionDetected;
  doc["timestamp"] = currentReading.timestamp;
  doc["alert_level"] = alertLevel;
  doc["alert_message"] = alertMessage;
  doc["uptime"] = millis() / 1000;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleHistoryData() {
  String csv = "Timestamp,Temperature,Humidity,Pressure,Altitude,Light,Motion\n";
  
  for (const auto& reading : dataHistory) {
    csv += String(reading.timestamp) + ",";
    csv += String(reading.temperature, 2) + ",";
    csv += String(reading.humidity, 2) + ",";
    csv += String(reading.pressure, 2) + ",";
    csv += String(reading.altitude, 2) + ",";
    csv += String(reading.lightLevel) + ",";
    csv += String(reading.motionDetected ? 1 : 0) + "\n";
  }
  
  server.send(200, "text/csv", csv);
}

void handleGetConfig() {
  JsonDocument doc;
  doc["temp_min"] = config.tempMin;
  doc["temp_max"] = config.tempMax;
  doc["humidity_min"] = config.humidityMin;
  doc["humidity_max"] = config.humidityMax;
  doc["pressure_min"] = config.pressureMin;
  doc["pressure_max"] = config.pressureMax;
  doc["light_min"] = config.lightMin;
  doc["light_max"] = config.lightMax;
  doc["alerts_enabled"] = config.alertsEnabled;
  doc["reading_interval"] = config.readingInterval;
  doc["sd_logging"] = config.sdLogging;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetConfig() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("alerts_enabled")) {
      config.alertsEnabled = doc["alerts_enabled"];
      preferences.putBool("alerts", config.alertsEnabled);
    }
    
    // Add more configuration updates as needed
    
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}