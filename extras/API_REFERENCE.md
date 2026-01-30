# API Reference

Complete API documentation for ESP32ProvisionToolkit library.

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
class ESP32ProvisionToolkit
```

Main class providing WiFi provisioning and recovery functionality.

**Header:** `ESP32ProvisionToolkit.h`

**Namespace:** Global

### Constructor

```cpp
ESP32ProvisionToolkit()
```

Creates a new provisioner instance. Initializes internal state.

**Example:**
```cpp
ESP32ProvisionToolkit provisioner;
```

### Destructor

```cpp
~ESP32ProvisionToolkit()
```

Cleans up allocated resources (DNS server, web server).

## Configuration Methods

All configuration methods return `ESP32ProvisionToolkit&` for method chaining.

### Access Point Configuration

#### setAPName

```cpp
ESP32ProvisionToolkit& setAPName(const String& name)
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
ESP32ProvisionToolkit& setAPPassword(const String& password)
```

Sets password for the configuration access point.

**Parameters:**
- `password` - WiFi password (empty string = open network)

**Returns:** Reference to this instance

**Default:** `""` (open network)

**Security Note:** Use strong passwords in production!

**Note:** As required by the WPA2 specifications, the password has to be at least of 8 characters long.

**Example:**
```cpp
provisioner.setAPPassword("ConfigPass123");
```

---

#### setAPTimeout

```cpp
ESP32ProvisionToolkit& setAPTimeout(uint32_t milliseconds)
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
ESP32ProvisionToolkit& setMaxRetries(uint8_t retries)
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
ESP32ProvisionToolkit& setRetryDelay(uint32_t milliseconds)
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
ESP32ProvisionToolkit& setAutoWipeOnMaxRetries(bool enable)
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
ESP32ProvisionToolkit& enableHardwareReset(
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
ESP32ProvisionToolkit& disableHardwareReset()
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
ESP32ProvisionToolkit& enableHttpReset(bool enable)
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
ESP32ProvisionToolkit& enableAuthenticatedHttpReset(bool enable)
```

Enables password-protected HTTP reset.

**Parameters:**
- `enable` - `true` to enable with authentication, `false` to disable

**Returns:** Reference to this instance

**Default:** Disabled

**Note:** Password must be set during provisioning via web interface. Please, note that this password is meant to be used only once during reset; since it is sent in plain-text this means that after sending it once it HAS to be considered compromised. This is still secure if you only use it for resetting the device and you keep it changing between device resets.

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
ESP32ProvisionToolkit& setLed(int8_t pin, bool activeLow = false)
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
ESP32ProvisionToolkit& enableMDNS(const String& name)
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
ESP32ProvisionToolkit& enableDoubleRebootDetect(
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
ESP32ProvisionToolkit& setLogLevel(LogLevel level)
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
ESP32ProvisionToolkit& onConnected(WiFiConnectedCallback callback)
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
ESP32ProvisionToolkit& onFailed(WiFiFailedCallback callback)
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
ESP32ProvisionToolkit& onAPMode(APModeCallback callback)
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
ESP32ProvisionToolkit& onReset(ResetCallback callback)
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

## Custom HTTP Routes

The ESP32ProvisionToolkit allows applications to **register custom HTTP endpoints** without accessing the internal web server directly.

Routes are:

* Registered **before `begin()`**
* Stored internally
* Automatically attached when the web server starts
* Scoped to provisioning mode, connected mode, or both
* Optionally protected by authentication

This enables clean extension points for:

* Diagnostics
* Status APIs
* OTA updates
* Device control endpoints

---

### Route Scope

#### HttpRouteScope

```cpp
enum HttpRouteScope {
    ROUTE_PROVISIONING_ONLY,  // Active only during AP provisioning mode
    ROUTE_CONNECTED_ONLY,     // Active only when connected to WiFi
    ROUTE_BOTH                // Active in both modes
}
```

Controls **when** a route is available.

**Default:** `ROUTE_CONNECTED_ONLY`

---

### Core Route Registration

#### addHttpRoute

```cpp
ESP32ProvisionToolkit& addHttpRoute(
    const String& path,
    HTTPMethod method,
    HttpRouteHandler handler,
    HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
    bool requiresAuth = false
)
```

Registers a custom HTTP route.

**Parameters:**

* `path` – URL path (e.g. `"/status"`)
* `method` – HTTP method (`HTTP_GET`, `HTTP_POST`, etc.)
* `handler` – Route handler function
* `scope` – When the route is active
* `requiresAuth` – Whether authentication is required

**Returns:** Reference to this instance

**Notes:**

* Routes are applied automatically when the web server starts
* Routes persist across server restarts
* No direct access to the internal WebServer is required

**Example:**

```cpp
provisioner.addHttpRoute(
    "/uptime",
    HTTP_GET,
    [](WebServer& server) {
        server.send(200, "text/plain", String(millis()));
    }
);
```

---

### Convenience Route Helpers

#### addGet

```cpp
ESP32ProvisionToolkit& addGet(
    const String& path,
    HttpRouteHandler handler,
    HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
    bool requiresAuth = false
)
```

Registers a `GET` route.

**Example:**

```cpp
provisioner.addGet("/ping", [](WebServer& s) {
    s.send(200, "text/plain", "pong");
});
```

---

#### addPost

```cpp
ESP32ProvisionToolkit& addPost(
    const String& path,
    HttpRouteHandler handler,
    HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
    bool requiresAuth = false
)
```

Registers a `POST` route.

**Example:**

```cpp
provisioner.addPost("/reboot", [](WebServer& s) {
    s.send(200, "text/plain", "Rebooting");
    ESP.restart();
}, ROUTE_CONNECTED_ONLY, true);
```

---

### JSON Route Helpers

JSON routes simplify the creation of REST-style endpoints by automatically setting the response content type.

---

#### addJsonRoute

```cpp
ESP32ProvisionToolkit& addJsonRoute(
    const String& path,
    HTTPMethod method,
    std::function<String()> jsonProvider,
    HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
    bool requiresAuth = false
)
```

Registers a JSON-producing route.

**Parameters:**

* `jsonProvider` – Function returning a JSON-formatted string

**Example:**

```cpp
provisioner.addJsonRoute(
    "/status",
    HTTP_GET,
    []() {
        return "{\"state\":\"ok\",\"uptime\":" + String(millis()) + "}";
    }
);
```

---

#### addGetJsonRoute

```cpp
ESP32ProvisionToolkit& addGetJsonRoute(
    const String& path,
    std::function<String()> jsonProvider,
    HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
    bool requiresAuth = false
)
```

Registers a JSON `GET` route.

**Example:**

```cpp
provisioner.addGetJsonRoute("/info", []() {
    return "{\"device\":\"esp32\",\"version\":\"1.0.0\"}";
});
```

---

#### addPostJsonRoute

```cpp
ESP32ProvisionToolkit& addPostJsonRoute(
    const String& path,
    std::function<String()> jsonProvider,
    HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
    bool requiresAuth = false
)
```

Registers a JSON `POST` route.

**Example:**

```cpp
provisioner.addPostJsonRoute(
    "/metrics",
    []() {
        return "{\"heap\":" + String(ESP.getFreeHeap()) + "}";
    },
    ROUTE_CONNECTED_ONLY,
    true
);
```

---

## Custom Route Introspection

Utility methods for inspecting registered custom HTTP routes.

---

### hasCustomRoutes

```cpp
bool hasCustomRoutes() const
```

Checks whether any custom HTTP routes have been registered.

**Returns:**

* `true` if at least one custom route exists
* `false` otherwise

**Example:**

```cpp
if (provisioner.hasCustomRoutes()) {
    Serial.println("Custom routes registered");
}
```

---

### hasConnectedOnlyRoutes

```cpp
bool hasConnectedOnlyRoutes() const
```

Checks whether any custom routes are registered with `ROUTE_CONNECTED_ONLY` scope.

**Returns:**

* `true` if at least one connected-only route exists
* `false` otherwise

**Example:**

```cpp
if (provisioner.hasConnectedOnlyRoutes()) {
    // Start web server in connected mode
}
```

---

### hasProvisioningOnlyRoutes

```cpp
bool hasProvisioningOnlyRoutes() const
```

Checks whether any custom routes are registered with `ROUTE_PROVISIONING_ONLY` scope.

**Returns:**

* `true` if at least one provisioning-only route exists
* `false` otherwise

**Example:**

```cpp
if (provisioner.hasProvisioningOnlyRoutes()) {
    // Start web server in provisioning mode
}
```

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

### HttpRouteScope

```cpp
enum HttpRouteScope {
    ROUTE_PROVISIONING_ONLY,
    ROUTE_CONNECTED_ONLY,
    ROUTE_BOTH
};
```

Used to provide custom routes only on selected scope(s).

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
