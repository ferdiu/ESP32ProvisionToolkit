/*
 * Headless Device Example
 *
 * This example demonstrates a headless IoT device configuration
 * suitable for deployed devices without user interaction.
 *
 * Features:
 * - Minimal LED feedback (no Serial output in production)
 * - Automatic recovery from connection failures
 * - Remote reset capability
 * - Persistent operation
 *
 * Use case: Sensor nodes, remote monitoring devices, etc.
 *
 * Hardware required:
 * - ESP32 development board
 * - LED on GPIO 2 (status indicator)
 * - Optional: Reset button on GPIO 0
 *
 * LED Patterns:
 * - Fast blink (100ms): Provisioning mode active
 * - Slow blink (900ms): Attempting to connect
 * - Solid on: Connected to WiFi
 * - Off: Error or disconnected
 *
 * Deployment workflow:
 * 1. Program the device with this sketch
 * 2. Power on - device enters provisioning mode
 * 3. Configure WiFi via captive portal
 * 4. Device connects and begins operation
 * 5. If connection is lost, automatic retry
 * 6. Remote reset via HTTP if needed
 */

#include <ESP32ProvisionToolkit.h>

ESP32ProvisionToolkit provisioner;

// GPIO Configuration
const int LED_PIN = 2;
const int RESET_BUTTON_PIN = 0;

// Application state
bool isRunning = false;
unsigned long lastDataSend = 0;
const unsigned long DATA_SEND_INTERVAL = 60000; // Send data every 60 seconds

// Simulated sensor reading
float getSensorReading() {
  // Replace with actual sensor code
  return random(0, 100) / 10.0;
}

void setup() {
  // Initialize Serial for debugging
  // In production, you might disable this to save power
  Serial.begin(115200);
  delay(100);

  Serial.println("=== Headless IoT Device ===");

  // Configure provisioner for headless operation
  provisioner
    // AP Configuration (minimal timeout for faster recovery)
    .setAPName("IoTDevice")
    .setAPTimeout(180000)               // 3 minutes then retry connection

    // Aggressive retry strategy for reliability
    .setMaxRetries(20)                  // Many retries before giving up
    .setRetryDelay(5000)                // 5 seconds between attempts
    .setAutoWipeOnMaxRetries(false)     // Don't auto-wipe (preserve config)

    // Hardware reset for field deployment
    .enableHardwareReset(RESET_BUTTON_PIN, 10000)  // 10 second hold

    // Remote reset for maintenance
    .enableHttpReset(true)              // Simple HTTP reset (trusted network)

    // Visual feedback via LED
    .setLed(LED_PIN)

    // mDNS for easy discovery
    .enableMDNS("iot-device")

    // Minimal logging in production
    #ifdef DEBUG
    .setLogLevel(LOG_DEBUG)
    #else
    .setLogLevel(LOG_ERROR)             // Only errors in production
    #endif

    // Callbacks
    .onConnected(onConnected)
    .onFailed(onFailed)
    .onAPMode(onAPMode)

    .begin();
}

void loop() {
  // Handle provisioner state machine
  provisioner.loop();

  // Only run application when connected
  if (provisioner.isConnected()) {
    if (!isRunning) {
      isRunning = true;
      startApplication();
    }

    runApplication();
  } else {
    if (isRunning) {
      isRunning = false;
      stopApplication();
    }
  }
}

// ===== Application Logic =====

void startApplication() {
  Serial.println("Starting application...");

  // Initialize your application
  // - Connect to MQTT broker
  // - Initialize sensors
  // - Set up timers
  // etc.

  lastDataSend = millis();
}

void stopApplication() {
  Serial.println("Stopping application (WiFi disconnected)");

  // Clean shutdown
  // - Disconnect from MQTT
  // - Save any pending data
  // etc.
}

void runApplication() {
  // Main application loop

  // Example: Send sensor data periodically
  if (millis() - lastDataSend >= DATA_SEND_INTERVAL) {
    sendSensorData();
    lastDataSend = millis();
  }

  // Add your application logic here:
  // - Read sensors
  // - Process data
  // - Handle MQTT messages
  // - Update actuators
  // etc.
}

void sendSensorData() {
  float reading = getSensorReading();

  Serial.print("Sensor reading: ");
  Serial.println(reading);

  // In a real application, send this data:
  // - Publish to MQTT
  // - POST to HTTP API
  // - Store locally if offline
  // etc.

  // Example: Simple HTTP POST
  /*
  WiFiClient client;
  HTTPClient http;

  http.begin(client, "http://your-server.com/api/data");
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"sensor\":\"temp\",\"value\":" + String(reading) + "}";
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("Data sent, response: %d\n", httpCode);
  } else {
    Serial.printf("Send failed: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  */
}

// ===== Callbacks =====

void onConnected() {
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(provisioner.getLocalIP());
  Serial.print("mDNS: http://iot-device.local\n");
}

void onFailed(uint8_t retryCount) {
  Serial.printf("Connection attempt %d failed\n", retryCount);

  // In production, you might:
  // - Log to local storage
  // - Flash LED in error pattern
  // - Go into low-power mode between retries
}

void onAPMode(const char* ssid, const char* ip) {
  Serial.println("Provisioning mode active");
  Serial.printf("Connect to: %s\n", ssid);
  Serial.printf("Configure at: http://%s\n", ip);

  // In production, you might:
  // - Send alert via backup connection (GSM, LoRa, etc.)
  // - Flash LED in specific pattern
  // - Log event to local storage
}

// ===== Power Management (Optional) =====

#ifdef ENABLE_POWER_MANAGEMENT
void enterLightSleep(uint32_t durationMs) {
  Serial.println("Entering light sleep...");
  Serial.flush();

  esp_sleep_enable_timer_wakeup(durationMs * 1000);
  esp_light_sleep_start();

  Serial.println("Woke from light sleep");
}

void runLowPowerMode() {
  // Example: Sleep between data transmissions to save power

  unsigned long timeUntilNextSend = DATA_SEND_INTERVAL - (millis() - lastDataSend);

  if (timeUntilNextSend > 10000) {
    // Sleep for most of the interval, wake up a bit early
    enterLightSleep(timeUntilNextSend - 5000);
  }
}
#endif

// ===== Remote Management =====

/*
 * You can trigger a reset remotely via HTTP:
 *
 * curl -X POST http://iot-device.local/reset
 *
 * Or programmatically from your application:
 */

void triggerRemoteReset() {
  Serial.println("Remote reset triggered");
  provisioner.reset();
}

// ===== Status Monitoring =====

void printStatus() {
  Serial.println("\n=== Device Status ===");
  Serial.print("State: ");

  switch(provisioner.getState()) {
    case STATE_CONNECTED:
      Serial.println("Connected");
      break;
    case STATE_CONNECTING:
      Serial.println("Connecting");
      break;
    case STATE_PROVISIONING:
    case STATE_PROVISIONING_ACTIVE:
      Serial.println("Provisioning");
      break;
    default:
      Serial.println("Other");
  }

  if (provisioner.isConnected()) {
    Serial.print("SSID: ");
    Serial.println(provisioner.getSSID());
    Serial.print("IP: ");
    Serial.println(provisioner.getLocalIP());
  }

  Serial.print("App running: ");
  Serial.println(isRunning ? "Yes" : "No");
  Serial.println("====================\n");
}
