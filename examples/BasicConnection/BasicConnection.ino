/*
  WiFiEnterprise Basic Connection Example
  
  This example demonstrates how to connect to a WPA2-Enterprise network
  using the WiFiEnterprise library with just SSID, username, and password.
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>

// Network credentials
const char* ssid = "YourEnterpriseNetwork";     // Your WPA2-Enterprise SSID
const char* username = "your_username";         // Your enterprise username
const char* password = "your_password";         // Your enterprise password

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("WiFiEnterprise Basic Connection Example");
  Serial.println("=====================================");
  
  // Connect to WPA2-Enterprise network with debug enabled
  Serial.println("Connecting to WPA2-Enterprise network...");
  
  if (WiFiEnterprise.begin(ssid, username, password, true)) {
    Serial.println("\n✅ Connection successful!");
    Serial.print("IP Address: ");
    Serial.println(WiFiEnterprise.localIP());
    Serial.print("Status: ");
    Serial.println(WiFiEnterprise.status());
  } else {
    Serial.println("\n❌ Connection failed!");
    Serial.println("Please check your credentials and network settings.");
  }
}

void loop() {
  // Check connection status every 10 seconds
  if (WiFiEnterprise.isConnected()) {
    Serial.println("📶 Still connected to enterprise network");
    Serial.print("IP: ");
    Serial.println(WiFiEnterprise.localIP());
  } else {
    Serial.println("❌ Connection lost! Attempting to reconnect...");
    
    // Try to reconnect
    if (WiFiEnterprise.begin(ssid, username, password, true)) {
      Serial.println("✅ Reconnected successfully!");
    } else {
      Serial.println("❌ Reconnection failed!");
    }
  }
  
  delay(10000); // Wait 10 seconds before next check
}