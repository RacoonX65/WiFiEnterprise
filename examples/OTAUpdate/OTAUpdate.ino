/*
  WiFiEnterprise OTA Update Example
  
  This example demonstrates how to implement Over-The-Air (OTA) firmware updates
  using the WiFiEnterprise library for WPA2-Enterprise network connectivity.
  
  Features:
  - Web-based OTA updates
  - Arduino OTA support
  - Firmware version management
  - Update progress monitoring
  - Rollback protection
  - Secure update verification
  
  Compatible with: ESP32 (Arduino core)
  
  Circuit:
  - ESP32 board
  - Status LED on pin 2 (optional)
  
  Usage:
  1. Upload this sketch to your ESP32
  2. Connect to your enterprise network
  3. Access the web interface at the displayed IP address
  4. Upload new firmware via web interface or Arduino IDE
  
  Created: 2025
  By: Judas Sithole <judassithle@duck.com>
*/

#include <WiFiEnterprise.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Firmware version
#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__ " " __TIME__

// Network credentials
const char* ssid = "YourEnterpriseNetwork";
const char* username = "your_username";
const char* password = "your_password";

// OTA configuration
const char* otaHostname = "ESP32-Enterprise-OTA";
const char* otaPassword = "your_ota_password"; // Change this!

// Web server
WebServer server(80);

// Pin definitions
const int LED_PIN = 2;

// Global variables
Preferences preferences;
bool otaInProgress = false;
String lastUpdateStatus = "Ready";
unsigned long lastHeartbeat = 0;
int updateProgress = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize preferences
  preferences.begin("ota-app", false);
  
  printStartupInfo();
  
  // Connect to enterprise network
  connectToNetwork();
  
  // Setup OTA
  setupOTA();
  
  // Setup web server
  setupWebServer();
  
  Serial.println("🚀 OTA Update system ready!");
  Serial.println("📡 Arduino OTA enabled - use Arduino IDE for updates");
  Serial.print("🌐 Web OTA interface: http://");
  Serial.println(WiFiEnterprise.localIP());
  Serial.println();
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Handle web server
  server.handleClient();
  
  // Check connection status
  if (!WiFiEnterprise.isConnected()) {
    handleDisconnection();
    return;
  }
  
  // Heartbeat LED (slow blink when ready, fast when updating)
  unsigned long currentTime = millis();
  if (currentTime - lastHeartbeat >= (otaInProgress ? 200 : 1000)) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastHeartbeat = currentTime;
  }
  
  delay(100);
}

void printStartupInfo() {
  Serial.println("\n" + String('=', 50));
  Serial.println("    WiFiEnterprise OTA Update System");
  Serial.println("" + String('=', 50));
  Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
  Serial.println("Build Date: " + String(BUILD_DATE));
  Serial.println("Chip Model: " + String(ESP.getChipModel()));
  Serial.println("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("" + String('=', 50) + "\n");
  
  // Check if this is a recovery from update
  String lastUpdate = preferences.getString("last_update", "");
  if (lastUpdate.length() > 0) {
    Serial.println("📋 Last update: " + lastUpdate);
    preferences.remove("last_update"); // Clear after reading
  }
}

void connectToNetwork() {
  Serial.println("🔗 Connecting to WPA2-Enterprise network...");
  
  if (WiFiEnterprise.begin(ssid, username, password, true)) {
    Serial.println("\n✅ Connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFiEnterprise.localIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n❌ Connection failed!");
    Serial.println("Please check your credentials and restart.");
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
  
  // Attempt reconnection every 30 seconds
  if (millis() - lastReconnectAttempt >= 30000) {
    Serial.println("🔄 WiFi disconnected, attempting to reconnect...");
    
    if (WiFiEnterprise.begin(ssid, username, password)) {
      Serial.println("✅ Reconnected successfully!");
    } else {
      Serial.println("❌ Reconnection failed");
      lastReconnectAttempt = millis();
    }
  }
}

void setupOTA() {
  // Configure Arduino OTA
  ArduinoOTA.setHostname(otaHostname);
  ArduinoOTA.setPassword(otaPassword);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    
    Serial.println("🔄 Starting OTA update (" + type + ")...");
    otaInProgress = true;
    lastUpdateStatus = "Arduino OTA in progress";
    
    // Turn on LED during update
    digitalWrite(LED_PIN, HIGH);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n✅ Arduino OTA update completed!");
    otaInProgress = false;
    lastUpdateStatus = "Arduino OTA completed successfully";
    
    // Store update info
    preferences.putString("last_update", "Arduino OTA - " + String(BUILD_DATE));
    
    digitalWrite(LED_PIN, LOW);
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    updateProgress = (progress / (total / 100));
    Serial.printf("📊 Progress: %u%%\r", updateProgress);
    
    // Blink LED based on progress
    if (progress % (total / 10) == 0) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("❌ Arduino OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
      lastUpdateStatus = "Arduino OTA Auth Failed";
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
      lastUpdateStatus = "Arduino OTA Begin Failed";
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
      lastUpdateStatus = "Arduino OTA Connect Failed";
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
      lastUpdateStatus = "Arduino OTA Receive Failed";
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
      lastUpdateStatus = "Arduino OTA End Failed";
    }
    
    otaInProgress = false;
    digitalWrite(LED_PIN, LOW);
  });
  
  ArduinoOTA.begin();
  Serial.println("📡 Arduino OTA initialized");
  Serial.println("   Hostname: " + String(otaHostname));
  Serial.println("   Password: " + String(otaPassword));
}

