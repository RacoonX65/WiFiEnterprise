/*
  WiFiEnterprise Simple Ping Example
  
  This example shows how to test network connectivity by pinging
  various servers over an enterprise WiFi network.
  
  Perfect for learning basic network diagnostics and connectivity testing.
  
  Features:
  - Ping multiple servers
  - Response time measurement
  - Success/failure tracking
  - Simple web interface
  - Serial output for debugging
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board only
  
  Usage:
  1. Update your network credentials below
  2. Upload to ESP32
  3. Open Serial Monitor to see ping results
  4. Open web browser and go to the IP address
  5. Watch live connectivity status
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <WebServer.h>
#include <WiFiClient.h>

// ========================================
// CHANGE THESE TO YOUR NETWORK SETTINGS
// ========================================
const char* ssid = "YourEnterpriseNetwork";     // Your enterprise WiFi name
const char* username = "your_username";          // Your username
const char* password = "your_password";          // Your password

// Pin definitions
const int STATUS_LED = 2;     // Built-in LED for status

// Create web server on port 80
WebServer server(80);

// Ping targets
struct PingTarget {
  String name;
  String host;
  int port;
  unsigned long lastPing;
  bool isReachable;
  unsigned long responseTime;
  int successCount;
  int totalAttempts;
};

PingTarget targets[] = {
  {"Google DNS", "8.8.8.8", 53, 0, false, 0, 0, 0},
  {"Cloudflare DNS", "1.1.1.1", 53, 0, false, 0, 0, 0},
  {"Google", "google.com", 80, 0, false, 0, 0, 0},
  {"GitHub", "github.com", 443, 0, false, 0, 0, 0}
};

const int NUM_TARGETS = sizeof(targets) / sizeof(targets[0]);
const unsigned long PING_INTERVAL = 10000; // Ping every 10 seconds
int currentTarget = 0;

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Setup pins
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  // Print startup message
  Serial.println();
  Serial.println("==================================");
  Serial.println("  WiFiEnterprise Simple Ping Test");
  Serial.println("==================================");
  Serial.println();
  
  // Connect to WiFi
  Serial.println("Connecting to enterprise network...");
  Serial.println("Network: " + String(ssid));
  Serial.println();
  
  if (WiFiEnterprise.begin(ssid, username, password)) {
    Serial.println("✅ WiFi Connected!");
    Serial.println("IP Address: " + WiFiEnterprise.localIP().toString());
    Serial.println("Gateway: " + WiFi.gatewayIP().toString());
    Serial.println("DNS: " + WiFi.dnsIP().toString());
    Serial.println();
    
    // Turn on status LED
    digitalWrite(STATUS_LED, HIGH);
    
    // Setup web server
    setupWebServer();
    
    Serial.println("🌐 Web server started!");
    Serial.println("🏓 Ping testing active!");
    Serial.println("Open your browser and go to: http://" + WiFiEnterprise.localIP().toString());
    Serial.println();
    
    // Start ping tests
    Serial.println("Starting connectivity tests...");
    Serial.println("Targets:");
    for (int i = 0; i < NUM_TARGETS; i++) {
      Serial.println("  " + String(i+1) + ". " + targets[i].name + " (" + targets[i].host + ":" + String(targets[i].port) + ")");
    }
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
  
  // Perform ping tests
  if (millis() - targets[currentTarget].lastPing >= PING_INTERVAL) {
    performPingTest(currentTarget);
    currentTarget = (currentTarget + 1) % NUM_TARGETS;
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
  
  delay(100);
}

void performPingTest(int targetIndex) {
  PingTarget& target = targets[targetIndex];
  target.lastPing = millis();
  target.totalAttempts++;
  
  Serial.print("🏓 Testing " + target.name + " (" + target.host + ":" + String(target.port) + ")... ");
  
  WiFiClient client;
  unsigned long startTime = millis();
  
  // Try to connect to the target
  bool connected = client.connect(target.host.c_str(), target.port);
  unsigned long endTime = millis();
  
  if (connected) {
    target.isReachable = true;
    target.responseTime = endTime - startTime;
    target.successCount++;
    
    Serial.println("✅ SUCCESS (" + String(target.responseTime) + "ms)");
    client.stop();
    
  } else {
    target.isReachable = false;
    target.responseTime = 0;
    
    Serial.println("❌ FAILED (timeout or unreachable)");
  }
  
  // Print summary
  float successRate = (target.totalAttempts > 0) ? (float)target.successCount / target.totalAttempts * 100 : 0;
  Serial.println("   Success rate: " + String(successRate, 1) + "% (" + String(target.successCount) + "/" + String(target.totalAttempts) + ")");
  Serial.println();
}

void setupWebServer() {
  // Main page
  server.on("/", handleRoot);
  
  // Ping results API endpoint
  server.on("/ping", handlePingResults);
  
  // Manual ping endpoint
  server.on("/ping/test", handleManualPing);
  
  // Start server
  server.begin();
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>ESP32 Ping Monitor</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='15'>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Arial; margin: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; min-height: 100vh; padding: 20px; box-sizing: border-box; }";
  html += ".container { max-width: 800px; margin: 0 auto; }";
  html += "h1 { text-align: center; margin-bottom: 40px; font-size: 32px; font-weight: 300; }";
  html += ".ping-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr)); gap: 20px; margin: 30px 0; }";
  html += ".ping-card { background: rgba(255,255,255,0.1); padding: 25px; border-radius: 15px; backdrop-filter: blur(10px); border: 1px solid rgba(255,255,255,0.2); }";
  html += ".ping-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }";
  html += ".ping-name { font-size: 18px; font-weight: bold; }";
  html += ".ping-status { padding: 5px 12px; border-radius: 20px; font-size: 12px; font-weight: bold; }";
  html += ".status-online { background: #28a745; color: white; }";
  html += ".status-offline { background: #dc3545; color: white; }";
  html += ".ping-details { font-size: 14px; line-height: 1.6; }";
  html += ".ping-host { opacity: 0.8; margin-bottom: 10px; }";
  html += ".ping-metrics { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin: 15px 0; }";
  html += ".metric { text-align: center; }";
  html += ".metric-value { font-size: 24px; font-weight: bold; margin-bottom: 5px; }";
  html += ".metric-label { font-size: 12px; opacity: 0.8; }";
  html += ".summary { background: rgba(255,255,255,0.1); padding: 25px; border-radius: 15px; margin: 30px 0; text-align: center; }";
  html += ".summary-stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".summary-item { }";
  html += ".summary-value { font-size: 28px; font-weight: bold; margin-bottom: 5px; }";
  html += ".summary-label { font-size: 14px; opacity: 0.8; }";
  html += ".device-info { background: rgba(255,255,255,0.1); padding: 20px; border-radius: 10px; margin: 30px 0; font-size: 14px; text-align: center; }";
  html += "@media (max-width: 600px) { .ping-grid { grid-template-columns: 1fr; } .ping-metrics { grid-template-columns: 1fr; } }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>🏓 ESP32 Ping Monitor</h1>";
  
  // Calculate overall statistics
  int totalOnline = 0;
  int totalTests = 0;
  int totalSuccesses = 0;
  unsigned long avgResponseTime = 0;
  int responsiveTargets = 0;
  
  for (int i = 0; i < NUM_TARGETS; i++) {
    if (targets[i].isReachable) totalOnline++;
    totalTests += targets[i].totalAttempts;
    totalSuccesses += targets[i].successCount;
    if (targets[i].isReachable && targets[i].responseTime > 0) {
      avgResponseTime += targets[i].responseTime;
      responsiveTargets++;
    }
  }
  
  if (responsiveTargets > 0) {
    avgResponseTime /= responsiveTargets;
  }
  
  float overallSuccessRate = (totalTests > 0) ? (float)totalSuccesses / totalTests * 100 : 0;
  
  // Summary section
  html += "<div class='summary'>";
  html += "<h2>📊 Network Status Summary</h2>";
  html += "<div class='summary-stats'>";
  html += "<div class='summary-item'>";
  html += "<div class='summary-value'>" + String(totalOnline) + "/" + String(NUM_TARGETS) + "</div>";
  html += "<div class='summary-label'>Targets Online</div>";
  html += "</div>";
  html += "<div class='summary-item'>";
  html += "<div class='summary-value'>" + String(overallSuccessRate, 1) + "%</div>";
  html += "<div class='summary-label'>Success Rate</div>";
  html += "</div>";
  html += "<div class='summary-item'>";
  html += "<div class='summary-value'>" + String(avgResponseTime) + "ms</div>";
  html += "<div class='summary-label'>Avg Response</div>";
  html += "</div>";
  html += "<div class='summary-item'>";
  html += "<div class='summary-value'>" + String(totalTests) + "</div>";
  html += "<div class='summary-label'>Total Tests</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  // Individual ping cards
  html += "<div class='ping-grid'>";
  
  for (int i = 0; i < NUM_TARGETS; i++) {
    PingTarget& target = targets[i];
    float successRate = (target.totalAttempts > 0) ? (float)target.successCount / target.totalAttempts * 100 : 0;
    
    html += "<div class='ping-card'>";
    html += "<div class='ping-header'>";
    html += "<div class='ping-name'>" + target.name + "</div>";
    
    if (target.isReachable) {
      html += "<div class='ping-status status-online'>ONLINE</div>";
    } else {
      html += "<div class='ping-status status-offline'>OFFLINE</div>";
    }
    
    html += "</div>";
    html += "<div class='ping-host'>" + target.host + ":" + String(target.port) + "</div>";
    
    html += "<div class='ping-metrics'>";
    html += "<div class='metric'>";
    if (target.isReachable) {
      html += "<div class='metric-value'>" + String(target.responseTime) + "ms</div>";
    } else {
      html += "<div class='metric-value'>--</div>";
    }
    html += "<div class='metric-label'>Response Time</div>";
    html += "</div>";
    
    html += "<div class='metric'>";
    html += "<div class='metric-value'>" + String(successRate, 1) + "%</div>";
    html += "<div class='metric-label'>Success Rate</div>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='ping-details'>";
    html += "Tests: " + String(target.successCount) + "/" + String(target.totalAttempts) + "<br>";
    
    if (target.lastPing > 0) {
      unsigned long timeSinceLastPing = (millis() - target.lastPing) / 1000;
      html += "Last test: " + String(timeSinceLastPing) + "s ago";
    } else {
      html += "Last test: Never";
    }
    
    html += "</div>";
    html += "</div>";
  }
  
  html += "</div>";
  
  // Device info
  html += "<div class='device-info'>";
  html += "<strong>📡 Device Information</strong><br>";
  html += "IP: " + WiFiEnterprise.localIP().toString() + " | ";
  html += "Gateway: " + WiFi.gatewayIP().toString() + " | ";
  html += "DNS: " + WiFi.dnsIP().toString() + "<br>";
  html += "Signal: " + String(WiFi.RSSI()) + " dBm | ";
  html += "Uptime: " + formatUptime(millis()) + " | ";
  html += "Memory: " + String(ESP.getFreeHeap()) + " bytes<br>";
  html += "Auto-refresh every 15 seconds";
  html += "</div>";
  
  html += "</div>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handlePingResults() {
  String json = "{";
  json += "\"targets\": [";
  
  for (int i = 0; i < NUM_TARGETS; i++) {
    if (i > 0) json += ",";
    
    PingTarget& target = targets[i];
    float successRate = (target.totalAttempts > 0) ? (float)target.successCount / target.totalAttempts * 100 : 0;
    
    json += "{";
    json += "\"name\": \"" + target.name + "\",";
    json += "\"host\": \"" + target.host + "\",";
    json += "\"port\": " + String(target.port) + ",";
    json += "\"reachable\": " + String(target.isReachable ? "true" : "false") + ",";
    json += "\"response_time\": " + String(target.responseTime) + ",";
    json += "\"success_rate\": " + String(successRate, 2) + ",";
    json += "\"success_count\": " + String(target.successCount) + ",";
    json += "\"total_attempts\": " + String(target.totalAttempts) + ",";
    json += "\"last_ping_ago\": " + String((millis() - target.lastPing)/1000);
    json += "}";
  }
  
  json += "],";
  json += "\"uptime\": " + String(millis()/1000) + ",";
  json += "\"ip_address\": \"" + WiFiEnterprise.localIP().toString() + "\",";
  json += "\"gateway\": \"" + WiFi.gatewayIP().toString() + "\",";
  json += "\"dns\": \"" + WiFi.dnsIP().toString() + "\",";
  json += "\"wifi_rssi\": " + String(WiFi.RSSI()) + ",";
  json += "\"free_heap\": " + String(ESP.getFreeHeap());
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleManualPing() {
  Serial.println("🔄 Manual ping test requested via web interface");
  
  // Perform immediate ping test on all targets
  for (int i = 0; i < NUM_TARGETS; i++) {
    performPingTest(i);
    delay(100); // Small delay between tests
  }
  
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Ping tests completed");
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