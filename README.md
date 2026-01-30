# ESP32 WiFi Provisioner

A production-ready WiFi provisioning and recovery library for ESP32 devices. Designed for real IoT products with robust error handling, multiple reset mechanisms, and configurable UX features.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)

## Features

### Core Functionality
- ✅ **Non-blocking operation** - Fully asynchronous state machine
- ✅ **Captive portal** - Responsive web interface for configuration
- ✅ **Persistent storage** - Credentials stored in NVS (Non-Volatile Storage)
- ✅ **Automatic retry** - Configurable retry logic with exponential backoff option
- ✅ **WiFi network scanning** - List available networks in configuration UI
- ✅ **State callbacks** - Hooks for application integration

### Reset Mechanisms
- 🔄 **Hardware button** - Long-press physical button to reset
- 🔄 **HTTP endpoint** - Simple or authenticated remote reset
- 🔄 **Double-reboot detection** - Emergency recovery mechanism
- 🔄 **Auto-wipe on max retries** - Optional credential clearing

### UX Enhancements
- 💡 **LED status indicator** - Visual feedback for device state
- 🌐 **mDNS support** - Access device via `http://device-name.local`
- 📱 **Mobile-friendly UI** - Responsive design works on all devices
- ⚡ **Fast configuration** - Network scanning and one-click selection

### Security Features
- 🔐 **Password hashing** - SHA-256 hashing for reset passwords
- 🔐 **Authenticated reset** - Require password for remote operations
- 🔐 **Configurable AP security** - Open or password-protected configuration AP
- 🔐 **Credential isolation** - Separate storage for WiFi and reset passwords

## Installation

### Arduino IDE
1. Download this repository as ZIP
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file

### PlatformIO
Add to `platformio.ini`:
```ini
lib_deps =
    https://github.com/ferdiu/ESP32ProvisionToolkit.git
```

## Quick Start

```cpp
#include <ESP32ProvisionToolkit.h>

ESP32ProvisionToolkit provisioner;

void setup() {
  Serial.begin(115200);

  provisioner
    .setAPName("MyDevice")  // Custom AP name
    .setLed(2)              // Enable LED on GPIO 2
    .onConnected([]() {
      Serial.println("Connected!");
    })
    .begin();
}

void loop() {
  provisioner.loop();

  // Your application code here
  if (provisioner.isConnected()) {
    // Do something
  }
}
```

## Configuration API

### Fluent Interface

The library uses a fluent API for easy configuration:

```cpp
provisioner
  .setAPName("MyESP32")
  .setAPPassword("config123")
  .setMaxRetries(10)
  .setRetryDelay(3000)
  .enableHardwareReset(GPIO_NUM_0, 5000)
  .enableAuthenticatedHttpReset(true)
  .setLed(GPIO_NUM_2)
  .enableMDNS("my-device")
  .setLogLevel(LOG_INFO)
  .onConnected(onConnectedCallback)
  .begin();
```

### Configuration Methods

#### Access Point Settings

| Method | Parameters | Description |
|--------|-----------|-------------|
| `setAPName(name)` | String | AP SSID base name (MAC suffix added) |
| `setAPPassword(password)` | String | AP password (empty = open network) |
| `setAPTimeout(ms)` | uint32_t | Timeout before exiting AP mode |

#### Connection Settings

| Method | Parameters | Description |
|--------|-----------|-------------|
| `setMaxRetries(count)` | uint8_t | Max connection attempts before action |
| `setRetryDelay(ms)` | uint32_t | Delay between retry attempts |
| `setAutoWipeOnMaxRetries(enable)` | bool | Clear credentials after max retries |

#### Hardware Reset

| Method | Parameters | Description |
|--------|-----------|-------------|
| `enableHardwareReset(pin, duration, activeLow)` | int8_t, uint32_t, bool | Enable button reset |
| `disableHardwareReset()` | - | Disable button reset |

**Example:**
```cpp
// BOOT button (GPIO 0), hold for 5 seconds, active low
provisioner.enableHardwareReset(0, 5000, true);
```

#### Software Reset

| Method | Parameters | Description |
|--------|-----------|-------------|
| `enableHttpReset(enable)` | bool | Enable simple HTTP reset |
| `enableAuthenticatedHttpReset(enable)` | bool | Enable password-protected reset |

**HTTP Reset Usage:**

Simple (unauthenticated):
```bash
curl -X POST http://device-ip/reset
```

Authenticated:
```bash
curl -X POST http://device-ip/reset -d "password=YOUR_RESET_PASSWORD"
```

> **NOTE:** If any time you reset the software you change this password it can still be considered secure to send the password in plain-text.

