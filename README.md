# ESP32WiFiEnterprise Library

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![ESP32](https://img.shields.io/badge/ESP32-Compatible-green.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Version](https://img.shields.io/badge/Version-1.0.0-orange.svg)](https://github.com/RacoonX65/WiFiEnterprise/releases)

A lightweight, easy-to-use Arduino-style library that allows ESP32 devices to connect to **WPA2-Enterprise networks** (EAP-PEAP) using only SSID, username, and password — similar to connecting to a normal Wi-Fi network.

## ✨ Features

- 🚀 **Simple Arduino-style API** - Just call `WiFiEnterprise.begin(ssid, username, password)`
- 🔐 **WPA2-Enterprise Support** - EAP-PEAP authentication
- 🐛 **Debug Logging** - Optional serial output for troubleshooting
- 📦 **Minimal Dependencies** - Pure ESP32/Arduino library
- 🔄 **Connection Management** - Built-in status checking and reconnection
- 💡 **Easy Integration** - Drop-in replacement for standard WiFi connections

## 🎯 Version 1.0.0 Goals

| Feature | Status | Notes |
|---------|-----------|-------|
| WPA2-Enterprise (EAP-PEAP) | ✅ | Username + Password login |
| Certificate support | ❌ | Planned for future version |
| Arduino-style API | ✅ | Simple `.begin()` function |
| Debug logging | ✅ | Optional serial output |
| Minimal dependencies | ✅ | Pure ESP32/Arduino library |
| ESP32 Compatible | ✅ | ESP32 (Arduino core) only |

## 📋 Requirements

- **Hardware**: ESP32 development board
- **Software**: Arduino IDE with ESP32 board package
- **Network**: WPA2-Enterprise network with EAP-PEAP support

## 📦 Installation

### Method 1: Arduino Library Manager (Recommended)

1. Open Arduino IDE
2. Go to **Sketch** → **Include Library** → **Manage Libraries**
3. Search for "ESP32WiFiEnterprise"
4. Click **Install**

### Method 2: Manual Installation

1. Download the latest release from [GitHub](https://github.com/RacoonX65/WiFiEnterprise/releases)
2. Extract the ZIP file
3. Copy the `ESP32WiFiEnterprise` folder to your Arduino libraries directory:
   - **Windows**: `Documents\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`
4. Restart Arduino IDE

## 🚀 Quick Start

```cpp
#include <WiFiEnterprise.h>

const char* ssid = "YourEnterpriseNetwork";
const char* username = "your_username";
const char* password = "your_password";

void setup() {
  Serial.begin(115200);
  
  // Connect to WPA2-Enterprise network
  if (WiFiEnterprise.begin(ssid, username, password, true)) {
    Serial.println("Connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFiEnterprise.localIP());
  } else {
    Serial.println("Connection failed!");
  }
}

void loop() {
  // Your code here
}
```

## 📚 API Reference

### WiFiEnterpriseClass

#### `begin(ssid, username, password, enableDebug)`
Initialize and connect to WPA2-Enterprise network.

**Parameters:**
- `ssid` (const char*): Network SSID
- `username` (const char*): EAP username
- `password` (const char*): EAP password
- `enableDebug` (bool, optional): Enable debug logging (default: false)

**Returns:** `bool` - true if connection successful, false otherwise

```cpp
// Basic connection
WiFiEnterprise.begin("MyNetwork", "user", "pass");

// With debug enabled
WiFiEnterprise.begin("MyNetwork", "user", "pass", true);
```

#### `end()`
Disconnect from the network.

```cpp
WiFiEnterprise.end();
```

#### `isConnected()`
Check if connected to network.

**Returns:** `bool` - true if connected, false otherwise

```cpp
if (WiFiEnterprise.isConnected()) {
  Serial.println("Still connected!");
}
```

#### `status()`
Get connection status.

**Returns:** `wl_status_t` - WiFi status code

```cpp
wl_status_t status = WiFiEnterprise.status();
```

#### `localIP()`
Get local IP address.

**Returns:** `IPAddress` - Current IP address

```cpp
IPAddress ip = WiFiEnterprise.localIP();
Serial.println(ip);
```

#### `setDebug(enable)`
Enable or disable debug logging.

**Parameters:**
- `enable` (bool): true to enable, false to disable

```cpp
WiFiEnterprise.setDebug(true);  // Enable debug
WiFiEnterprise.setDebug(false); // Disable debug
```

## 📖 Examples

### Basic Connection
Simple connection to WPA2-Enterprise network with status monitoring.

### Advanced Connection
Demonstrates advanced features:
- Connection monitoring
- Automatic reconnection
- LED status indicator
- Error handling
- Detailed logging

## 🔧 Troubleshooting

### Common Issues

**Connection fails immediately:**
- Verify SSID, username, and password are correct
- Ensure the network supports EAP-PEAP authentication
- Check if ESP32 is within range of the access point

**Connection succeeds but no internet:**
- Network may require additional authentication steps
- Check with your network administrator

**Frequent disconnections:**
- Poor signal strength - move closer to access point
- Network configuration issues
- Power supply problems

### Debug Output

Enable debug logging to see detailed connection information:

```cpp
WiFiEnterprise.begin(ssid, username, password, true);
```

Example debug output:
```
WiFiEnterprise: Starting connection to WPA2-Enterprise network
SSID: MyEnterpriseNetwork
Username: john.doe
WiFiEnterprise: Configuring WPA2-Enterprise settings
WiFiEnterprise: Attempting to connect...
......................
WiFiEnterprise: Connected successfully!
IP Address: 192.168.1.100
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👨‍💻 Author

**Judas Sithole**  
📧 Email: judassithle@duck.com

## 🙏 Acknowledgments

- ESP32 Arduino Core team for the excellent WiFi libraries
- Arduino community for inspiration and support

## 📞 Support

If you encounter any issues or have questions:

1. Check the [troubleshooting section](#-troubleshooting)
2. Look at the [examples](examples/)
3. Open an [issue on GitHub](https://github.com/RacoonX65/WiFiEnterprise/issues)

---

**Made with ❤️ for the ESP32 community**