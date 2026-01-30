/*
 * Basic WiFi Provisioning Example
 *
 * This example demonstrates the minimal setup required to use
 * the ESP32ProvisionToolkit library.
 *
 * Features demonstrated:
 * - Automatic WiFi connection with stored credentials
 * - Captive portal for initial configuration
 * - Automatic retry on connection failure
 * - LED status indicator
 *
 * Hardware required:
 * - ESP32 development board
 * - Optional: LED on GPIO 2 (built-in LED on most boards)
 *
 * Usage:
 * 1. Upload this sketch to your ESP32
 * 2. If no credentials are stored, the device will create a WiFi access point
 * 3. Connect to the AP (name will be "ESP32-Config-XXXXXX")
 * 4. A captive portal should open automatically
 * 5. Select your WiFi network and enter the password
 * 6. The device will save the credentials and connect
 */

#include <ESP32ProvisionToolkit.h>

// Create provisioner instance
ESP32ProvisionToolkit provisioner;

// LED pin (GPIO 2 is the built-in LED on most ESP32 boards)
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== ESP32 WiFi Provisioner - Basic Example ===\n");

  // Configure the provisioner with fluent API
  provisioner
    .setAPName("MyESP32")              // Custom AP name (will append MAC suffix)
    .setAPPassword("")                  // Open network (or set a password)
    .setMaxRetries(10)                  // Try 10 times before giving up
    .setRetryDelay(3000)                // Wait 3 seconds between retries
    .setAutoWipeOnMaxRetries(true)      // Clear credentials if max retries exceeded
    .setLed(LED_PIN)                    // Enable LED status indicator
    .setLogLevel(LOG_INFO)              // Set logging level
    .onConnected(onWiFiConnected)       // Callback when connected
    .onFailed(onWiFiFailed)             // Callback when connection fails
    .onAPMode(onAPModeStarted)          // Callback when AP mode starts
    .begin();                           // Start the provisioner

  Serial.println("Provisioner initialized");
}

void loop() {
  // Call loop() regularly to handle WiFi state machine
  provisioner.loop();

  // Your application code here
  // The provisioner runs non-blocking, so you can do other work

  if (provisioner.isConnected()) {
    // Do something when connected
    // This is where your main application logic would go
  }
}

// ===== Callback Functions =====

void onWiFiConnected() {
  Serial.println("\n=== WiFi Connected! ===");
  Serial.print("SSID: ");
  Serial.println(provisioner.getSSID());
  Serial.print("IP Address: ");
  Serial.println(provisioner.getLocalIP());
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
