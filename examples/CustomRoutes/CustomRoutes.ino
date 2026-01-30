/*
 * Custom Routes Example
 *
 * This example demonstrates:
 * - Automatic WiFi provisioning with captive portal
 * - LED status indicator
 * - Retry and auto-wipe logic
 * - Custom HTTP routes using the new route APIs
 *
 * Custom routes demonstrated:
 * - GET  /ping        → simple health check
 * - GET  /status      → JSON device status
 *
 * Hardware required:
 * - ESP32 development board
 * - Optional: LED on GPIO 2 (built-in LED on most boards)
 *
 * Usage:
 * 1. Upload this sketch to your ESP32
 * 2. If no credentials are stored, the device creates a WiFi AP
 * 3. Connect to the AP (e.g. "MyESP32-XXXXXX")
 * 4. Complete WiFi provisioning via captive portal
 * 5. Once connected, access custom routes on the device IP
 */

#include <ESP32_WiFiProvisioner.h>

// Create provisioner instance
ESP32_WiFiProvisioner provisioner;

// LED pin (GPIO 2 is the built-in LED on most ESP32 boards)
const int LED_PIN = 2;

// Forward declarations for callbacks
void onWiFiConnected();
void onWiFiFailed(uint8_t retryCount);
void onAPModeStarted(const char* ssid, const char* ip);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== ESP32 WiFi Provisioner - Routes Example ===\n");

  // Configure the provisioner with fluent API
  provisioner
    .setAPName("MyESP32")              // Custom AP name (MAC suffix appended)
    .setAPPassword("")                // Open network
    .setMaxRetries(10)                // Try 10 times before giving up
    .setRetryDelay(3000)              // 3 seconds between retries
    .setAutoWipeOnMaxRetries(true)    // Clear credentials if retries exceeded
    .setLed(LED_PIN)                  // Enable LED status indicator
    .setLogLevel(LOG_INFO)            // Logging level

    // ----- Custom HTTP Routes -----

    // Simple text route
    .addGet("/ping", [](WebServer& server) {
      server.send(200, "text/plain", "pong");
    })

    // JSON status route
    .addGetJsonRoute("/status", []() {
      return String("{") +
        "\"connected\":" + (provisioner.isConnected() ? "true" : "false") + "," +
        "\"ssid\":\"" + provisioner.getSSID() + "\"," +
        "\"uptime_ms\":" + String(millis()) +
      "}";
    })

    // ----- Callbacks -----
    .onConnected(onWiFiConnected)
    .onFailed(onWiFiFailed)
    .onAPMode(onAPModeStarted)

    // Start the provisioner
    .begin();

  Serial.println("Provisioner initialized");
}

void loop() {
  // Handle provisioning state machine
  provisioner.loop();

  // Your application logic can run here
  if (provisioner.isConnected()) {
    // Main application code would go here
  }
}

// ===== Callback Functions =====

void onWiFiConnected() {
  Serial.println("\n=== WiFi Connected! ===");
  Serial.print("SSID: ");
  Serial.println(provisioner.getSSID());
  Serial.print("IP Address: ");
  Serial.println(provisioner.getLocalIP());

  Serial.println("Custom routes available:");
  Serial.println("  GET  /ping");
  Serial.println("  GET  /status");
  Serial.println("========================\n");
}

void onWiFiFailed(uint8_t retryCount) {
  Serial.printf("WiFi connection failed (attempt %d)\n", retryCount);
}

void onAPModeStarted(const char* ssid, const char* ip) {
  Serial.println("\n=== Provisioning Mode Started ===");
  Serial.printf("AP SSID: %s\n", ssid);
  Serial.printf("AP IP: %s\n", ip);
  Serial.println("Connect to this network and open a browser");
  Serial.println("The captive portal should open automatically");
  Serial.println("================================\n");
}
