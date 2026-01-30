# API Reference

Complete API documentation for ESP32_WiFiProvisioner library.

## Table of Contents

1. [Class Overview](#class-overview)
2. [Configuration Methods](#configuration-methods)
3. [Control Methods](#control-methods)
4. [Status Methods](#status-methods)
5. [Callback Types](#callback-types)
6. [Enumerations](#enumerations)
7. [Structures](#structures)

## Class Overview

```cpp
class ESP32_WiFiProvisioner
```

Main class providing WiFi provisioning and recovery functionality.

**Header:** `ESP32_WiFiProvisioner.h`

**Namespace:** Global

### Constructor

```cpp
ESP32_WiFiProvisioner()
```

Creates a new provisioner instance. Initializes internal state.

**Example:**
```cpp
ESP32_WiFiProvisioner provisioner;
```

### Destructor

```cpp
~ESP32_WiFiProvisioner()
```

Cleans up allocated resources (DNS server, web server).

## Configuration Methods

All configuration methods return `ESP32_WiFiProvisioner&` for method chaining.

### Access Point Configuration

#### setAPName

```cpp
ESP32_WiFiProvisioner& setAPName(const String& name)
```

Sets the base name for the configuration access point.

**Parameters:**
- `name` - Base SSID (MAC suffix will be appended)

**Returns:** Reference to this instance

**Default:** `"ESP32-Config"`

**Example:**
```cpp
provisioner.setAPName("MyDevice");
// Results in AP name like: "MyDevice-A1B2C3"
```

---

#### setAPPassword

```cpp
ESP32_WiFiProvisioner& setAPPassword(const String& password)
```

Sets password for the configuration access point.

**Parameters:**
- `password` - WiFi password (empty string = open network)

**Returns:** Reference to this instance

**Default:** `""` (open network)

**Security Note:** Use strong passwords in production!

**Example:**
```cpp
provisioner.setAPPassword("ConfigPass123");
```

---

#### setAPTimeout

```cpp
ESP32_WiFiProvisioner& setAPTimeout(uint32_t milliseconds)
```

Sets timeout for AP mode before attempting reconnection.

**Parameters:**
- `milliseconds` - Timeout in milliseconds (0 = no timeout)

**Returns:** Reference to this instance

**Default:** `300000` (5 minutes)

**Example:**
```cpp
provisioner.setAPTimeout(180000); // 3 minutes
```

---

### Connection Configuration

#### setMaxRetries

```cpp
ESP32_WiFiProvisioner& setMaxRetries(uint8_t retries)
```

Sets maximum number of connection retry attempts.

**Parameters:**
- `retries` - Number of attempts (0-255)

**Returns:** Reference to this instance

**Default:** `10`

**Example:**
```cpp
provisioner.setMaxRetries(20); // Try 20 times
```

---

#### setRetryDelay

```cpp
ESP32_WiFiProvisioner& setRetryDelay(uint32_t milliseconds)
```

Sets delay between connection retry attempts.

**Parameters:**
- `milliseconds` - Delay in milliseconds

**Returns:** Reference to this instance

**Default:** `3000` (3 seconds)

**Example:**
```cpp
provisioner.setRetryDelay(5000); // 5 second delay
```

---

#### setAutoWipeOnMaxRetries

```cpp
ESP32_WiFiProvisioner& setAutoWipeOnMaxRetries(bool enable)
```

Configures whether to clear credentials after max retries exceeded.

**Parameters:**
- `enable` - `true` to auto-wipe, `false` to keep retrying

**Returns:** Reference to this instance

**Default:** `true`

**Example:**
```cpp
provisioner.setAutoWipeOnMaxRetries(false); // Keep credentials
```

---

### Hardware Reset Configuration

#### enableHardwareReset

```cpp
ESP32_WiFiProvisioner& enableHardwareReset(
    int8_t pin, 
    uint32_t durationMs = 5000, 
    bool activeLow = true
)
```

Enables hardware button reset mechanism.

**Parameters:**
- `pin` - GPIO pin number for reset button
- `durationMs` - Required button hold duration in milliseconds
- `activeLow` - `true` for pull-up (button connects to GND), `false` for pull-down

**Returns:** Reference to this instance

**Default:** Disabled

**Note:** Automatically configures pin as INPUT or INPUT_PULLUP.

**Example:**
```cpp
// BOOT button (GPIO 0), 5 second hold, active low
provisioner.enableHardwareReset(0, 5000, true);
```

---

#### disableHardwareReset

```cpp
ESP32_WiFiProvisioner& disableHardwareReset()
```

Disables hardware button reset.

**Returns:** Reference to this instance

**Example:**
```cpp
provisioner.disableHardwareReset();
```

---

### Software Reset Configuration

#### enableHttpReset

```cpp
ESP32_WiFiProvisioner& enableHttpReset(bool enable)
```

Enables simple unauthenticated HTTP reset.

**Parameters:**
- `enable` - `true` to enable, `false` to disable

**Returns:** Reference to this instance

**Default:** Disabled

**Security Warning:** Only use on trusted networks!

**HTTP Endpoint:** `POST /reset`

**Example:**
```cpp
provisioner.enableHttpReset(true);
```

**Usage:**
```bash
curl -X POST http://192.168.4.1/reset
```

---

#### enableAuthenticatedHttpReset

```cpp
ESP32_WiFiProvisioner& enableAuthenticatedHttpReset(bool enable)
```

Enables password-protected HTTP reset.

**Parameters:**
- `enable` - `true` to enable with authentication, `false` to disable

**Returns:** Reference to this instance

**Default:** Disabled

**Note:** Password must be set during provisioning via web interface.

**HTTP Endpoint:** `POST /reset` with `password` parameter

**Example:**
```cpp
provisioner.enableAuthenticatedHttpReset(true);
```

**Usage:**
```bash
curl -X POST http://device-ip/reset -d "password=YOUR_PASSWORD"
```

---

### UX Configuration

#### setLed

```cpp
ESP32_WiFiProvisioner& setLed(int8_t pin, bool activeLow = false)
```

Enables LED status indicator.

**Parameters:**
- `pin` - GPIO pin number for LED
- `activeLow` - `true` if LED is active-low (common cathode)

**Returns:** Reference to this instance

**Default:** Disabled

**LED Patterns:**
- Fast blink (100ms): Provisioning mode
- Slow blink (100ms on, 900ms off): Connecting
- Solid on: Connected

**Example:**
```cpp
// Built-in LED on GPIO 2 (active low on most ESP32 boards)
provisioner.setLed(2, true);
```

---

#### enableMDNS

```cpp
ESP32_WiFiProvisioner& enableMDNS(const String& name)
```

Enables mDNS responder for easy device discovery.

**Parameters:**
- `name` - mDNS hostname (will respond to `name.local`)

**Returns:** Reference to this instance

**Default:** Disabled

**Note:** Only active when connected to WiFi.

**Example:**
```cpp
provisioner.enableMDNS("my-esp32");
// Device accessible at: http://my-esp32.local
```

---

#### enableDoubleRebootDetect

```cpp
ESP32_WiFiProvisioner& enableDoubleRebootDetect(
    uint32_t windowMs = 10000
)
```

Enables double-reboot detection for emergency reset.

**Parameters:**
- `windowMs` - Time window for detecting double reboot (milliseconds)

**Returns:** Reference to this instance

**Default:** Disabled

**Usage:** Reboot device twice within the time window to clear credentials.

**Example:**
```cpp
provisioner.enableDoubleRebootDetect(10000); // 10 second window
```

---

### Logging Configuration

#### setLogLevel

```cpp
ESP32_WiFiProvisioner& setLogLevel(LogLevel level)
```

Sets logging verbosity level.

**Parameters:**
- `level` - Log level (see [LogLevel enum](#loglevel))

**Returns:** Reference to this instance

**Default:** `LOG_INFO`

**Example:**
```cpp
#ifdef DEBUG
    provisioner.setLogLevel(LOG_DEBUG);
#else
    provisioner.setLogLevel(LOG_ERROR);
#endif
```

---

### Callback Configuration

#### onConnected

```cpp
ESP32_WiFiProvisioner& onConnected(WiFiConnectedCallback callback)
```

Sets callback for WiFi connection success.

**Parameters:**
- `callback` - Function pointer `void (*callback)()`

**Returns:** Reference to this instance

**Called when:** WiFi connection is established

**Example:**
```cpp
provisioner.onConnected([]() {
    Serial.println("WiFi Connected!");
});
```

---

#### onFailed

```cpp
ESP32_WiFiProvisioner& onFailed(WiFiFailedCallback callback)
```

Sets callback for WiFi connection failure.

**Parameters:**
- `callback` - Function pointer `void (*callback)(uint8_t retryCount)`

**Returns:** Reference to this instance

**Called when:** Each connection attempt fails

**Example:**
```cpp
provisioner.onFailed([](uint8_t retryCount) {
    Serial.printf("Retry %d failed\n", retryCount);
});
```

---

#### onAPMode

```cpp
ESP32_WiFiProvisioner& onAPMode(APModeCallback callback)
```

Sets callback for AP mode activation.

**Parameters:**
- `callback` - Function pointer `void (*callback)(const char* ssid, const char* ip)`

**Returns:** Reference to this instance

**Called when:** Device enters provisioning mode

**Example:**
```cpp
provisioner.onAPMode([](const char* ssid, const char* ip) {
    Serial.printf("AP: %s at %s\n", ssid, ip);
});
```

---

#### onReset

```cpp
ESP32_WiFiProvisioner& onReset(ResetCallback callback)
```

Sets callback for device reset.

**Parameters:**
- `callback` - Function pointer `void (*callback)()`

**Returns:** Reference to this instance

**Called when:** Reset is triggered (before reboot)

**Example:**
```cpp
provisioner.onReset([]() {
    Serial.println("Resetting device!");
    // Save any pending data
});
```

---

## Control Methods

### begin

```cpp
bool begin()
```

Initializes and starts the provisioner.

**Returns:** `true` on success, `false` on error

**Must be called** in `setup()` after all configuration.

**Example:**
```cpp
void setup() {
    provisioner.setAPName("MyDevice").begin();
}
```

---

### loop

```cpp
void loop()
```

Handles provisioner state machine. **Must be called** in `loop()`.

**Non-blocking** - safe to call frequently.

**Example:**
```cpp
void loop() {
    provisioner.loop();
    // Your code here
}
```

---

### reset

```cpp
void reset()
```

Triggers programmatic device reset.

**Actions:**
1. Calls `onReset` callback (if set)
2. Clears all credentials
3. Reboots device

**Example:**
```cpp
if (factoryResetRequested) {
    provisioner.reset();
}
```

---

## Status Methods

### isConnected

```cpp
bool isConnected() const
```

Checks if device is currently connected to WiFi.

**Returns:** `true` if connected, `false` otherwise

**Example:**
```cpp
if (provisioner.isConnected()) {
    // Safe to use network
    sendData();
}
```

---

### isProvisioning

```cpp
bool isProvisioning() const
```

Checks if device is in provisioning mode.

**Returns:** `true` if in AP provisioning mode, `false` otherwise

**Example:**
```cpp
if (provisioner.isProvisioning()) {
    Serial.println("Waiting for configuration...");
}
```

---

### getState

```cpp
ProvisionerState getState() const
```

Gets current state machine state.

**Returns:** Current state (see [ProvisionerState enum](#provisionerstate))

**Example:**
```cpp
switch (provisioner.getState()) {
    case STATE_CONNECTED:
        // Handle connected state
        break;
    case STATE_PROVISIONING:
        // Handle provisioning state
        break;
}
```

---

### getSSID

```cpp
String getSSID() const
```

Gets the stored WiFi SSID.

**Returns:** SSID string (empty if not configured)

**Example:**
```cpp
Serial.print("Connected to: ");
Serial.println(provisioner.getSSID());
```

---

### getLocalIP

```cpp
IPAddress getLocalIP() const
```

Gets the device's IP address when connected.

**Returns:** IP address (0.0.0.0 if not connected)

**Example:**
```cpp
Serial.print("IP: ");
Serial.println(provisioner.getLocalIP());
```

---

### getAPIP

```cpp
String getAPIP() const
```

Gets the AP mode IP address.

**Returns:** IP address string (typically "192.168.4.1")

**Example:**
```cpp
if (provisioner.isProvisioning()) {
    Serial.printf("Configure at: http://%s\n", 
        provisioner.getAPIP().c_str());
}
```

---

## Manual Control Methods

### setCredentials

```cpp
bool setCredentials(
    const String& ssid, 
    const String& password, 
    bool reboot = true
)
```

Manually sets WiFi credentials.

**Parameters:**
- `ssid` - Network SSID
- `password` - Network password
- `reboot` - Reboot after saving (default: true)

**Returns:** `true` on success, `false` on error

**Example:**
```cpp
// Programmatically configure network
provisioner.setCredentials("MyNetwork", "password123");
```

---

### clearCredentials

```cpp
bool clearCredentials(bool reboot = true)
```

Manually clears stored WiFi credentials.

**Parameters:**
- `reboot` - Reboot after clearing (default: true)

**Returns:** `true` on success

**Example:**
```cpp
// Clear and reboot to provisioning mode
provisioner.clearCredentials(true);

// Or clear without rebooting
provisioner.clearCredentials(false);
ESP.restart(); // Manual restart later
```

---

## Callback Types

### WiFiConnectedCallback

```cpp
typedef void (*WiFiConnectedCallback)()
```

Callback type for WiFi connection success.

**Called when:** Device successfully connects to WiFi

---

### WiFiFailedCallback

```cpp
typedef void (*WiFiFailedCallback)(uint8_t retryCount)
```

Callback type for WiFi connection failure.

**Parameters:**
- `retryCount` - Current retry attempt number

**Called when:** Each connection attempt fails

---

### APModeCallback

```cpp
typedef void (*APModeCallback)(const char* ssid, const char* ip)
```

Callback type for AP mode activation.

**Parameters:**
- `ssid` - AP SSID
- `ip` - AP IP address

**Called when:** Device enters provisioning mode

---

### ResetCallback

```cpp
typedef void (*ResetCallback)()
```

Callback type for device reset.

**Called when:** Reset is triggered (before credentials cleared and reboot)

---

## Enumerations

### LogLevel

```cpp
enum LogLevel {
    LOG_NONE = 0,    // No logging
    LOG_ERROR = 1,   // Errors only
    LOG_INFO = 2,    // Normal operation info
    LOG_DEBUG = 3    // Detailed debugging
}
```

Logging verbosity levels.

---

### ProvisionerState

```cpp
enum ProvisionerState {
    STATE_INIT,                  // Initialization
    STATE_LOAD_CONFIG,          // Loading stored credentials
    STATE_CONNECTING,           // Attempting WiFi connection
    STATE_CONNECTED,            // Successfully connected
    STATE_RETRY_WAIT,           // Waiting before retry
    STATE_PROVISIONING,         // Starting AP mode
    STATE_PROVISIONING_ACTIVE   // AP mode active
}
```

Internal state machine states.

---

## Structures

### WiFiProvisionerConfig

```cpp
struct WiFiProvisionerConfig {
    // AP Configuration
    String apName;
    String apPassword;
    uint32_t apTimeout;
    
    // Connection settings
    uint8_t maxRetries;
    uint32_t retryDelay;
    bool autoWipeOnMaxRetries;
    
    // Hardware reset
    bool hardwareResetEnabled;
    int8_t resetButtonPin;
    uint32_t resetButtonDuration;
    bool resetButtonActiveLow;
    
    // Software reset
    bool httpResetEnabled;
    bool httpResetAuthRequired;
    
    // UX Features
    bool ledEnabled;
    int8_t ledPin;
    bool ledActiveLow;
    bool mdnsEnabled;
    String mdnsName;
    bool doubleRebootDetectEnabled;
    uint32_t doubleRebootWindow;
    
    // Logging
    LogLevel logLevel;
}
```

Internal configuration structure. Not directly accessible; use setter methods instead.

---

## Constants

```cpp
#define WIFI_PROVISIONER_VERSION "1.0.0"
#define DEFAULT_AP_NAME "ESP32-Config"
#define DEFAULT_AP_PASSWORD ""
#define DEFAULT_MAX_RETRIES 10
#define DEFAULT_RETRY_DELAY_MS 3000
#define DEFAULT_AP_TIMEOUT_MS 300000
#define DEFAULT_RESET_BUTTON_DURATION_MS 5000
#define DEFAULT_DOUBLE_REBOOT_WINDOW_MS 10000
#define DNS_PORT 53
#define WEB_SERVER_PORT 80
```

Library constants and defaults.
