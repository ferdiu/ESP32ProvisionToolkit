/*
 * Secure Reset Example
 * 
 * This example demonstrates how to use all reset mechanisms:
 * - Hardware reset button (long-press)
 * - Authenticated HTTP reset
 * - Double-reboot detection
 * 
 * This configuration is suitable for production devices deployed
 * in untrusted environments where security is important.
 * 
 * Hardware required:
 * - ESP32 development board
 * - Push button connected to GPIO 0 (BOOT button on most boards)
 * - Optional: LED on GPIO 2
 * 
 * Security features:
 * - HTTP reset requires password authentication
 * - Password is hashed before storage
 * - Hardware button requires 5-second press to prevent accidental reset
 * - Double-reboot detection for emergency recovery
 * 
 * Usage:
 * 1. Upload and configure WiFi credentials via captive portal
 * 2. Set a reset password during configuration
 * 3. To reset via hardware: hold BOOT button for 5 seconds
 * 4. To reset via HTTP: POST to http://<device-ip>/reset with password
 * 5. Emergency reset: reboot device twice within 10 seconds
 */

#include <ESP32_WiFiProvisioner.h>

ESP32_WiFiProvisioner provisioner;

// GPIO pins
const int RESET_BUTTON_PIN = 0;  // BOOT button on most ESP32 boards
const int LED_PIN = 2;            // Built-in LED

// Configuration
const unsigned long RESET_BUTTON_HOLD_TIME = 5000;  // 5 seconds
const unsigned long DOUBLE_REBOOT_WINDOW = 10000;   // 10 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32 WiFi Provisioner - Secure Reset Example ===\n");
  
  // Configure with all security features enabled
  provisioner
    // AP Configuration
    .setAPName("SecureESP32")
    .setAPPassword("config123")         // Password-protected AP
    .setAPTimeout(300000)                // 5 minute timeout
    
    // Connection settings
    .setMaxRetries(10)
    .setRetryDelay(3000)
    .setAutoWipeOnMaxRetries(true)
    
    // Hardware reset button
    .enableHardwareReset(RESET_BUTTON_PIN, RESET_BUTTON_HOLD_TIME)
    
    // Authenticated HTTP reset
    .enableAuthenticatedHttpReset(true)
    
    // UX features
    .setLed(LED_PIN)
    .enableMDNS("secure-esp32")         // Access via http://secure-esp32.local
    .enableDoubleRebootDetect(DOUBLE_REBOOT_WINDOW)
    
    // Logging
    .setLogLevel(LOG_INFO)
    
    // Callbacks
    .onConnected(onWiFiConnected)
    .onFailed(onWiFiFailed)
    .onAPMode(onAPModeStarted)
    .onReset(onResetTriggered)
    
    .begin();
  
  Serial.println("Provisioner initialized with secure reset features");
  printResetInstructions();
}

void loop() {
  provisioner.loop();
  
  // Your secure application code here
  if (provisioner.isConnected()) {
    // Connected - run main application
    
    // Example: Check for remote reset command
    // In a real application, this might come from MQTT, HTTP API, etc.
  }
}

// ===== Callback Functions =====

void onWiFiConnected() {
  Serial.println("\n=== WiFi Connected! ===");
  Serial.print("SSID: ");
  Serial.println(provisioner.getSSID());
  Serial.print("IP Address: ");
  Serial.println(provisioner.getLocalIP());
  Serial.print("mDNS: http://secure-esp32.local\n");
  Serial.println("========================\n");
  
  printResetInstructions();
}

void onWiFiFailed(uint8_t retryCount) {
  Serial.printf("WiFi connection failed (attempt %d)\n", retryCount);
}

void onAPModeStarted(const char* ssid, const char* ip) {
  Serial.println("\n=== Provisioning Mode Started ===");
  Serial.printf("AP SSID: %s\n", ssid);
  Serial.printf("AP Password: config123\n");
  Serial.printf("AP IP: %s\n", ip);
  Serial.println("\nIMPORTANT: Set a reset password during configuration!");
  Serial.println("This password will be required for HTTP reset operations.");
  Serial.println("================================\n");
}

void onResetTriggered() {
  Serial.println("\n!!! RESET TRIGGERED !!!");
  Serial.println("Clearing all credentials...");
  Serial.println("Device will reboot into provisioning mode");
}

// ===== Helper Functions =====

void printResetInstructions() {
  Serial.println("\n=== Reset Methods Available ===");
  Serial.println("1. Hardware Reset:");
  Serial.printf("   - Hold BOOT button (GPIO %d) for %lu seconds\n", 
                RESET_BUTTON_PIN, RESET_BUTTON_HOLD_TIME / 1000);
  
  Serial.println("\n2. HTTP Reset (requires password):");
  Serial.println("   curl -X POST http://<device-ip>/reset -d \"password=YOUR_PASSWORD\"");
  Serial.println("   or via mDNS:");
  Serial.println("   curl -X POST http://secure-esp32.local/reset -d \"password=YOUR_PASSWORD\"");
  
  Serial.println("\n3. Emergency Double-Reboot:");
  Serial.printf("   - Reboot device twice within %lu seconds\n", 
                DOUBLE_REBOOT_WINDOW / 1000);
  
  Serial.println("================================\n");
}

// ===== Example: Programmatic Reset =====
// Uncomment this function and call it to trigger a reset from code

/*
void triggerProgrammaticReset() {
  Serial.println("Triggering programmatic reset...");
  provisioner.reset();
  // Device will reboot
}
*/

// ===== Example: Clear credentials without reboot =====
// Useful for testing or specific application needs

/*
void clearCredentialsOnly() {
  Serial.println("Clearing credentials (no reboot)...");
  provisioner.clearCredentials(false);
  Serial.println("Credentials cleared. Restart manually to enter provisioning mode.");
}
*/
