/*
  WiFiEnterprise Simple LED Control Example
  
  This example shows how to create a basic web server that can
  control an LED remotely over an enterprise WiFi network.
  
  Perfect for learning basic IoT control concepts.
  
  Features:
  - Simple web interface
  - LED on/off control
  - Basic HTML page
  - Easy to understand code
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - LED connected to pin 4 (with 220Ω resistor)
  - Built-in LED on pin 2 for status
  
  Usage:
  1. Update your network credentials below
  2. Upload to ESP32
  3. Open Serial Monitor to see IP address
  4. Open web browser and go to the IP address
  5. Click buttons to control LED
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <WebServer.h>

// ========================================
// CHANGE THESE TO YOUR NETWORK SETTINGS
// ========================================
const char* ssid = "YourEnterpriseNetwork";     // Your enterprise WiFi name
const char* username = "your_username";          // Your username
const char* password = "your_password";          // Your password

// Pin definitions
const int LED_PIN = 4;        // LED to control
const int STATUS_LED = 2;     // Built-in LED for status

// Create web server on port 80
WebServer server(80);

// LED state
bool ledState = false;

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Setup pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);
  
  // Print startup message
  Serial.println();
  Serial.println("======================================");
  Serial.println("  WiFiEnterprise Simple LED Control");
  Serial.println("======================================");
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
    Serial.println("Open your browser and go to: http://" + WiFiEnterprise.localIP().toString());
    Serial.println();
    
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

void setupWebServer() {
  // Main page
  server.on("/", handleRoot);
  
  // LED control endpoints
  server.on("/led/on", handleLEDOn);
  server.on("/led/off", handleLEDOff);
  server.on("/led/toggle", handleLEDToggle);
  
  // Status endpoint
  server.on("/status", handleStatus);
  
  // Start server
  server.begin();
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>ESP32 LED Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; margin: 50px; background: #f0f0f0; }";
  html += ".container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; margin-bottom: 30px; }";
  html += ".led-status { font-size: 24px; margin: 20px 0; padding: 15px; border-radius: 8px; }";
  html += ".led-on { background: #d4edda; color: #155724; border: 2px solid #c3e6cb; }";
  html += ".led-off { background: #f8d7da; color: #721c24; border: 2px solid #f5c6cb; }";
  html += "button { font-size: 18px; padding: 15px 30px; margin: 10px; border: none; border-radius: 8px; cursor: pointer; transition: all 0.3s; }";
  html += ".btn-on { background: #28a745; color: white; }";
  html += ".btn-off { background: #dc3545; color: white; }";
  html += ".btn-toggle { background: #007bff; color: white; }";
  html += "button:hover { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.2); }";
  html += ".info { margin-top: 30px; padding: 15px; background: #e9ecef; border-radius: 8px; font-size: 14px; color: #6c757d; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>💡 ESP32 LED Control</h1>";
  
  // LED status display
  if (ledState) {
    html += "<div class='led-status led-on'>🟢 LED is ON</div>";
  } else {
    html += "<div class='led-status led-off'>🔴 LED is OFF</div>";
  }
  
  // Control buttons
  html += "<div>";
  html += "<button class='btn-on' onclick='location.href=\"/led/on\"'>Turn ON</button>";
  html += "<button class='btn-off' onclick='location.href=\"/led/off\"'>Turn OFF</button>";
  html += "<br>";
  html += "<button class='btn-toggle' onclick='location.href=\"/led/toggle\"'>Toggle LED</button>";
  html += "</div>";
  
  // Device info
  html += "<div class='info'>";
  html += "<strong>Device Info:</strong><br>";
  html += "IP: " + WiFiEnterprise.localIP().toString() + "<br>";
  html += "Uptime: " + String(millis()/1000) + " seconds<br>";
  html += "Signal: " + String(WiFi.RSSI()) + " dBm";
  html += "</div>";
  
  html += "</div>";
  
  // Auto-refresh script
  html += "<script>";
  html += "setTimeout(function(){ location.reload(); }, 30000);";
  html += "</script>";
  
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleLEDOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED turned ON via web interface");
  
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "LED turned ON");
}

void handleLEDOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED turned OFF via web interface");
  
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "LED turned OFF");
}

void handleLEDToggle() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  Serial.println("LED toggled to: " + String(ledState ? "ON" : "OFF"));
  
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "LED toggled");
}

void handleStatus() {
  String status = "{";
  status += "\"led_state\": " + String(ledState ? "true" : "false") + ",";
  status += "\"uptime\": " + String(millis()/1000) + ",";
  status += "\"ip_address\": \"" + WiFiEnterprise.localIP().toString() + "\",";
  status += "\"wifi_rssi\": " + String(WiFi.RSSI()) + ",";
  status += "\"free_heap\": " + String(ESP.getFreeHeap());
  status += "}";
  
  server.send(200, "application/json", status);
}