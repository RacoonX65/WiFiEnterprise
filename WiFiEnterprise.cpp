#include "WiFiEnterprise.h"

// Global instance
WiFiEnterpriseClass WiFiEnterprise;

bool WiFiEnterpriseClass::begin(const char* ssid, const char* username, const char* password, bool enableDebug) {
    _debugEnabled = enableDebug;
    
    debugPrint("WiFiEnterprise: Starting connection to WPA2-Enterprise network");
    debugPrint("SSID: ", ssid);
    debugPrint("Username: ", username);
    
    // Disconnect any existing connection
    WiFi.disconnect(true);
    delay(1000);
    
    // Set WiFi mode to station
    WiFi.mode(WIFI_STA);
    
    debugPrint("WiFiEnterprise: Configuring WPA2-Enterprise settings");
    
    // Configure WPA2-Enterprise
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
    
    // Enable WPA2-Enterprise
    esp_wifi_sta_wpa2_ent_enable();
    
    debugPrint("WiFiEnterprise: Attempting to connect...");
    
    // Begin connection
    WiFi.begin(ssid);
    
    // Wait for connection with timeout
    unsigned long startTime = millis();
    const unsigned long timeout = 20000; // 20 seconds timeout
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
        delay(500);
        if (_debugEnabled) {
            Serial.print(".");
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        _connected = true;
        debugPrint("\nWiFiEnterprise: Connected successfully!");
        debugPrint("IP Address: ", WiFi.localIP().toString().c_str());
        return true;
    } else {
        _connected = false;
        debugPrint("\nWiFiEnterprise: Connection failed!");
        debugPrint("Status: ", String(WiFi.status()).c_str());
        
        // Disable WPA2-Enterprise on failure
        esp_wifi_sta_wpa2_ent_disable();
        return false;
    }
}

void WiFiEnterpriseClass::end() {
    debugPrint("WiFiEnterprise: Disconnecting...");
    
    // Disable WPA2-Enterprise
    esp_wifi_sta_wpa2_ent_disable();
    
    // Disconnect WiFi
    WiFi.disconnect(true);
    
    _connected = false;
    debugPrint("WiFiEnterprise: Disconnected");
}

bool WiFiEnterpriseClass::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

wl_status_t WiFiEnterpriseClass::status() {
    return WiFi.status();
}

IPAddress WiFiEnterpriseClass::localIP() {
    return WiFi.localIP();
}

void WiFiEnterpriseClass::setDebug(bool enable) {
    _debugEnabled = enable;
}

void WiFiEnterpriseClass::debugPrint(const char* message) {
    if (_debugEnabled) {
        Serial.println(message);
    }
}

void WiFiEnterpriseClass::debugPrint(const char* message, const char* value) {
    if (_debugEnabled) {
        Serial.print(message);
        Serial.println(value);
    }
}