#### UX Features

| Method | Parameters | Description |
|--------|-----------|-------------|
| `setLed(pin, activeLow)` | int8_t, bool | Enable LED status indicator |
| `enableMDNS(name)` | String | Enable mDNS responder |
| `enableDoubleRebootDetect(windowMs)` | uint32_t | Enable double-reboot reset |

**LED Patterns:**
- **Fast blink (100ms on/off)**: Provisioning mode
- **Slow blink (100ms on, 900ms off)**: Connecting
- **Solid on**: Connected
- **Off**: Idle/Error

#### Logging

| Method | Parameters | Description |
|--------|-----------|-------------|
| `setLogLevel(level)` | LogLevel | Set verbosity level |

**Log Levels:**
- `LOG_NONE` - No output
- `LOG_ERROR` - Errors only
- `LOG_INFO` - Normal operation info
- `LOG_DEBUG` - Detailed debugging

#### Callbacks

| Method | Parameters | Description |
|--------|-----------|-------------|
| `onConnected(callback)` | void (*callback)() | Called when WiFi connects |
| `onFailed(callback)` | void (*callback)(uint8_t) | Called on connection failure |
| `onAPMode(callback)` | void (*callback)(const char*, const char*) | Called when AP mode starts |
| `onReset(callback)` | void (*callback)() | Called before device reset |

**Example:**
```cpp
provisioner.onConnected([]() {
  Serial.println("Connected to WiFi!");
  Serial.println(WiFi.localIP());
});

provisioner.onFailed([](uint8_t retryCount) {
  Serial.printf("Connection failed, retry %d\n", retryCount);
});

provisioner.onAPMode([](const char* ssid, const char* ip) {
  Serial.printf("AP Mode: %s at %s\n", ssid, ip);
});
```

## Status Query Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `isConnected()` | bool | True if WiFi connected |
| `isProvisioning()` | bool | True if in AP provisioning mode |
| `getState()` | ProvisionerState | Current state machine state |
| `getSSID()` | String | Connected SSID |
| `getLocalIP()` | IPAddress | Device IP address |
| `getAPIP()` | String | AP mode IP address |

## Manual Control Methods

| Method | Parameters | Description |
|--------|-----------|-------------|
| `setCredentials(ssid, password, reboot)` | String, String, bool | Set WiFi credentials |
| `clearCredentials(reboot)` | bool | Clear stored credentials |
| `reset()` | - | Trigger programmatic reset |

**Example:**
```cpp
// Programmatically set credentials
provisioner.setCredentials("MyNetwork", "password123", true);

// Clear and reboot
provisioner.clearCredentials(true);

// Trigger reset
provisioner.reset();
```

## Architecture

### State Machine

```
┌─────────────┐
│    INIT     │
└──────┬──────┘
       │
       v
┌─────────────┐      No Credentials      ┌──────────────┐
│ LOAD_CONFIG │─────────────────────────>│ PROVISIONING │
└──────┬──────┘                          └───────┬──────┘
       │                                         │
       │ Has Credentials                         │
       v                                         │
┌─────────────┐      Success         ┌───────────v─────────┐
│ CONNECTING  │─────────────────────>│ PROVISIONING_ACTIVE │
└──────┬──────┘                      └─────────────────────┘
       │
       │ Failure
       v
┌─────────────┐      Max Retries     ┌──────────────┐
│ RETRY_WAIT  │─────────────────────>│  WIPE & AP   │
└──────┬──────┘                      └──────────────┘
       │
       │ Retry
       v
┌─────────────┐
│  CONNECTED  │◄─── Main operation state
└─────────────┘
```

### Storage Layout (NVS)

Namespace: `wifiprov`

| Key | Type | Description |
|-----|------|-------------|
| `ssid` | String | WiFi SSID |
| `password` | String | WiFi password |
| `reset_pwd` | String | SHA-256 hash of reset password |
| `boot_count` | uint32_t | Boot counter for double-reboot |
| `boot_time` | uint32_t | Last boot timestamp |

## Security Considerations

### Production Deployment

1. **Always use authenticated reset** in untrusted environments:
   ```cpp
   provisioner.enableAuthenticatedHttpReset(true);
   ```

2. **Secure the configuration AP** with a strong password:
   ```cpp
   provisioner.setAPPassword("Strong!Pass123");
   ```

3. **Use hardware reset** for field deployment:
   ```cpp
   provisioner.enableHardwareReset(BUTTON_PIN, 10000); // 10 sec
   ```

4. **Disable debug logging** in production:
   ```cpp
   provisioner.setLogLevel(LOG_ERROR);
   ```

