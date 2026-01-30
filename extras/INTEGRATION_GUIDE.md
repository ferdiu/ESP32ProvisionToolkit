# Integration Guide

This guide walks through integrating ESP32ProvisionToolkit into your project.

## Table of Contents

1. [Installation](#installation)
2. [Basic Integration](#basic-integration)
3. [Common Use Cases](#common-use-cases)
4. [Advanced Integration](#advanced-integration)
5. [Testing & Debugging](#testing--debugging)
6. [Troubleshooting](#troubleshooting)

## Installation

### Arduino IDE

1. **Download the Library**
   - Download ZIP from GitHub releases
   - Or clone: `git clone https://github.com/ferdiu/ESP32ProvisionToolkit.git`

2. **Install in Arduino IDE**
   - Sketch → Include Library → Add .ZIP Library
   - Select the downloaded ZIP file
   - Restart Arduino IDE

3. **Verify Installation**
   ```cpp
   #include <ESP32ProvisionToolkit.h>

   void setup() {
     ESP32ProvisionToolkit provisioner;
     // Compiles successfully = installed correctly
   }
   ```

### PlatformIO

1. **Add to `platformio.ini`**
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   lib_deps =
       https://github.com/ferdiu/ESP32ProvisionToolkit.git
   ```

2. **Or Install Manually**
   ```bash
   pio lib install ESP32ProvisionToolkit
   ```

3. **Verify Installation**
   ```bash
   pio run
   ```

## Basic Integration

### Step 1: Include Header

```cpp
#include <ESP32ProvisionToolkit.h>
```

### Step 2: Create Instance

Create a global instance:

```cpp
ESP32ProvisionToolkit provisioner;
```

**Important:** Must be global, not local to `setup()`.

### Step 3: Configure in setup()

```cpp
void setup() {
  Serial.begin(115200);

  provisioner
    .setAPName("MyDevice")
    .setLed(2)
    .begin();
}
```

### Step 4: Call loop()

```cpp
void loop() {
  provisioner.loop();

  // Your application code
}
```

### Complete Minimal Example

```cpp
#include <ESP32ProvisionToolkit.h>

ESP32ProvisionToolkit provisioner;

void setup() {
  Serial.begin(115200);

  provisioner
    .setAPName("MyESP32")
    .setLed(2)
    .onConnected([]() {
      Serial.println("WiFi Connected!");
    })
    .begin();
}

void loop() {
  provisioner.loop();
}
```

## Common Use Cases

### 1. IoT Sensor Device

```cpp
#include <ESP32ProvisionToolkit.h>

ESP32ProvisionToolkit provisioner;
unsigned long lastReading = 0;
const unsigned long READING_INTERVAL = 60000; // 1 minute

void setup() {
  Serial.begin(115200);

  provisioner
    .setAPName("TempSensor")
    .setMaxRetries(20)
    .setLed(2)
    .enableMDNS("temp-sensor")
    .onConnected(onWiFiConnected)
    .begin();

  // Initialize your sensor here
  // Wire.begin();
  // sensor.begin();
}

void loop() {
  provisioner.loop();

  if (provisioner.isConnected()) {
    if (millis() - lastReading >= READING_INTERVAL) {
      readAndSendData();
      lastReading = millis();
    }
  }
}

void onWiFiConnected() {
  Serial.println("Connected! Starting data collection...");
  // Connect to MQTT, HTTP API, etc.
}

void readAndSendData() {
  // Read sensor
  float temperature = readTemperature();

  // Send to cloud
  sendToServer(temperature);
}
```

### 2. Smart Home Device

```cpp
#include <ESP32ProvisionToolkit.h>
#include <PubSubClient.h>

ESP32ProvisionToolkit provisioner;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

const char* MQTT_SERVER = "mqtt.home.local";
const int MQTT_PORT = 1883;

void setup() {
  Serial.begin(115200);

  provisioner
    .setAPName("SmartPlug")
    .enableHardwareReset(0, 10000)  // 10 sec hold
    .enableAuthenticatedHttpReset(true)
    .setLed(2)
    .enableMDNS("smart-plug")
    .onConnected(connectMQTT)
    .onFailed([](uint8_t retry) {
      // Disconnect MQTT on WiFi failure
      mqtt.disconnect();
    })
    .begin();

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
  provisioner.loop();

  if (provisioner.isConnected()) {
    if (!mqtt.connected()) {
      connectMQTT();
    }
    mqtt.loop();
  }
}

void connectMQTT() {
  if (mqtt.connect("smart-plug-001")) {
    mqtt.subscribe("home/plug/command");
    Serial.println("MQTT connected");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle MQTT commands
  if (strcmp(topic, "home/plug/command") == 0) {
    if (payload[0] == '1') {
      digitalWrite(RELAY_PIN, HIGH);
    } else {
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}
```

### 3. Remote Monitoring Station

```cpp
#include <ESP32ProvisionToolkit.h>
#include <HTTPClient.h>

ESP32ProvisionToolkit provisioner;

const char* SERVER_URL = "https://api.example.com/data";
const char* API_KEY = "your-api-key";

void setup() {
  Serial.begin(115200);

  provisioner
    .setAPName("MonitorStation")
    .setAPTimeout(300000)  // 5 min
    .setMaxRetries(30)
    .setRetryDelay(5000)
    .enableHardwareReset(0, 5000)
    .setLed(2)
    .setLogLevel(LOG_INFO)
    .onConnected([]() {
      Serial.println("Online - starting monitoring");
    })
    .begin();
}

void loop() {
  provisioner.loop();

  static unsigned long lastUpdate = 0;

  if (provisioner.isConnected() &&
      millis() - lastUpdate >= 300000) { // 5 minutes

    sendUpdate();
    lastUpdate = millis();
  }
}

void sendUpdate() {
  HTTPClient http;

  // Collect data
  float voltage = analogRead(35) * 3.3 / 4095.0;

  // Prepare JSON
  String payload = "{";
  payload += "\"device\":\"station-001\",";
  payload += "\"voltage\":" + String(voltage, 2) + ",";
  payload += "\"rssi\":" + String(WiFi.RSSI());
  payload += "}";

  // Send to server
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    Serial.println("Update sent successfully");
  } else {
    Serial.printf("Update failed: %d\n", httpCode);
  }

  http.end();
}
```

### 4. Battery-Powered Device

```cpp
#include <ESP32ProvisionToolkit.h>

ESP32ProvisionToolkit provisioner;

#define SLEEP_DURATION_SECONDS 3600  // 1 hour

void setup() {
  Serial.begin(115200);

  // Aggressive timeout for battery saving
  provisioner
    .setAPName("BatteryDevice")
    .setAPTimeout(180000)      // 3 min timeout
    .setMaxRetries(5)          // Fewer retries
    .setRetryDelay(2000)       // Shorter delay
    .enableDoubleRebootDetect(10000)  // Emergency config
    .setLogLevel(LOG_ERROR)    // Minimal logging
    .onConnected(sendDataAndSleep)
    .onFailed([](uint8_t retry) {
      if (retry >= 5) {
        // Failed to connect, sleep anyway
        goToSleep();
      }
    })
    .begin();
}

void loop() {
  provisioner.loop();

  // Will sleep after connecting or failing
}

void sendDataAndSleep() {
  // Quick data transmission
  float batteryVoltage = readBattery();
  float sensorValue = readSensor();

  sendToServer(batteryVoltage, sensorValue);

  // Go to deep sleep
  goToSleep();
}

void goToSleep() {
  Serial.println("Going to sleep...");
  Serial.flush();

  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

float readBattery() {
  return analogRead(35) * 2 * 3.3 / 4095.0;
}
```

## Advanced Integration

### Custom Web Server Endpoints

If you need additional HTTP endpoints alongside provisioning:

```cpp
#include <ESP32ProvisionToolkit.h>
#include <WebServer.h>

ESP32ProvisionToolkit provisioner;
WebServer server(8080);  // Use different port

void setup() {
  provisioner
    .setAPName("MyDevice")
    .onConnected(startWebServer)
    .begin();
}

void startWebServer() {
  server.on("/api/status", HTTP_GET, []() {
    String json = "{\"status\":\"ok\"}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Web server started on port 8080");
}

void loop() {
  provisioner.loop();

  if (provisioner.isConnected()) {
    server.handleClient();
  }
}
```

### Integration with OTA Updates

```cpp
#include <ESP32ProvisionToolkit.h>
#include <ArduinoOTA.h>

ESP32ProvisionToolkit provisioner;

void setup() {
  provisioner
    .setAPName("OTADevice")
    .enableMDNS("ota-device")
    .onConnected(setupOTA)
    .begin();
}

void setupOTA() {
  ArduinoOTA.setHostname("ota-device");
  ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void loop() {
  provisioner.loop();

  if (provisioner.isConnected()) {
    ArduinoOTA.handle();
  }
}
```

### Conditional Compilation for Different Builds

```cpp
#include <ESP32ProvisionToolkit.h>

ESP32ProvisionToolkit provisioner;

void setup() {
  provisioner.setAPName("MyDevice");

  #ifdef PRODUCTION
    // Production configuration
    provisioner
      .setAPTimeout(180000)
      .enableAuthenticatedHttpReset(true)
      .setLogLevel(LOG_ERROR);
  #else
    // Development configuration
    provisioner
      .setAPTimeout(600000)  // Longer timeout
      .enableHttpReset(true)  // Simple reset
      .setLogLevel(LOG_DEBUG);
  #endif

  #ifdef ENABLE_HARDWARE_RESET
    provisioner.enableHardwareReset(RESET_PIN, RESET_DURATION);
  #endif

  provisioner.begin();
}
```

### Multi-Network Fallback (Future Feature)

```cpp
// Not yet implemented, but shows intended usage pattern
void setup() {
  provisioner
    .setAPName("MyDevice")
    .addNetwork("PrimarySSID", "password1", 1)     // Priority 1
    .addNetwork("BackupSSID", "password2", 2)      // Priority 2
    .addNetwork("GuestSSID", "password3", 3)       // Priority 3
    .enableNetworkFallback(true)
    .begin();
}
```

## Testing & Debugging

### Enable Debug Output

```cpp
void setup() {
  Serial.begin(115200);

  provisioner
    .setLogLevel(LOG_DEBUG)  // Maximum verbosity
    .begin();
}
```

### Monitor State Transitions

```cpp
void setup() {
  provisioner
    .onConnected([]() {
      Serial.println("STATE: Connected");
    })
    .onFailed([](uint8_t retry) {
      Serial.printf("STATE: Failed (retry %d)\n", retry);
    })
    .onAPMode([](const char* ssid, const char* ip) {
      Serial.printf("STATE: AP Mode (%s @ %s)\n", ssid, ip);
    })
    .onReset([]() {
      Serial.println("STATE: Resetting");
    })
    .begin();
}

void loop() {
  provisioner.loop();

  static ProvisionerState lastState = STATE_INIT;
  ProvisionerState currentState = provisioner.getState();

  if (currentState != lastState) {
    Serial.printf("State changed: %d -> %d\n", lastState, currentState);
    lastState = currentState;
  }
}
```

### Test Reset Mechanisms

```cpp
void setup() {
  // ... provisioner setup ...

  // Test button after 10 seconds
  delay(10000);
  Serial.println("Press and hold button NOW");
}
```

### Simulate Network Conditions

```cpp
void loop() {
  provisioner.loop();

  // Simulate intermittent connectivity
  static unsigned long lastDisconnect = 0;
  if (millis() - lastDisconnect > 60000) {
    WiFi.disconnect();
    Serial.println("Simulated disconnect");
    lastDisconnect = millis();
  }
}
```

## Troubleshooting

### Problem: Device immediately reboots in a loop

**Cause:** Double-reboot detection triggered

**Solution:**
```cpp
// Disable during development
provisioner.enableDoubleRebootDetect(false);

// Or clear NVS
#include <Preferences.h>
Preferences prefs;
prefs.begin("wifiprov", false);
prefs.clear();
prefs.end();
```

### Problem: Cannot access configuration page

**Solution:**
1. Check Serial output for AP IP (usually 192.168.4.1)
2. Manually navigate to IP instead of waiting for captive portal
3. Verify AP is started:
   ```cpp
   if (provisioner.isProvisioning()) {
     Serial.printf("AP at: http://%s\n", provisioner.getAPIP().c_str());
   }
   ```

### Problem: Credentials not persisting after reboot

**Solution:**
1. Check NVS partition in partition table
2. Verify partition is large enough (min 12KB)
3. Check for NVS initialization errors in Serial output

### Problem: High current consumption

**Cause:** WiFi is power-hungry

**Solution:**
```cpp
// Use WiFi power saving
WiFi.setSleep(true);

// Or use deep sleep between operations (see battery example)
```

### Problem: Slow captive portal response

**Solution:**
1. Reduce scan time:
   - Network scanning is automatic but can be slow
   - Consider caching results
2. Optimize HTML size
3. Use faster web server (AsyncWebServer)

## Best Practices

1. **Always check `isConnected()` before network operations**
   ```cpp
   if (provisioner.isConnected()) {
     // Safe to use network
   }
   ```

2. **Handle connection failures gracefully**
   ```cpp
   provisioner.onFailed([](uint8_t retry) {
     // Log, save data locally, etc.
   });
   ```

3. **Use callbacks for state transitions**
   ```cpp
   provisioner
     .onConnected(startServices)
     .onFailed(stopServices);
   ```

4. **Set appropriate timeouts for your use case**
   ```cpp
   // Short timeout for battery devices
   provisioner.setAPTimeout(180000);

   // Longer for development
   provisioner.setAPTimeout(600000);
   ```

5. **Enable security features in production**
   ```cpp
   #ifdef PRODUCTION
   provisioner.enableAuthenticatedHttpReset(true);
   #endif
   ```

## Next Steps

- Review the [API Reference](extras/API_REFERENCE.md)
- Check out the [examples](examples/)
- Join the community discussions