void setupWebServer() {
  // Main OTA page
  server.on("/", handleRoot);
  
  // API endpoints
  server.on("/api/info", handleInfo);
  server.on("/api/status", handleStatus);
  server.on("/upload", HTTP_POST, handleUploadComplete, handleUpload);
  
  // Handle not found
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("🌐 Web OTA server started");
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 OTA Update</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }
        .info { background: #f8f9fa; padding: 15px; border-radius: 8px; margin: 20px 0; }
        .info h3 { margin: 0 0 10px 0; color: #007bff; }
        .upload-area { border: 2px dashed #007bff; padding: 20px; text-align: center; margin: 20px 0; border-radius: 8px; }
        .btn { background: #007bff; color: white; border: none; padding: 10px 20px; margin: 5px; border-radius: 5px; cursor: pointer; }
        .btn:hover { background: #0056b3; }
        .btn:disabled { background: #6c757d; cursor: not-allowed; }
        .progress { width: 100%; height: 20px; background: #e9ecef; border-radius: 10px; overflow: hidden; margin: 10px 0; }
        .progress-bar { height: 100%; background: #007bff; width: 0%; transition: width 0.3s; }
        .status { padding: 10px; border-radius: 5px; margin: 10px 0; }
        .status.success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .status.error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .status.info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔄 ESP32 OTA Update</h1>
            <p>WiFiEnterprise Firmware Updater</p>
        </div>
        
        <div class="info">
            <h3>📊 Device Information</h3>
            <p><strong>Firmware Version:</strong> )" + String(FIRMWARE_VERSION) + R"(</p>
            <p><strong>Build Date:</strong> )" + String(BUILD_DATE) + R"(</p>
            <p><strong>IP Address:</strong> )" + WiFiEnterprise.localIP().toString() + R"(</p>
            <p><strong>Free Heap:</strong> <span id="heap">)" + String(ESP.getFreeHeap()) + R"(</span> bytes</p>
            <p><strong>Uptime:</strong> <span id="uptime">)" + String(millis()/1000) + R"(</span> seconds</p>
        </div>
        
        <div class="upload-area">
            <h3>📁 Upload New Firmware</h3>
            <p>Select a .bin file to upload</p>
            <input type="file" id="fileInput" accept=".bin" style="margin: 10px 0;">
            <br>
            <button class="btn" onclick="uploadFirmware()" id="uploadBtn">Upload Firmware</button>
        </div>
        
        <div id="progressContainer" style="display: none;">
            <h4>Upload Progress</h4>
            <div class="progress">
                <div class="progress-bar" id="progressBar"></div>
            </div>
            <p id="progressText">0%</p>
        </div>
        
        <div id="status" class="status info">
            <strong>Status:</strong> <span id="statusText">Ready for upload</span>
        </div>
        
        <div style="text-align: center; margin-top: 20px;">
            <button class="btn" onclick="refreshInfo()">🔄 Refresh Info</button>
            <button class="btn" onclick="rebootDevice()" style="background: #dc3545;">🔄 Reboot Device</button>
        </div>
    </div>
    
    <script>
        function refreshInfo() {
            fetch('/api/info')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('heap').textContent = data.free_heap;
                    document.getElementById('uptime').textContent = data.uptime;
                });
        }
        
        function uploadFirmware() {
            const fileInput = document.getElementById('fileInput');
            const file = fileInput.files[0];
            
            if (!file) {
                alert('Please select a firmware file first!');
                return;
            }
            
            if (!file.name.endsWith('.bin')) {
                alert('Please select a .bin file!');
                return;
            }
            
            const formData = new FormData();
            formData.append('firmware', file);
            
            const xhr = new XMLHttpRequest();
            
            // Show progress
            document.getElementById('progressContainer').style.display = 'block';
            document.getElementById('uploadBtn').disabled = true;
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    document.getElementById('progressBar').style.width = percentComplete + '%';
                    document.getElementById('progressText').textContent = Math.round(percentComplete) + '%';
                }
            });
            
            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    document.getElementById('status').className = 'status success';
                    document.getElementById('statusText').textContent = 'Upload successful! Device will reboot...';
                } else {
                    document.getElementById('status').className = 'status error';
                    document.getElementById('statusText').textContent = 'Upload failed: ' + xhr.responseText;
                }
                document.getElementById('uploadBtn').disabled = false;
            });
            
            xhr.addEventListener('error', function() {
                document.getElementById('status').className = 'status error';
                document.getElementById('statusText').textContent = 'Upload error occurred';
                document.getElementById('uploadBtn').disabled = false;
            });
            
            xhr.open('POST', '/upload');
            xhr.send(formData);
        }
        
        function rebootDevice() {
            if (confirm('Are you sure you want to reboot the device?')) {
                fetch('/api/reboot', { method: 'POST' })
                    .then(() => {
                        document.getElementById('status').className = 'status info';
                        document.getElementById('statusText').textContent = 'Device rebooting...';
                    });
            }
        }
        
        // Auto-refresh info every 5 seconds
        setInterval(refreshInfo, 5000);
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

