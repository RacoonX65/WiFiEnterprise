/*
  WiFiEnterprise Web Server Example
  
  This example demonstrates how to create a simple web server using
  the WiFiEnterprise library to connect to a WPA2-Enterprise network.
  The server displays sensor data and allows basic control via web interface.
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - Optional: DHT22 sensor on pin 4
  - Optional: LED on pin 2
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Network credentials
const char* ssid = "YourEnterpriseNetwork";
const char* username = "your_username";
const char* password = "your_password";

// Web server on port 80
WebServer server(80);

// Pin definitions
const int LED_PIN = 2;
const int SENSOR_PIN = 4;

// Variables
bool ledState = false;
float temperature = 23.5;
float humidity = 60.0;
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 5000; // Read sensor every 5 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("WiFiEnterprise Web Server Example");
  Serial.println("=================================");
  
  // Connect to enterprise network
  Serial.println("Connecting to WPA2-Enterprise network...");
  
  if (WiFiEnterprise.begin(ssid, username, password, true)) {
    Serial.println("\n✅ Connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFiEnterprise.localIP());
    
    // Setup web server routes
    setupWebServer();
    
    // Start the server
    server.begin();
    Serial.println("🌐 Web server started!");
    Serial.print("Access your device at: http://");
    Serial.println(WiFiEnterprise.localIP());
    
  } else {
    Serial.println("\n❌ Connection failed!");
    Serial.println("Please check your credentials and restart.");
    while(true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
  }
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Read sensors periodically
  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    lastSensorRead = millis();
  }
  
  // Check connection status
  if (!WiFiEnterprise.isConnected()) {
    Serial.println("⚠️ Connection lost! Attempting to reconnect...");
    if (WiFiEnterprise.begin(ssid, username, password)) {
      Serial.println("✅ Reconnected!");
    }
  }
  
  delay(100);
}

void setupWebServer() {
  // Root page
  server.on("/", handleRoot);
  
  // API endpoints
  server.on("/api/status", handleStatus);
  server.on("/api/sensors", handleSensors);
  server.on("/api/led/on", handleLedOn);
  server.on("/api/led/off", handleLedOff);
  server.on("/api/led/toggle", handleLedToggle);
  
  // Handle not found
  server.onNotFound(handleNotFound);
  
  Serial.println("📡 Web server routes configured:");
  Serial.println("   GET  /              - Main dashboard");
  Serial.println("   GET  /api/status     - Device status");
  Serial.println("   GET  /api/sensors    - Sensor data");
  Serial.println("   POST /api/led/on     - Turn LED on");
  Serial.println("   POST /api/led/off    - Turn LED off");
  Serial.println("   POST /api/led/toggle - Toggle LED");
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Enterprise Dashboard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }
        .status { display: flex; justify-content: space-around; margin: 20px 0; }
        .card { background: #f8f9fa; padding: 15px; border-radius: 8px; text-align: center; min-width: 120px; }
        .card h3 { margin: 0 0 10px 0; color: #007bff; }
        .card p { margin: 0; font-size: 18px; font-weight: bold; }
        .controls { margin: 20px 0; }
        .btn { background: #007bff; color: white; border: none; padding: 10px 20px; margin: 5px; border-radius: 5px; cursor: pointer; }
        .btn:hover { background: #0056b3; }
        .btn.danger { background: #dc3545; }
        .btn.danger:hover { background: #c82333; }
        .refresh { text-align: center; margin: 20px 0; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌐 ESP32 Enterprise Dashboard</h1>
            <p>Connected via WiFiEnterprise Library</p>
        </div>
        
        <div class="status" id="status">
            <div class="card">
                <h3>🌡️ Temperature</h3>
                <p id="temp">--°C</p>
            </div>
            <div class="card">
                <h3>💧 Humidity</h3>
                <p id="humidity">--%</p>
            </div>
            <div class="card">
                <h3>💡 LED Status</h3>
                <p id="led">--</p>
            </div>
            <div class="card">
                <h3>📶 WiFi</h3>
                <p id="wifi">--</p>
            </div>
        </div>
        
        <div class="controls">
            <h3>LED Control</h3>
            <button class="btn" onclick="controlLed('on')">Turn On</button>
            <button class="btn danger" onclick="controlLed('off')">Turn Off</button>
            <button class="btn" onclick="controlLed('toggle')">Toggle</button>
        </div>
        
        <div class="refresh">
            <button class="btn" onclick="refreshData()">🔄 Refresh Data</button>
        </div>
    </div>
    
    <script>
        function refreshData() {
            fetch('/api/sensors')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temp').textContent = data.temperature + '°C';
                    document.getElementById('humidity').textContent = data.humidity + '%';
                });
            
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('led').textContent = data.led ? 'ON' : 'OFF';
                    document.getElementById('wifi').textContent = data.connected ? 'Connected' : 'Disconnected';
                });
        }
        
        function controlLed(action) {
            fetch('/api/led/' + action, { method: 'POST' })
                .then(() => refreshData());
        }
        
        // Auto-refresh every 5 seconds
        setInterval(refreshData, 5000);
        
        // Initial load
        refreshData();
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  JsonDocument doc;
  doc["connected"] = WiFiEnterprise.isConnected();
  doc["ip"] = WiFiEnterprise.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["led"] = ledState;
  doc["uptime"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSensors() {
  JsonDocument doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = millis();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleLedOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  server.send(200, "text/plain", "LED turned ON");
  Serial.println("💡 LED turned ON via web interface");
}

void handleLedOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "LED turned OFF");
  Serial.println("💡 LED turned OFF via web interface");
}

void handleLedToggle() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  server.send(200, "text/plain", ledState ? "LED turned ON" : "LED turned OFF");
  Serial.println("💡 LED toggled via web interface: " + String(ledState ? "ON" : "OFF"));
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

void readSensors() {
  // Simulate sensor readings (replace with actual sensor code)
  temperature = 20.0 + random(0, 100) / 10.0; // 20-30°C
  humidity = 40.0 + random(0, 400) / 10.0;     // 40-80%
  
  // For real DHT22 sensor, use:
  // temperature = dht.readTemperature();
  // humidity = dht.readHumidity();
  
  Serial.println("📊 Sensor readings updated: T=" + String(temperature, 1) + "°C, H=" + String(humidity, 1) + "%");
}