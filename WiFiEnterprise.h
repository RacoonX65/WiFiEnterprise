#ifndef WIFI_ENTERPRISE_H
#define WIFI_ENTERPRISE_H

#include <WiFi.h>
#include <esp_wpa2.h>
#include <esp_wifi.h>

/**
 * WiFiEnterprise - A lightweight Arduino-style library for ESP32
 * to connect to WPA2-Enterprise networks using EAP-PEAP authentication
 * 
 * Version: 1.0.0
 * Author: Judas Sithole <judassithle@duck.com>
 * License: MIT
 */

class WiFiEnterpriseClass {
public:
    /**
     * Initialize and connect to WPA2-Enterprise network
     * @param ssid Network SSID
     * @param username EAP username
     * @param password EAP password
     * @param enableDebug Enable debug logging (default: false)
     * @return true if connection successful, false otherwise
     */
    bool begin(const char* ssid, const char* username, const char* password, bool enableDebug = false);
    
    /**
     * Disconnect from the network
     */
    void end();
    
    /**
     * Check if connected to network
     * @return true if connected, false otherwise
     */
    bool isConnected();
    
    /**
     * Get connection status
     * @return WiFi status
     */
    wl_status_t status();
    
    /**
     * Get local IP address
     * @return IPAddress object
     */
    IPAddress localIP();
    
    /**
     * Enable or disable debug logging
     * @param enable true to enable, false to disable
     */
    void setDebug(bool enable);
    
private:
    bool _debugEnabled = false;
    bool _connected = false;
    
    /**
     * Print debug message if debug is enabled
     * @param message Debug message to print
     */
    void debugPrint(const char* message);
    
    /**
     * Print debug message with value if debug is enabled
     * @param message Debug message to print
     * @param value Value to print
     */
    void debugPrint(const char* message, const char* value);
};

// Global instance
extern WiFiEnterpriseClass WiFiEnterprise;

#endif // WIFI_ENTERPRISE_H