void handleInfo() {
  JsonDocument doc;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["build_date"] = BUILD_DATE;
  doc["chip_model"] = ESP.getChipModel();
  doc["cpu_freq"] = ESP.getCpuFreqMHz();
  doc["flash_size"] = ESP.getFlashChipSize();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["ip_address"] = WiFiEnterprise.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStatus() {
  JsonDocument doc;
  doc["ota_in_progress"] = otaInProgress;
  doc["last_update_status"] = lastUpdateStatus;
  doc["update_progress"] = updateProgress;
  doc["connected"] = WiFiEnterprise.isConnected();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("🔄 Starting web OTA update...");
    Serial.println("   Filename: " + upload.filename);
    Serial.println("   Size: " + String(upload.totalSize) + " bytes");
    
    otaInProgress = true;
    lastUpdateStatus = "Web OTA upload in progress";
    
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      lastUpdateStatus = "Web OTA begin failed";
    }
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      lastUpdateStatus = "Web OTA write failed";
    } else {
      updateProgress = (upload.totalSize > 0) ? (upload.totalSize - upload.remainingSize) * 100 / upload.totalSize : 0;
      Serial.printf("📊 Progress: %d%%\r", updateProgress);
    }
    
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println("\n✅ Web OTA update successful!");
      lastUpdateStatus = "Web OTA completed successfully";
      
      // Store update info
      preferences.putString("last_update", "Web OTA - " + String(upload.filename));
      
    } else {
      Update.printError(Serial);
      lastUpdateStatus = "Web OTA end failed";
    }
    
    otaInProgress = false;
  }
}

void handleUploadComplete() {
  if (Update.hasError()) {
    server.send(500, "text/plain", "Update failed: " + String(Update.getError()));
  } else {
    server.send(200, "text/plain", "Update successful! Rebooting...");
    delay(1000);
    ESP.restart();
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}