### Password Security

- Reset passwords are hashed using SHA-256 before storage
- Original passwords are never stored in plaintext
- Hash verification is constant-time to prevent timing attacks

### Network Security

- The library does not implement WPA3 or certificate pinning
- Use WPA2-PSK or better for production networks
- Consider implementing additional application-level encryption
- For highly sensitive applications, use VPN or TLS for all communications

## Troubleshooting

### Device doesn't enter provisioning mode

**Causes:**
- Credentials are stored from previous session
- Double-reboot window not met

**Solutions:**
```cpp
// Check if credentials exist
if (!provisioner.isProvisioning()) {
  // Manually trigger reset
  provisioner.clearCredentials(true);
}

// Or use hardware button
// Hold BOOT button for configured duration
```

### Cannot connect to configured network

**Causes:**
- Wrong password
- Network out of range
- Router MAC filtering

**Solutions:**
- Check router logs
- Verify SSID and password
- Increase max retries:
  ```cpp
  provisioner.setMaxRetries(20);
  ```
- Enable auto-wipe to re-enter provisioning:
  ```cpp
  provisioner.setAutoWipeOnMaxRetries(true);
  ```

### Captive portal doesn't open automatically

**Causes:**
- Mobile device doesn't support captive portal detection
- DNS server not starting properly

**Solutions:**
- Manually navigate to AP IP address (usually `192.168.4.1`)
- Check Serial output for DNS server status
- Ensure DNS server is enabled in your network settings

### HTTP reset returns 403

**Causes:**
- Reset feature not enabled
- Using authenticated endpoint without password

**Solutions:**
```cpp
// Enable simple reset
provisioner.enableHttpReset(true);

// OR enable authenticated reset
provisioner.enableAuthenticatedHttpReset(true);
// Then set password during configuration
```

### LED doesn't blink

**Causes:**
- LED not enabled
- Wrong GPIO pin
- Active-low setting incorrect

**Solutions:**
```cpp
// For most ESP32 boards (active-low built-in LED)
provisioner.setLed(2, true); // GPIO 2, active low

// For external LED (active-high)
provisioner.setLed(LED_PIN, false);
```

## Advanced Usage

### Custom NVS Namespace

The library uses the `wifiprov` namespace by default. To avoid conflicts:

```cpp
// Currently not configurable, but you can modify the source:
// In ESP32ProvisionToolkit.cpp:
// #define NVS_NAMESPACE "myapp_wifi"
```

### Integration with MQTT

```cpp
#include <ESP32ProvisionToolkit.h>
#include <PubSubClient.h>

ESP32ProvisionToolkit provisioner;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup() {
  provisioner
    .onConnected([]() {
      // Connect to MQTT when WiFi connects
      mqtt.setServer("mqtt.example.com", 1883);
      mqtt.connect("esp32-client");
    })
    .begin();
}

void loop() {
  provisioner.loop();

  if (provisioner.isConnected()) {
    mqtt.loop();
  }
}
```

### OTA Updates Integration

```cpp
#include <ESP32ProvisionToolkit.h>
#include <ArduinoOTA.h>

void setup() {
  provisioner
    .onConnected([]() {
      ArduinoOTA.begin();
    })
    .begin();
}

void loop() {
  provisioner.loop();

  if (provisioner.isConnected()) {
    ArduinoOTA.handle();
  }
}
```

### Power Management

```cpp
void loop() {
  provisioner.loop();

  if (provisioner.isConnected()) {
    // Do work
    sendSensorData();

    // Sleep until next transmission
    esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 seconds
    esp_light_sleep_start();
  }
}
```

## Examples

The library includes four complete examples:

1. **Basic** - Minimal setup for getting started
2. **SecureReset** - All security features enabled
3. **Headless** - Production IoT device configuration
4. **CustomRoutes** - The basic example with additional user-defined routes

See `/examples` directory for complete code.

## API Reference

See the header file `ESP32ProvisionToolkit.h` for complete API documentation with detailed comments.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests if applicable
4. Submit a pull request

## License

MIT License - See LICENSE file for details

## Support

- **Issues**: [GitHub Issues](https://github.com/ferdiu/ESP32ProvisionToolkit/issues)

## Changelog

### Version 1.0.0 (2026-01-30)
- Initial release
- Complete WiFi provisioning system
- Multiple reset mechanisms
- Production-ready features
- Comprehensive documentation

## Credits

Developed with ❤️ for the ESP32 community

## Acknowledgments

- Espressif for the ESP32 platform
- Arduino community for the excellent ecosystem
