# Quick Reference Card

## Installation
```cpp
#include <ESP32ProvisionToolkit.h>
ESP32ProvisionToolkit provisioner;
```

## Basic Setup
```cpp
void setup() {
  provisioner.setAPName("MyDevice").begin();
}

void loop() {
  provisioner.loop();
}
```

## Common Configurations

### Production Device
```cpp
provisioner
  .setAPName("Product")
  .setAPPassword("Config123!")
  .enableHardwareReset(0, 10000)
  .enableAuthenticatedHttpReset(true)
  .setLed(2)
  .setLogLevel(LOG_ERROR)
  .begin();
```

### Development Device
```cpp
provisioner
  .setAPName("DevDevice")
  .enableHttpReset(true)
  .setLed(2)
  .setLogLevel(LOG_DEBUG)
  .begin();
```

### Battery-Powered
```cpp
provisioner
  .setAPTimeout(180000)
  .setMaxRetries(5)
  .setRetryDelay(2000)
  .setLogLevel(LOG_ERROR)
  .begin();
```

## Status Checks
```cpp
if (provisioner.isConnected()) { /* connected */ }
if (provisioner.isProvisioning()) { /* in AP mode */ }

String ssid = provisioner.getSSID();
IPAddress ip = provisioner.getLocalIP();
```

## Callbacks
```cpp
provisioner
  .onConnected([]() {
    Serial.println("Connected!");
  })
  .onFailed([](uint8_t retry) {
    Serial.printf("Failed: %d\n", retry);
  })
  .onAPMode([](const char* ssid, const char* ip) {
    Serial.printf("AP: %s @ %s\n", ssid, ip);
  });
```

## Manual Control
```cpp
// Set credentials programmatically
provisioner.setCredentials("SSID", "password");

// Clear credentials
provisioner.clearCredentials();

// Trigger reset
provisioner.reset();
```

## Configuration Methods

| Method | Purpose |
|--------|---------|
| `setAPName(name)` | Set AP SSID base |
| `setAPPassword(pwd)` | Set AP password |
| `setMaxRetries(n)` | Connection attempts |
| `setRetryDelay(ms)` | Delay between retries |
| `setLed(pin)` | Enable LED indicator |
| `enableMDNS(name)` | Enable mDNS |
| `setLogLevel(level)` | Set verbosity |

## Reset Methods

| Method | Description |
|--------|-------------|
| `enableHardwareReset(pin, ms)` | Button reset |
| `enableHttpReset(true)` | Simple HTTP reset |
| `enableAuthenticatedHttpReset(true)` | Password-protected |
| `enableDoubleRebootDetect(ms)` | Emergency reset |

## HTTP Reset Usage

**Simple:**
```bash
curl -X POST http://device-ip/reset
```

**Authenticated:**
```bash
curl -X POST http://device-ip/reset -d "password=YOUR_PASSWORD"
```

## LED Patterns

| Pattern | Meaning |
|---------|---------|
| Fast blink (100ms) | Provisioning mode |
| Slow blink (900ms) | Connecting |
| Solid on | Connected |
| Off | Error/Idle |

## Log Levels

| Level | Output |
|-------|--------|
| `LOG_NONE` | Nothing |
| `LOG_ERROR` | Errors only |
| `LOG_INFO` | Normal info |
| `LOG_DEBUG` | Verbose |

## State Machine

```
INIT → LOAD_CONFIG → CONNECTING → CONNECTED
                   ↘ PROVISIONING ↗
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Won't enter AP mode | `provisioner.clearCredentials()` |
| Can't connect | Check SSID/password, increase retries |
| Captive portal won't open | Navigate to 192.168.4.1 manually |
| High current | Enable WiFi sleep: `WiFi.setSleep(true)` |

## Default Values

```cpp
AP Name:     "ESP32-Config"
AP Password: "" (open)
Max Retries: 10
Retry Delay: 3000ms
AP Timeout:  300000ms (5 min)
Log Level:   LOG_INFO
```

## Memory Usage

```
Base:     ~2KB
AP Mode:  ~15KB
Connected: ~2KB
```

## Links

- Full API: `extras/API_REFERENCE.md`
- Examples: `examples/`
- Integration: `extras/INTEGRATION_GUIDE.md`
