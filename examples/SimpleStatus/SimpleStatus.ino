/*
  WiFiEnterprise Simple Status Example
  
  This is the simplest possible example showing how to connect to
  a WPA2-Enterprise network and display connection status.
  
  Perfect for beginners to test their network credentials.
  
  Features:
  - Basic enterprise network connection
  - Simple status display
  - LED status indicator
  - Serial output for debugging
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - Built-in LED (pin 2) for status
  
  Usage:
  1. Update your network credentials below
  2. Upload to ESP32
  3. Open Serial Monitor at 115200 baud
  4. Watch for connection status
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>

// ========================================
// CHANGE THESE TO YOUR NETWORK SETTINGS
// ========================================
const char* ssid = "YourEnterpriseNetwork";     // Your enterprise WiFi name
const char* username = "your_username";          // Your username
const char* password = "your_password";          // Your password

// Pin for status LED (built-in LED on most ESP32 boards)
const int LED_PIN = 2;

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Setup LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Print startup message
  Serial.println();
  Serial.println("=====================================");
  Serial.println("  WiFiEnterprise Simple Status Test");
  Serial.println("=====================================");
  Serial.println();
  
  // Show what we're trying to connect to
  Serial.println("Attempting to connect to:");
  Serial.println("Network: " + String(ssid));
  Serial.println("Username: " + String(username));
  Serial.println();
  
  // Try to connect
  Serial.println("Connecting to enterprise network...");
  
  if (WiFiEnterprise.begin(ssid, username, password)) {
    // Success!
    Serial.println();
    Serial.println("✅ CONNECTION SUCCESSFUL!");
    Serial.println();
    Serial.println("Network Information:");
    Serial.println("- IP Address: " + WiFiEnterprise.localIP().toString());
    Serial.println("- MAC Address: " + WiFi.macAddress());
    Serial.println("- Signal Strength: " + String(WiFi.RSSI()) + " dBm");
    Serial.println();
    
    // Turn on LED to show success
    digitalWrite(LED_PIN, HIGH);
    
  } else {
    // Failed to connect
    Serial.println();
    Serial.println("❌ CONNECTION FAILED!");
    Serial.println();
    Serial.println("Please check:");
    Serial.println("1. Network name (SSID) is correct");
    Serial.println("2. Username is correct");
    Serial.println("3. Password is correct");
    Serial.println("4. You're in range of the network");
    Serial.println("5. The network supports WPA2-Enterprise");
    Serial.println();
  }
}

void loop() {
  // Check if we're still connected
  if (WiFiEnterprise.isConnected()) {
    // Connected - slow blink
    digitalWrite(LED_PIN, HIGH);
    delay(900);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    
    // Print status every 10 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 10000) {
      Serial.println("Status: Connected | IP: " + WiFiEnterprise.localIP().toString() + 
                     " | Signal: " + String(WiFi.RSSI()) + " dBm | Uptime: " + 
                     String(millis()/1000) + "s");
      lastPrint = millis();
    }
    
  } else {
    // Disconnected - fast blink
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    
    // Try to reconnect every 30 seconds
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect >= 30000) {
      Serial.println("Connection lost. Attempting to reconnect...");
      
      if (WiFiEnterprise.begin(ssid, username, password)) {
        Serial.println("✅ Reconnected successfully!");
      } else {
        Serial.println("❌ Reconnection failed. Will try again in 30 seconds.");
      }
      
      lastReconnect = millis();
    }
  }
}