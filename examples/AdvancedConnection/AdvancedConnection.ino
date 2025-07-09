/*
  WiFiEnterprise Advanced Connection Example
  
  This example demonstrates advanced features of the WiFiEnterprise library:
  - Connection monitoring
  - Automatic reconnection
  - Debug control
  - Status reporting
  - Error handling
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - Optional: LED on pin 2 for connection status
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>

// Network credentials
const char* ssid = "YourEnterpriseNetwork";     // Your WPA2-Enterprise SSID
const char* username = "your_username";         // Your enterprise username
const char* password = "your_password";         // Your enterprise password

// Configuration
const int LED_PIN = 2;                          // Built-in LED pin
const unsigned long CHECK_INTERVAL = 5000;      // Check connection every 5 seconds
const int MAX_RECONNECT_ATTEMPTS = 3;           // Maximum reconnection attempts

// Variables
unsigned long lastCheck = 0;
int reconnectAttempts = 0;
bool wasConnected = false;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("WiFiEnterprise Advanced Connection Example");
  Serial.println("=========================================");
  Serial.println();
  
  // Initial connection attempt
  connectToNetwork();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check connection status periodically
  if (currentTime - lastCheck >= CHECK_INTERVAL) {
    lastCheck = currentTime;
    
    bool currentlyConnected = WiFiEnterprise.isConnected();
    
    if (currentlyConnected) {
      // Connected
      digitalWrite(LED_PIN, HIGH);
      
      if (!wasConnected) {
        Serial.println("✅ Connection established!");
        printConnectionInfo();
        reconnectAttempts = 0; // Reset reconnect counter
      }
      
      wasConnected = true;
      
    } else {
      // Disconnected
      digitalWrite(LED_PIN, LOW);
      
      if (wasConnected) {
        Serial.println("❌ Connection lost!");
        wasConnected = false;
      }
      
      // Attempt reconnection
      if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        Serial.printf("🔄 Reconnection attempt %d/%d...\n", reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
        
        if (connectToNetwork()) {
          Serial.println("✅ Reconnected successfully!");
          reconnectAttempts = 0;
        } else {
          reconnectAttempts++;
          if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            Serial.println("❌ Maximum reconnection attempts reached!");
            Serial.println("💡 Please check your network settings and restart the device.");
          }
        }
      }
    }
  }
  
  // Blink LED when disconnected
  if (!WiFiEnterprise.isConnected()) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(200);
  } else {
    delay(100);
  }
}

bool connectToNetwork() {
  Serial.println("🔗 Connecting to WPA2-Enterprise network...");
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Username: %s\n", username);
  Serial.println();
  
  // Enable debug for connection attempts
  WiFiEnterprise.setDebug(true);
  
  bool success = WiFiEnterprise.begin(ssid, username, password, true);
  
  if (success) {
    Serial.println();
    printConnectionInfo();
  } else {
    Serial.println();
    printErrorInfo();
  }
  
  // Disable debug after connection attempt
  WiFiEnterprise.setDebug(false);
  
  return success;
}

void printConnectionInfo() {
  Serial.println("📊 Connection Information:");
  Serial.printf("   IP Address: %s\n", WiFiEnterprise.localIP().toString().c_str());
  Serial.printf("   Status: %d\n", WiFiEnterprise.status());
  Serial.printf("   RSSI: %d dBm\n", WiFi.RSSI());
  Serial.printf("   MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.println();
}

void printErrorInfo() {
  Serial.println("❌ Connection Error Information:");
  Serial.printf("   Status Code: %d\n", WiFiEnterprise.status());
  
  switch (WiFiEnterprise.status()) {
    case WL_NO_SSID_AVAIL:
      Serial.println("   Error: SSID not found");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("   Error: Connection failed (check credentials)");
      break;
    case WL_CONNECTION_LOST:
      Serial.println("   Error: Connection lost");
      break;
    case WL_DISCONNECTED:
      Serial.println("   Error: Disconnected");
      break;
    default:
      Serial.println("   Error: Unknown error");
      break;
  }
  
  Serial.println("💡 Troubleshooting tips:");
  Serial.println("   - Verify SSID, username, and password");
  Serial.println("   - Check if the network supports EAP-PEAP");
  Serial.println("   - Ensure ESP32 is within range of the access point");
  Serial.println();
}