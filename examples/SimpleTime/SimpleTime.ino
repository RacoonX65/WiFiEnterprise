/*
  WiFiEnterprise Simple Time Example
  
  This example shows how to get the current time from the internet
  using NTP (Network Time Protocol) over an enterprise WiFi network.
  
  Perfect for learning basic time synchronization in IoT projects.
  
  Features:
  - NTP time synchronization
  - Multiple timezone support
  - Simple web display
  - Serial time output
  - Automatic time updates
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board only
  
  Usage:
  1. Update your network credentials below
  2. Set your timezone offset
  3. Upload to ESP32
  4. Open Serial Monitor to see time updates
  5. Open web browser and go to the IP address
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <WebServer.h>
#include <time.h>

// ========================================
// CHANGE THESE TO YOUR NETWORK SETTINGS
// ========================================
const char* ssid = "YourEnterpriseNetwork";     // Your enterprise WiFi name
const char* username = "your_username";          // Your username
const char* password = "your_password";          // Your password

// ========================================
// TIMEZONE SETTINGS
// ========================================
// Common timezone offsets (in seconds):
// UTC: 0
// EST (New York): -5 * 3600 = -18000
// PST (Los Angeles): -8 * 3600 = -28800
// CET (Berlin): 1 * 3600 = 3600
// JST (Tokyo): 9 * 3600 = 32400
// SAST (South Africa): 2 * 3600 = 7200
const long timezoneOffset = 2 * 3600;  // Change this to your timezone offset in seconds
const char* timezoneName = "SAST";      // Change this to your timezone name

// NTP servers
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const char* ntpServer3 = "time.google.com";

// Pin definitions
const int STATUS_LED = 2;     // Built-in LED for status

// Create web server on port 80
WebServer server(80);

// Time variables
bool timeInitialized = false;
unsigned long lastTimeUpdate = 0;
const unsigned long TIME_UPDATE_INTERVAL = 3600000; // Update every hour

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
  Serial.println("  WiFiEnterprise Simple Time");
  Serial.println("==================================");
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
    
    // Initialize time
    initializeTime();
    
    // Setup web server
    setupWebServer();
    
    Serial.println("🌐 Web server started!");
    Serial.println("🕐 Time service active!");
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
  
  // Print time every 10 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 10000) {
    if (timeInitialized) {
      printCurrentTime();
    }
    lastPrint = millis();
  }
  
  // Update time periodically
  if (timeInitialized && (millis() - lastTimeUpdate >= TIME_UPDATE_INTERVAL)) {
    Serial.println("🔄 Updating time from NTP servers...");
    initializeTime();
  }
  
  // Check WiFi connection
  if (!WiFiEnterprise.isConnected()) {
    Serial.println("WiFi connection lost!");
    digitalWrite(STATUS_LED, LOW);
    timeInitialized = false;
    
    // Try to reconnect
    if (WiFiEnterprise.begin(ssid, username, password)) {
      Serial.println("WiFi reconnected!");
      digitalWrite(STATUS_LED, HIGH);
      initializeTime();
    }
  }
  
  delay(100);
}

void initializeTime() {
  Serial.println("🕐 Initializing time from NTP servers...");
  Serial.println("   Primary: " + String(ntpServer1));
  Serial.println("   Secondary: " + String(ntpServer2));
  Serial.println("   Backup: " + String(ntpServer3));
  
  // Configure time with timezone offset and daylight saving
  configTime(timezoneOffset, 0, ntpServer1, ntpServer2, ntpServer3);
  
  // Wait for time to be set
  Serial.print("   Waiting for time sync");
  int attempts = 0;
  while (!time(nullptr) && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (time(nullptr)) {
    timeInitialized = true;
    lastTimeUpdate = millis();
    Serial.println("✅ Time synchronized successfully!");
    printCurrentTime();
  } else {
    timeInitialized = false;
    Serial.println("❌ Failed to synchronize time!");
  }
  Serial.println();
}

void printCurrentTime() {
  if (!timeInitialized) return;
  
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%A, %B %d, %Y %H:%M:%S", timeinfo);
  
  Serial.println("🕐 Current Time: " + String(timeStr) + " " + String(timezoneName));
}

void setupWebServer() {
  // Main page
  server.on("/", handleRoot);
  
  // Time API endpoint
  server.on("/time", handleTime);
  
  // Start server
  server.begin();
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>ESP32 Time Display</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Arial; text-align: center; margin: 0; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%); color: white; min-height: 100vh; display: flex; align-items: center; justify-content: center; }";
  html += ".container { max-width: 600px; background: rgba(255,255,255,0.1); padding: 40px; border-radius: 20px; backdrop-filter: blur(15px); box-shadow: 0 8px 32px rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.2); }";
  html += "h1 { margin-bottom: 40px; font-size: 32px; font-weight: 300; }";
  html += ".time-display { background: rgba(255,255,255,0.2); padding: 30px; border-radius: 15px; margin: 30px 0; border: 1px solid rgba(255,255,255,0.3); }";
  html += ".current-time { font-size: 48px; font-weight: bold; margin: 20px 0; font-family: 'Courier New', monospace; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }";
  html += ".current-date { font-size: 24px; margin: 15px 0; opacity: 0.9; }";
  html += ".timezone { font-size: 18px; opacity: 0.8; margin: 10px 0; }";
  html += ".info-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 30px 0; }";
  html += ".info-card { background: rgba(255,255,255,0.1); padding: 20px; border-radius: 12px; border: 1px solid rgba(255,255,255,0.2); }";
  html += ".info-title { font-size: 14px; opacity: 0.8; margin-bottom: 8px; }";
  html += ".info-value { font-size: 18px; font-weight: bold; }";
  html += ".status { margin-top: 30px; padding: 15px; background: rgba(0,255,0,0.2); border-radius: 10px; border: 1px solid rgba(0,255,0,0.3); }";
  html += "@media (max-width: 600px) { .info-grid { grid-template-columns: 1fr; } .current-time { font-size: 36px; } }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>🕐 ESP32 Time Display</h1>";
  
  if (timeInitialized) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char timeStr[32], dateStr[64];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
    strftime(dateStr, sizeof(dateStr), "%A, %B %d, %Y", timeinfo);
    
    html += "<div class='time-display'>";
    html += "<div class='current-time'>" + String(timeStr) + "</div>";
    html += "<div class='current-date'>" + String(dateStr) + "</div>";
    html += "<div class='timezone'>" + String(timezoneName) + " (UTC" + (timezoneOffset >= 0 ? "+" : "") + String(timezoneOffset/3600) + ")</div>";
    html += "</div>";
    
    // Additional time info
    html += "<div class='info-grid'>";
    
    html += "<div class='info-card'>";
    html += "<div class='info-title'>Unix Timestamp</div>";
    html += "<div class='info-value'>" + String((unsigned long)now) + "</div>";
    html += "</div>";
    
    html += "<div class='info-card'>";
    html += "<div class='info-title'>Day of Year</div>";
    html += "<div class='info-value'>" + String(timeinfo->tm_yday + 1) + " / 365</div>";
    html += "</div>";
    
    html += "<div class='info-card'>";
    html += "<div class='info-title'>Week Day</div>";
    char weekday[16];
    strftime(weekday, sizeof(weekday), "%A", timeinfo);
    html += "<div class='info-value'>" + String(weekday) + "</div>";
    html += "</div>";
    
    html += "<div class='info-card'>";
    html += "<div class='info-title'>Time Since Boot</div>";
    html += "<div class='info-value'>" + formatUptime(millis()) + "</div>";
    html += "</div>";
    
    html += "</div>";
    
    html += "<div class='status'>";
    html += "✅ Time synchronized with NTP servers<br>";
    html += "Last update: " + String((millis() - lastTimeUpdate)/1000) + " seconds ago";
    html += "</div>";
    
  } else {
    html += "<div class='time-display'>";
    html += "<div class='current-time'>--:--:--</div>";
    html += "<div class='current-date'>Time not synchronized</div>";
    html += "</div>";
    
    html += "<div class='status' style='background: rgba(255,0,0,0.2); border-color: rgba(255,0,0,0.3);'>";
    html += "❌ Waiting for time synchronization...";
    html += "</div>";
  }
  
  // Device info
  html += "<div style='margin-top: 30px; padding: 20px; background: rgba(255,255,255,0.1); border-radius: 10px; font-size: 14px; opacity: 0.8;'>";
  html += "<strong>📡 Device Information</strong><br>";
  html += "IP: " + WiFiEnterprise.localIP().toString() + " | ";
  html += "Signal: " + String(WiFi.RSSI()) + " dBm | ";
  html += "Memory: " + String(ESP.getFreeHeap()) + " bytes";
  html += "</div>";
  
  html += "</div>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleTime() {
  String json = "{";
  
  if (timeInitialized) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char timeStr[32], dateStr[64], isoStr[32];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
    strftime(dateStr, sizeof(dateStr), "%A, %B %d, %Y", timeinfo);
    strftime(isoStr, sizeof(isoStr), "%Y-%m-%dT%H:%M:%S", timeinfo);
    
    json += "\"synchronized\": true,";
    json += "\"time\": \"" + String(timeStr) + "\",";
    json += "\"date\": \"" + String(dateStr) + "\",";
    json += "\"iso\": \"" + String(isoStr) + "\",";
    json += "\"timezone\": \"" + String(timezoneName) + "\",";
    json += "\"timezone_offset\": " + String(timezoneOffset) + ",";
    json += "\"unix_timestamp\": " + String((unsigned long)now) + ",";
    json += "\"day_of_year\": " + String(timeinfo->tm_yday + 1) + ",";
    json += "\"last_sync_ago\": " + String((millis() - lastTimeUpdate)/1000);
  } else {
    json += "\"synchronized\": false,";
    json += "\"error\": \"Time not synchronized\"";
  }
  
  json += ",\"uptime\": " + String(millis()/1000);
  json += ",\"ip_address\": \"" + WiFiEnterprise.localIP().toString() + "\"";
  json += ",\"wifi_rssi\": " + String(WiFi.RSSI());
  json += ",\"free_heap\": " + String(ESP.getFreeHeap());
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