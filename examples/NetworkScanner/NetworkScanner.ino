/*
  WiFiEnterprise Network Scanner Example
  
  This example demonstrates how to scan for available WiFi networks
  and identify enterprise networks that may be compatible with the
  WiFiEnterprise library.
  
  Features:
  - Comprehensive network scanning
  - Enterprise network detection
  - Signal strength analysis
  - Security type identification
  - Network quality assessment
  - JSON output for integration
  - Periodic scanning
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - Status LED on pin 2 (optional)
  
  Usage:
  1. Upload this sketch to your ESP32
  2. Open Serial Monitor at 115200 baud
  3. View detailed network scan results
  4. Identify enterprise networks for connection
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFi.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>

// Pin definitions
const int LED_PIN = 2;

// Scan configuration
const int SCAN_INTERVAL = 30000; // 30 seconds
const int MAX_NETWORKS = 50;
const int SCAN_TIMEOUT = 10000; // 10 seconds

// Network information structure
struct NetworkInfo {
  String ssid;
  String bssid;
  int32_t rssi;
  uint8_t channel;
  wifi_auth_mode_t authMode;
  bool isHidden;
  bool isEnterprise;
  String securityType;
  String qualityRating;
  int frequency;
};

// Global variables
std::vector<NetworkInfo> networks;
unsigned long lastScan = 0;
int scanCount = 0;
bool scanInProgress = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  printHeader();
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("🔍 WiFi Network Scanner initialized");
  Serial.println("📡 Starting network discovery...");
  Serial.println();
  
  // Perform initial scan
  performNetworkScan();
}

void loop() {
  // Check if it's time for another scan
  if (millis() - lastScan >= SCAN_INTERVAL && !scanInProgress) {
    performNetworkScan();
  }
  
  // Handle serial commands
  if (Serial.available()) {
    handleSerialCommand();
  }
  
  // Heartbeat LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= (scanInProgress ? 200 : 1000)) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
  
  delay(100);
}

void printHeader() {
  Serial.println("\n" + String('=', 60));
  Serial.println("        WiFiEnterprise Network Scanner");
  Serial.println("" + String('=', 60));
  Serial.println("ESP32 Chip: " + String(ESP.getChipModel()));
  Serial.println("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("MAC Address: " + WiFi.macAddress());
  Serial.println("" + String('=', 60));
  Serial.println();
  
  Serial.println("📋 Commands:");
  Serial.println("   's' - Start new scan");
  Serial.println("   'e' - Show enterprise networks only");
  Serial.println("   'j' - Output JSON format");
  Serial.println("   'c' - Show channel distribution");
  Serial.println("   'h' - Show this help");
  Serial.println();
}

void performNetworkScan() {
  scanInProgress = true;
  scanCount++;
  lastScan = millis();
  
  Serial.println("🔍 Scan #" + String(scanCount) + " - " + getCurrentTime());
  Serial.println("📡 Scanning for networks...");
  
  // Clear previous results
  networks.clear();
  
  // Start WiFi scan
  int networkCount = WiFi.scanNetworks(false, true, false, SCAN_TIMEOUT);
  
  if (networkCount == WIFI_SCAN_FAILED) {
    Serial.println("❌ Network scan failed!");
    scanInProgress = false;
    return;
  }
  
  if (networkCount == 0) {
    Serial.println("📭 No networks found");
    scanInProgress = false;
    return;
  }
  
  Serial.println("📊 Found " + String(networkCount) + " networks");
  Serial.println();
  
  // Process scan results
  for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
    NetworkInfo network;
    
    network.ssid = WiFi.SSID(i);
    network.bssid = WiFi.BSSIDstr(i);
    network.rssi = WiFi.RSSI(i);
    network.channel = WiFi.channel(i);
    network.authMode = WiFi.encryptionType(i);
    network.isHidden = (network.ssid.length() == 0);
    network.securityType = getSecurityType(network.authMode);
    network.isEnterprise = isEnterpriseNetwork(network.authMode);
    network.qualityRating = getSignalQuality(network.rssi);
    network.frequency = getFrequency(network.channel);
    
    networks.push_back(network);
  }
  
  // Sort networks by signal strength
  std::sort(networks.begin(), networks.end(), [](const NetworkInfo& a, const NetworkInfo& b) {
    return a.rssi > b.rssi;
  });
  
  // Display results
  displayScanResults();
  
  // Clean up
  WiFi.scanDelete();
  scanInProgress = false;
  
  Serial.println("✅ Scan completed\n");
}

void displayScanResults() {
  Serial.println("" + String('-', 80));
  Serial.printf("%-3s %-25s %-18s %-4s %-4s %-15s %-10s %s\n", 
                "#", "SSID", "BSSID", "Ch", "RSSI", "Security", "Quality", "Enterprise");
  Serial.println("" + String('-', 80));
  
  int enterpriseCount = 0;
  int hiddenCount = 0;
  
  for (size_t i = 0; i < networks.size(); i++) {
    const NetworkInfo& net = networks[i];
    
    String ssid = net.isHidden ? "<Hidden Network>" : net.ssid;
    if (ssid.length() > 25) ssid = ssid.substring(0, 22) + "...";
    
    String enterprise = net.isEnterprise ? "✅ YES" : "❌ No";
    
    Serial.printf("%-3d %-25s %-18s %-4d %-4d %-15s %-10s %s\n",
                  i + 1,
                  ssid.c_str(),
                  net.bssid.c_str(),
                  net.channel,
                  net.rssi,
                  net.securityType.c_str(),
                  net.qualityRating.c_str(),
                  enterprise.c_str());
    
    if (net.isEnterprise) enterpriseCount++;
    if (net.isHidden) hiddenCount++;
  }
  
  Serial.println("" + String('-', 80));
  Serial.println("📊 Summary:");
  Serial.println("   Total Networks: " + String(networks.size()));
  Serial.println("   Enterprise Networks: " + String(enterpriseCount));
  Serial.println("   Hidden Networks: " + String(hiddenCount));
  Serial.println("   2.4GHz Networks: " + String(count24GHz()));
  Serial.println("   5GHz Networks: " + String(count5GHz()));
  
  if (enterpriseCount > 0) {
    Serial.println();
    Serial.println("🏢 Enterprise Networks Detected:");
    showEnterpriseNetworks();
  }
  
  Serial.println();
}

void showEnterpriseNetworks() {
  for (const auto& net : networks) {
    if (net.isEnterprise) {
      Serial.println("   📡 " + (net.isHidden ? "<Hidden>" : net.ssid));
      Serial.println("      BSSID: " + net.bssid);
      Serial.println("      Channel: " + String(net.channel) + " (" + String(net.frequency) + " MHz)");
      Serial.println("      Signal: " + String(net.rssi) + " dBm (" + net.qualityRating + ")");
      Serial.println("      Security: " + net.securityType);
      Serial.println();
    }
  }
}

void handleSerialCommand() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();
  
  if (command == "s") {
    Serial.println("🔄 Starting manual scan...");
    performNetworkScan();
    
  } else if (command == "e") {
    Serial.println("🏢 Enterprise Networks Only:");
    showEnterpriseNetworks();
    
  } else if (command == "j") {
    Serial.println("📄 JSON Output:");
    outputJSON();
    
  } else if (command == "c") {
    Serial.println("📊 Channel Distribution:");
    showChannelDistribution();
    
  } else if (command == "h") {
    printHeader();
    
  } else if (command.length() > 0) {
    Serial.println("❓ Unknown command: " + command);
    Serial.println("   Type 'h' for help");
  }
}

void outputJSON() {
  JsonDocument doc;
  JsonArray networksArray = doc["networks"].to<JsonArray>();
  
  for (const auto& net : networks) {
    JsonObject networkObj = networksArray.add<JsonObject>();
    networkObj["ssid"] = net.isHidden ? "" : net.ssid;
    networkObj["bssid"] = net.bssid;
    networkObj["rssi"] = net.rssi;
    networkObj["channel"] = net.channel;
    networkObj["frequency"] = net.frequency;
    networkObj["security"] = net.securityType;
    networkObj["enterprise"] = net.isEnterprise;
    networkObj["hidden"] = net.isHidden;
    networkObj["quality"] = net.qualityRating;
  }
  
  doc["scan_info"]["timestamp"] = getCurrentTime();
  doc["scan_info"]["count"] = networks.size();
  doc["scan_info"]["enterprise_count"] = countEnterpriseNetworks();
  doc["scan_info"]["scanner_version"] = "1.0.0";
  
  String output;
  serializeJsonPretty(doc, output);
  Serial.println(output);
  Serial.println();
}

void showChannelDistribution() {
  int channelCount[15] = {0}; // Channels 1-14 + others
  
  for (const auto& net : networks) {
    if (net.channel >= 1 && net.channel <= 14) {
      channelCount[net.channel]++;
    } else {
      channelCount[0]++; // Other channels
    }
  }
  
  Serial.println("📊 2.4GHz Channel Usage:");
  for (int i = 1; i <= 14; i++) {
    if (channelCount[i] > 0) {
      String bar = "";
      for (int j = 0; j < channelCount[i]; j++) {
        bar += "█";
      }
      Serial.printf("   Ch %2d: %s (%d networks)\n", i, bar.c_str(), channelCount[i]);
    }
  }
  
  if (channelCount[0] > 0) {
    Serial.println("   5GHz: " + String(channelCount[0]) + " networks");
  }
  
  // Find least congested channels
  int minCount = 999;
  String bestChannels = "";
  for (int i = 1; i <= 11; i++) { // Only check 1-11 for recommendations
    if (channelCount[i] < minCount) {
      minCount = channelCount[i];
      bestChannels = String(i);
    } else if (channelCount[i] == minCount) {
      bestChannels += ", " + String(i);
    }
  }
  
  Serial.println("\n💡 Least congested channels: " + bestChannels + " (" + String(minCount) + " networks)");
  Serial.println();
}

String getSecurityType(wifi_auth_mode_t authMode) {
  switch (authMode) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3-PSK";
    case WIFI_AUTH_WAPI_PSK: return "WAPI-PSK";
    default: return "Unknown";
  }
}

bool isEnterpriseNetwork(wifi_auth_mode_t authMode) {
  return (authMode == WIFI_AUTH_WPA2_ENTERPRISE);
}

String getSignalQuality(int32_t rssi) {
  if (rssi >= -30) return "Excellent";
  else if (rssi >= -50) return "Very Good";
  else if (rssi >= -60) return "Good";
  else if (rssi >= -70) return "Fair";
  else if (rssi >= -80) return "Weak";
  else return "Very Weak";
}

int getFrequency(uint8_t channel) {
  if (channel >= 1 && channel <= 14) {
    return 2412 + (channel - 1) * 5; // 2.4GHz
  } else if (channel >= 36) {
    return 5000 + channel * 5; // 5GHz (approximate)
  }
  return 0;
}

int count24GHz() {
  int count = 0;
  for (const auto& net : networks) {
    if (net.channel >= 1 && net.channel <= 14) count++;
  }
  return count;
}

int count5GHz() {
  int count = 0;
  for (const auto& net : networks) {
    if (net.channel >= 36) count++;
  }
  return count;
}

int countEnterpriseNetworks() {
  int count = 0;
  for (const auto& net : networks) {
    if (net.isEnterprise) count++;
  }
  return count;
}

String getCurrentTime() {
  unsigned long uptime = millis() / 1000;
  unsigned long hours = uptime / 3600;
  unsigned long minutes = (uptime % 3600) / 60;
  unsigned long seconds = uptime % 60;
  
  return String(hours) + ":" + 
         (minutes < 10 ? "0" : "") + String(minutes) + ":" + 
         (seconds < 10 ? "0" : "") + String(seconds);
}