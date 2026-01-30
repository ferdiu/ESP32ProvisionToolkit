/*
 * ESP32_WiFiProvisioner.cpp
 * Implementation of Wi-Fi provisioning and recovery system
 */

#include "ESP32_WiFiProvisioner.h"
#include <esp_wifi.h>

// Static instance pointer for web server callbacks
ESP32_WiFiProvisioner* ESP32_WiFiProvisioner::_instance = nullptr;

// NVS namespace and keys
#define NVS_NAMESPACE "wifiprov"
#define NVS_SSID "ssid"
#define NVS_PASSWORD "password"
#define NVS_RESET_PWD "reset_pwd"
#define NVS_BOOT_COUNT "boot_count"
#define NVS_BOOT_TIME "boot_time"

ESP32_WiFiProvisioner::ESP32_WiFiProvisioner() :
    _state(STATE_INIT),
    _retryCount(0),
    _lastRetryTime(0),
    _apStartTime(0),
    _buttonPressStart(0),
    _buttonPressed(false),
    _dnsServer(nullptr),
    _webServer(nullptr),
    _onConnectedCallback(nullptr),
    _onFailedCallback(nullptr),
    _onAPModeCallback(nullptr),
    _onResetCallback(nullptr),
    _lastLedToggle(0),
    _ledState(false)
{
    _instance = this;
}

ESP32_WiFiProvisioner::~ESP32_WiFiProvisioner() {
    if (_dnsServer) delete _dnsServer;
    if (_webServer) delete _webServer;
    _instance = nullptr;
}

// ===== Configuration API =====

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setAPName(const String& name) {
    _config.apName = name;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setAPPassword(const String& password) {
    _config.apPassword = password;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setAPTimeout(uint32_t milliseconds) {
    _config.apTimeout = milliseconds;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setMaxRetries(uint8_t retries) {
    _config.maxRetries = retries;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setRetryDelay(uint32_t milliseconds) {
    _config.retryDelay = milliseconds;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setAutoWipeOnMaxRetries(bool enable) {
    _config.autoWipeOnMaxRetries = enable;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::enableHardwareReset(int8_t pin, uint32_t durationMs, bool activeLow) {
    _config.hardwareResetEnabled = true;
    _config.resetButtonPin = pin;
    _config.resetButtonDuration = durationMs;
    _config.resetButtonActiveLow = activeLow;

    pinMode(pin, activeLow ? INPUT_PULLUP : INPUT);

    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::disableHardwareReset() {
    _config.hardwareResetEnabled = false;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::enableHttpReset(bool enable) {
    _config.httpResetEnabled = enable;
    _config.httpResetAuthRequired = false;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::enableAuthenticatedHttpReset(bool enable) {
    _config.httpResetEnabled = enable;
    _config.httpResetAuthRequired = enable;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setLed(int8_t pin, bool activeLow) {
    _config.ledEnabled = true;
    _config.ledPin = pin;
    _config.ledActiveLow = activeLow;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, activeLow ? HIGH : LOW);

    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::enableMDNS(const String& name) {
    _config.mdnsEnabled = true;
    _config.mdnsName = name;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::enableDoubleRebootDetect(uint32_t windowMs) {
    _config.doubleRebootDetectEnabled = true;
    _config.doubleRebootWindow = windowMs;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::setLogLevel(LogLevel level) {
    _config.logLevel = level;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::onConnected(WiFiConnectedCallback callback) {
    _onConnectedCallback = callback;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::onFailed(WiFiFailedCallback callback) {
    _onFailedCallback = callback;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::onAPMode(APModeCallback callback) {
    _onAPModeCallback = callback;
    return *this;
}

ESP32_WiFiProvisioner& ESP32_WiFiProvisioner::onReset(ResetCallback callback) {
    _onResetCallback = callback;
    return *this;
}

// ===== Core Control =====

bool ESP32_WiFiProvisioner::begin() {
    log(LOG_INFO, "WiFiProvisioner v%s starting...", WIFI_PROVISIONER_VERSION);

    // Check for double-reboot detection
    if (_config.doubleRebootDetectEnabled) {
        checkDoubleReboot();
    }

    // Load configuration
    _state = STATE_LOAD_CONFIG;
    return true;
}

void ESP32_WiFiProvisioner::loop() {
    // Handle reset button
    if (_config.hardwareResetEnabled) {
        checkHardwareReset();
    }

    // Update LED
    if (_config.ledEnabled) {
        updateLED();
    }

    // State machine
    switch (_state) {
        case STATE_INIT:
            handleStateInit();
            break;

        case STATE_LOAD_CONFIG:
            handleStateLoadConfig();
            break;

        case STATE_CONNECTING:
            handleStateConnecting();
            break;

        case STATE_CONNECTED:
            handleStateConnected();
            break;

        case STATE_RETRY_WAIT:
            handleStateRetryWait();
            break;

        case STATE_PROVISIONING:
            handleStateProvisioning();
            break;

        case STATE_PROVISIONING_ACTIVE:
            handleStateProvisioningActive();
            break;
    }
}

void ESP32_WiFiProvisioner::reset() {
    performReset("Programmatic reset");
}

// ===== Status Query =====

bool ESP32_WiFiProvisioner::isConnected() const {
    return _state == STATE_CONNECTED && WiFi.status() == WL_CONNECTED;
}

bool ESP32_WiFiProvisioner::isProvisioning() const {
    return _state == STATE_PROVISIONING || _state == STATE_PROVISIONING_ACTIVE;
}

ProvisionerState ESP32_WiFiProvisioner::getState() const {
    return _state;
}

String ESP32_WiFiProvisioner::getSSID() const {
    return _storedSSID;
}

IPAddress ESP32_WiFiProvisioner::getLocalIP() const {
    return WiFi.localIP();
}

String ESP32_WiFiProvisioner::getAPIP() const {
    return WiFi.softAPIP().toString();
}

// ===== Manual Control =====

bool ESP32_WiFiProvisioner::setCredentials(const String& ssid, const String& password, bool reboot) {
    if (saveCredentials(ssid, password)) {
        log(LOG_INFO, "Credentials saved: %s", ssid.c_str());
        if (reboot) {
            delay(500);
            ESP.restart();
        }
        return true;
    }
    return false;
}

bool ESP32_WiFiProvisioner::clearCredentials(bool reboot) {
    clearAllCredentials();
    log(LOG_INFO, "Credentials cleared");
    if (reboot) {
        delay(500);
        ESP.restart();
    }
    return true;
}

// ===== Storage =====

bool ESP32_WiFiProvisioner::loadCredentials() {
    if (!_preferences.begin(NVS_NAMESPACE, true)) {
        log(LOG_ERROR, "Failed to open NVS");
        return false;
    }

    _storedSSID = _preferences.getString(NVS_SSID, "");
    _storedPassword = _preferences.getString(NVS_PASSWORD, "");

    _preferences.end();

    bool hasCredentials = _storedSSID.length() > 0;
    log(LOG_DEBUG, "Loaded credentials: SSID=%s, hasPassword=%d",
        _storedSSID.c_str(), _storedPassword.length() > 0);

    return hasCredentials;
}

bool ESP32_WiFiProvisioner::saveCredentials(const String& ssid, const String& password) {
    if (!_preferences.begin(NVS_NAMESPACE, false)) {
        log(LOG_ERROR, "Failed to open NVS for writing");
        return false;
    }

    _preferences.putString(NVS_SSID, ssid);
    _preferences.putString(NVS_PASSWORD, password);

    _preferences.end();

    _storedSSID = ssid;
    _storedPassword = password;

    return true;
}

bool ESP32_WiFiProvisioner::loadResetPassword() {
    if (!_preferences.begin(NVS_NAMESPACE, true)) {
        return false;
    }

    _resetPassword = _preferences.getString(NVS_RESET_PWD, "");
    _preferences.end();

    return _resetPassword.length() > 0;
}

bool ESP32_WiFiProvisioner::saveResetPassword(const String& password) {
    if (!_preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }

    String hash = hashPassword(password);
    _preferences.putString(NVS_RESET_PWD, hash);
    _preferences.end();

    _resetPassword = hash;

    return true;
}

void ESP32_WiFiProvisioner::clearAllCredentials() {
    if (_preferences.begin(NVS_NAMESPACE, false)) {
        _preferences.clear();
        _preferences.end();
    }

    _storedSSID = "";
    _storedPassword = "";
    _resetPassword = "";
}

// ===== State Machine =====

void ESP32_WiFiProvisioner::handleStateInit() {
    _state = STATE_LOAD_CONFIG;
}

void ESP32_WiFiProvisioner::handleStateLoadConfig() {
    if (loadCredentials()) {
        log(LOG_INFO, "Found stored credentials for: %s", _storedSSID.c_str());
        _retryCount = 0;
        _state = STATE_CONNECTING;
        setLEDPattern(100, 900); // Slow blink
    } else {
        log(LOG_INFO, "No credentials found, entering provisioning mode");
        _state = STATE_PROVISIONING;
    }
}

void ESP32_WiFiProvisioner::handleStateConnecting() {
    if (connectToWiFi()) {
        log(LOG_INFO, "Connected to WiFi: %s", _storedSSID.c_str());
        log(LOG_INFO, "IP Address: %s", WiFi.localIP().toString().c_str());

        // Setup mDNS if enabled
        if (_config.mdnsEnabled) {
            if (MDNS.begin(_config.mdnsName.c_str())) {
                log(LOG_INFO, "mDNS responder started: %s.local", _config.mdnsName.c_str());
            }
        }

        _state = STATE_CONNECTED;
        setLEDPattern(0, 0); // Solid on

        // Start minimal web server if reset is enabled
        startConnectedWebServer();

        if (_onConnectedCallback) {
            _onConnectedCallback();
        }
    } else {
        _state = STATE_RETRY_WAIT;
        _lastRetryTime = millis();
    }
}

void ESP32_WiFiProvisioner::handleStateConnected() {
    // Client handling
    if (_webServer) {
        _webServer->handleClient();
    }

    // Check if still connected
    if (WiFi.status() != WL_CONNECTED) {
        log(LOG_ERROR, "WiFi connection lost");
        _retryCount = 0;
        _state = STATE_CONNECTING;
        setLEDPattern(100, 900);
    }
}

void ESP32_WiFiProvisioner::handleStateRetryWait() {
    if (millis() - _lastRetryTime >= _config.retryDelay) {
        _retryCount++;

        log(LOG_INFO, "Retry %d/%d", _retryCount, _config.maxRetries);

        if (_retryCount >= _config.maxRetries) {
            log(LOG_ERROR, "Max retries exceeded");

            if (_onFailedCallback) {
                _onFailedCallback(_retryCount);
            }

            if (_config.autoWipeOnMaxRetries) {
                log(LOG_INFO, "Auto-wiping credentials");
                clearAllCredentials();
                _state = STATE_PROVISIONING;
            } else {
                // Keep retrying
                _retryCount = 0;
                _state = STATE_CONNECTING;
            }
        } else {
            _state = STATE_CONNECTING;
        }
    }
}

void ESP32_WiFiProvisioner::handleStateProvisioning() {
    startProvisioningMode();
    _state = STATE_PROVISIONING_ACTIVE;
}

void ESP32_WiFiProvisioner::handleStateProvisioningActive() {
    // Handle DNS requests
    if (_dnsServer) {
        _dnsServer->processNextRequest();
    }

    // Handle web requests
    if (_webServer) {
        _webServer->handleClient();
    }

    // Check timeout
    if (_config.apTimeout > 0 && millis() - _apStartTime >= _config.apTimeout) {
        log(LOG_INFO, "AP timeout reached");
        stopProvisioningMode();

        // Retry connection if we have credentials
        if (_storedSSID.length() > 0) {
            _state = STATE_CONNECTING;
        }
    }
}

// ===== Connection =====

bool ESP32_WiFiProvisioner::connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(_storedSSID.c_str(), _storedPassword.c_str());

    unsigned long startAttempt = millis();
    const unsigned long timeout = 10000; // 10 second timeout per attempt

    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < timeout) {
        delay(100);
    }

    return WiFi.status() == WL_CONNECTED;
}

void ESP32_WiFiProvisioner::disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

// ===== Provisioning =====

void ESP32_WiFiProvisioner::startProvisioningMode() {
    log(LOG_INFO, "Starting provisioning mode");

    // Stop any existing connection
    disconnectWiFi();

    // Stop any existing webserver
    stopWebServer();

    // Create unique AP name
    String apName = _config.apName + "-" + getMACAddress().substring(9);
    apName.replace(":", "");

    // Start AP
    WiFi.mode(WIFI_AP);

    if (_config.apPassword.length() > 0) {
        WiFi.softAP(apName.c_str(), _config.apPassword.c_str());
        log(LOG_INFO, "AP started: %s (password protected)", apName.c_str());
    } else {
        WiFi.softAP(apName.c_str());
        log(LOG_INFO, "AP started: %s (open network)", apName.c_str());
    }

    IPAddress apIP = WiFi.softAPIP();
    log(LOG_INFO, "AP IP: %s", apIP.toString().c_str());

    // Start DNS server for captive portal
    if (!_dnsServer) {
        _dnsServer = new DNSServer();
    }
    _dnsServer->start(DNS_PORT, "*", apIP);

    // Load reset password if needed
    if (_config.httpResetAuthRequired) {
        loadResetPassword();
    }

    // Start web server
    setupWebServerProvisioningMode();

    _apStartTime = millis();
    setLEDPattern(100, 100); // Fast blink

    if (_onAPModeCallback) {
        _onAPModeCallback(apName.c_str(), apIP.toString().c_str());
    }
}

void ESP32_WiFiProvisioner::stopProvisioningMode() {
    log(LOG_INFO, "Stopping provisioning mode");

    if (_dnsServer) {
        _dnsServer->stop();
    }

    stopWebServer();

    WiFi.softAPdisconnect(true);
}

void ESP32_WiFiProvisioner::setupWebServerProvisioningMode() {
    if (!_webServer) {
        _webServer = new WebServer(WEB_SERVER_PORT);
    }

    _webServer->on("/", HTTP_GET, staticHandleRoot);
    _webServer->on("/scan", HTTP_GET, staticHandleScan);
    _webServer->on("/save", HTTP_POST, staticHandleSave);

    // This avoid captive portals redirect after form submission
    _webServer->on("/save", HTTP_GET, staticHandleSaveGet);

    if (_config.httpResetEnabled) {
        _webServer->on("/reset", HTTP_POST, staticHandleReset);
    }

    _webServer->onNotFound(staticHandleNotFound);

    _webServer->begin();
    log(LOG_INFO, "Web server started on port %d", WEB_SERVER_PORT);
}

// ===== Web Server Handlers =====

void ESP32_WiFiProvisioner::handleRoot() {
    String html = generateHTML();
    _webServer->send(200, "text/html", html);
}

void ESP32_WiFiProvisioner::handleScan() {
    log(LOG_DEBUG, "Scanning for networks...");

    int n = WiFi.scanNetworks();
    String json = "[";

    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
        json += "}";
    }

    json += "]";

    WiFi.scanDelete();

    _webServer->send(200, "application/json", json);
}

void ESP32_WiFiProvisioner::handleSave() {
    String ssid = _webServer->arg("ssid");
    String password = _webServer->arg("password");
    String resetPwd = _webServer->arg("reset_password");

    log(LOG_INFO, "Received configuration: SSID=%s", ssid.c_str());

    if (ssid.length() == 0) {
        _webServer->send(400, "text/plain", "SSID is required");
        return;
    }

    // Save WiFi credentials
    if (!saveCredentials(ssid, password)) {
        _webServer->send(500, "text/plain", "Failed to save credentials");
        return;
    }

    // Save reset password if authentication is enabled
    if (_config.httpResetAuthRequired && resetPwd.length() > 0) {
        saveResetPassword(resetPwd);
    }

    _webServer->send(200, "text/plain", "Configuration saved. Rebooting...");

    log(LOG_INFO, "Configuration saved, rebooting in 2 seconds");
    delay(2000);
    ESP.restart();
}

void ESP32_WiFiProvisioner::handleSaveGet() {
    log(LOG_INFO, "Sending saved status to the client");
    _webServer->send(200, "text/plain", "OK");
}

void ESP32_WiFiProvisioner::handleReset() {
    if (!_config.httpResetEnabled) {
        _webServer->send(403, "text/plain", "Reset disabled");
        return;
    }

    // Check authentication if required
    if (_config.httpResetAuthRequired) {
        String password = _webServer->arg("password");

        if (password.length() == 0) {
            _webServer->send(401, "text/plain", "Password required");
            return;
        }

        if (!verifyPassword(password, _resetPassword)) {
            log(LOG_ERROR, "Reset authentication failed");
            _webServer->send(401, "text/plain", "Invalid password");
            return;
        }
    }

    log(LOG_INFO, "HTTP reset triggered");

    _webServer->send(200, "text/plain", "Resetting device...");

    delay(1000);
    performReset("HTTP reset");
}

void ESP32_WiFiProvisioner::handleNotFound() {
    log(LOG_DEBUG, "NotFound: %s %s",
        _webServer->method() == HTTP_GET ? "GET" : "POST",
        _webServer->uri().c_str());

    // Captive portal redirect
    _webServer->sendHeader("Location", "/", true);
    _webServer->send(302, "text/plain", "");
}

// Static web server handlers
void ESP32_WiFiProvisioner::staticHandleRoot() {
    if (_instance) _instance->handleRoot();
}

void ESP32_WiFiProvisioner::staticHandleScan() {
    if (_instance) _instance->handleScan();
}

void ESP32_WiFiProvisioner::staticHandleSave() {
    if (_instance) _instance->handleSave();
}

void ESP32_WiFiProvisioner::staticHandleSaveGet() {
    if (_instance) _instance->handleSaveGet();
}

void ESP32_WiFiProvisioner::staticHandleReset() {
    if (_instance) _instance->handleReset();
}

void ESP32_WiFiProvisioner::staticHandleNotFound() {
    if (_instance) _instance->handleNotFound();
}

// ===== Reset Mechanisms =====

void ESP32_WiFiProvisioner::checkHardwareReset() {
    bool buttonState = digitalRead(_config.resetButtonPin);
    bool isPressed = _config.resetButtonActiveLow ? (buttonState == LOW) : (buttonState == HIGH);

    if (isPressed && !_buttonPressed) {
        // Button just pressed
        _buttonPressed = true;
        _buttonPressStart = millis();
    } else if (isPressed && _buttonPressed) {
        // Button still pressed, check duration
        if (millis() - _buttonPressStart >= _config.resetButtonDuration) {
            log(LOG_INFO, "Hardware reset button held for %lu ms", _config.resetButtonDuration);
            performReset("Hardware button");
        }
    } else if (!isPressed && _buttonPressed) {
        // Button released before threshold
        _buttonPressed = false;
    }
}

void ESP32_WiFiProvisioner::checkDoubleReboot() {
    if (!_preferences.begin(NVS_NAMESPACE, false)) {
        return;
    }

    uint32_t bootCount = _preferences.getUInt(NVS_BOOT_COUNT, 0);
    uint32_t lastBootTime = _preferences.getUInt(NVS_BOOT_TIME, 0);
    uint32_t currentTime = millis();

    bootCount++;
    _preferences.putUInt(NVS_BOOT_COUNT, bootCount);
    _preferences.putUInt(NVS_BOOT_TIME, currentTime);

    log(LOG_DEBUG, "Boot count: %u, last boot: %u ms ago", bootCount, currentTime - lastBootTime);

    // Check if this is a double reboot
    if (bootCount >= 2 && (currentTime - lastBootTime) < _config.doubleRebootWindow) {
        log(LOG_INFO, "Double reboot detected, clearing credentials");
        clearAllCredentials();
        _preferences.putUInt(NVS_BOOT_COUNT, 0);
    }

    _preferences.end();

    // Schedule boot count reset
    // This would normally be done with a timer, but we'll handle it in the main loop
}

void ESP32_WiFiProvisioner::performReset(const char* reason) {
    log(LOG_INFO, "Performing reset: %s", reason);

    if (_onResetCallback) {
        _onResetCallback();
    }

    clearAllCredentials();
    delay(500);
    ESP.restart();
}

// ===== Web server controls =====

void ESP32_WiFiProvisioner::startConnectedWebServer() {
    // Only start if software reset is enabled
    if (!_config.httpResetEnabled) {
        log(LOG_DEBUG, "HTTP reset disabled, not starting connected web server");
        return;
    }

    if (_webServer) {
        return; // Already running
    }

    _webServer = new WebServer(WEB_SERVER_PORT);

    // Reset endpoint
    _webServer->on("/reset", HTTP_POST, staticHandleReset);

    // Optional: simple status endpoint (very useful)
    _webServer->on("/status", HTTP_GET, [this]() {
        String json = "{";
        json += "\"state\":\"connected\",";
        json += "\"ssid\":\"" + _storedSSID + "\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
        json += "}";
        _webServer->send(200, "application/json", json);
    });

    _webServer->begin();

    log(LOG_INFO, "Connected-mode web server started on port %d", WEB_SERVER_PORT);
}

void ESP32_WiFiProvisioner::stopWebServer() {
    if (_webServer) {
        _webServer->stop();
        delete _webServer;
        _webServer = nullptr;
        log(LOG_DEBUG, "Web server stopped");
    }
}

// ===== UX =====

void ESP32_WiFiProvisioner::updateLED() {
    // LED is controlled by pattern set in setLEDPattern
    // This is a simple implementation; could be enhanced with more patterns

    static uint32_t onTime = 0;
    static uint32_t offTime = 0;

    if (_state == STATE_PROVISIONING || _state == STATE_PROVISIONING_ACTIVE) {
        onTime = 100;
        offTime = 100;
    } else if (_state == STATE_CONNECTING || _state == STATE_RETRY_WAIT) {
        onTime = 100;
        offTime = 900;
    } else if (_state == STATE_CONNECTED) {
        onTime = 0;
        offTime = 0;
        digitalWrite(_config.ledPin, _config.ledActiveLow ? LOW : HIGH);
        return;
    } else {
        digitalWrite(_config.ledPin, _config.ledActiveLow ? HIGH : LOW);
        return;
    }

    unsigned long now = millis();
    uint32_t period = onTime + offTime;

    if (period == 0) {
        // Solid on
        digitalWrite(_config.ledPin, _config.ledActiveLow ? LOW : HIGH);
        return;
    }

    unsigned long phase = now % period;
    bool shouldBeOn = phase < onTime;

    if (shouldBeOn) {
        digitalWrite(_config.ledPin, _config.ledActiveLow ? LOW : HIGH);
    } else {
        digitalWrite(_config.ledPin, _config.ledActiveLow ? HIGH : LOW);
    }
}

void ESP32_WiFiProvisioner::setLEDPattern(uint32_t onTime, uint32_t offTime) {
    // This is handled directly in updateLED for simplicity
    // A more sophisticated implementation would store patterns
}

// ===== Utilities =====

void ESP32_WiFiProvisioner::log(LogLevel level, const char* format, ...) {
    if (level > _config.logLevel) return;

    const char* levelStr;
    switch (level) {
        case LOG_ERROR: levelStr = "ERROR"; break;
        case LOG_INFO:  levelStr = "INFO "; break;
        case LOG_DEBUG: levelStr = "DEBUG"; break;
        default:        levelStr = "     "; break;
    }

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("[WiFiProv][%s] %s\n", levelStr, buffer);
}

String ESP32_WiFiProvisioner::getMACAddress() {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(macStr);
}

String ESP32_WiFiProvisioner::hashPassword(const String& password) {
    // Simple SHA-256 hash
    byte shaResult[32];

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)password.c_str(), password.length());
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    String hash = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", shaResult[i]);
        hash += hex;
    }

    return hash;
}

bool ESP32_WiFiProvisioner::verifyPassword(const String& password, const String& hash) {
    return hashPassword(password) == hash;
}

String ESP32_WiFiProvisioner::generateHTML() {
    String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Configuration</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 500px;
            width: 100%;
            overflow: hidden;
        }

        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }

        .header h1 {
            font-size: 24px;
            font-weight: 600;
            margin-bottom: 8px;
        }

        .header p {
            font-size: 14px;
            opacity: 0.9;
        }

        .content {
            padding: 30px;
        }

        .form-group {
            margin-bottom: 20px;
        }

        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            color: #333;
            font-size: 14px;
        }

        input, select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 14px;
            transition: border-color 0.3s;
        }

        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }

        button {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 20px rgba(102, 126, 234, 0.4);
        }

        button:active {
            transform: translateY(0);
        }

        button:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none;
        }

        .scan-btn {
            background: #f5f5f5;
            color: #333;
            margin-bottom: 15px;
        }

        .scan-btn:hover {
            background: #e0e0e0;
            box-shadow: 0 4px 12px rgba(0,0,0,0.1);
        }

        .network-list {
            max-height: 200px;
            overflow-y: auto;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            margin-bottom: 20px;
        }

        .network-item {
            padding: 12px;
            border-bottom: 1px solid #f0f0f0;
            cursor: pointer;
            transition: background 0.2s;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .network-item:last-child {
            border-bottom: none;
        }

        .network-item:hover {
            background: #f8f8f8;
        }

        .network-item.selected {
            background: #e8edff;
            color: #667eea;
        }

        .signal {
            font-size: 12px;
            opacity: 0.7;
        }

        .lock-icon::before {
            content: "🔒";
            margin-left: 8px;
        }

        .status {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 20px;
            font-size: 14px;
            text-align: center;
        }

        .status.info {
            background: #e3f2fd;
            color: #1976d2;
        }

        .status.success {
            background: #e8f5e9;
            color: #388e3c;
        }

        .status.error {
            background: #ffebee;
            color: #d32f2f;
        }

        .hidden {
            display: none;
        }

        .advanced {
            margin-top: 20px;
            padding-top: 20px;
            border-top: 2px solid #f0f0f0;
        }

        .toggle-advanced {
            background: none;
            color: #667eea;
            border: 2px solid #667eea;
            margin-bottom: 20px;
        }

        .toggle-advanced:hover {
            background: #667eea;
            color: white;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>WiFi Configuration</h1>
            <p>Configure your device's network connection</p>
        </div>

        <div class="content">
            <div id="status" class="status hidden"></div>

            <button class="scan-btn" onclick="scanNetworks();">Scan for Networks</button>

            <div id="networks" class="network-list hidden"></div>

            <form id="configForm" onsubmit="return saveConfig(event);">
                <div class="form-group">
                    <label for="ssid">Network Name (SSID)</label>
                    <input type="text" id="ssid" name="ssid" required placeholder="Enter or select network">
                </div>

                <div class="form-group">
                    <label for="password">Password</label>
                    <input type="password" id="password" name="password" placeholder="Leave blank for open networks">
                </div>

)";

    // Add reset password field if authentication is enabled
    if (_config.httpResetAuthRequired) {
        html += R"(
                <button type="button" class="toggle-advanced" onclick="toggleAdvanced();">Advanced Options</button>

                <div id="advanced" class="advanced hidden">
                    <div class="form-group">
                        <label for="reset_password">Reset Password (Optional)</label>
                        <input type="password" id="reset_password" name="reset_password" placeholder="For remote device reset">
                    </div>
                </div>
)";
    }

    html += R"(
                <button type="submit" id="submitBtn">Save Configuration</button>
            </form>
        </div>
    </div>

    <script>
        let networks = [];

        function showStatus(message, type) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.className = 'status ' + type;
            status.classList.remove('hidden');
        }

        function hideStatus() {
            document.getElementById('status').classList.add('hidden');
        }

        function toggleAdvanced() {
            const advanced = document.getElementById('advanced');
            advanced.classList.toggle('hidden');
        }

        function scanNetworks() {
            showStatus('Scanning for networks...', 'info');

            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    networks = data;
                    displayNetworks(data);
                    hideStatus();
                })
                .catch(error => {
                    showStatus('Scan failed. Please try again.', 'error');
                });
        }

        function displayNetworks(networks) {
            const container = document.getElementById('networks');

            if (networks.length === 0) {
                container.innerHTML = '<div class="network-item">No networks found</div>';
            } else {
                container.innerHTML = networks.map((network, index) => {
                    const signal = network.rssi;
                    const bars = signal > -50 ? '▂▄▆█' : signal > -60 ? '▂▄▆' : signal > -70 ? '▂▄' : '▂';
                    const lock = network.secure ? '<span class="lock-icon"></span>' : '';

                    return `
                        <div class="network-item" onclick="selectNetwork(${index});">
                            <span>${network.ssid}</span>
                            <span class="signal">${bars} ${lock}</span>
                        </div>
                    `;
                }).join('');
            }

            container.classList.remove('hidden');
        }

        function selectNetwork(index) {
            const network = networks[index];
            document.getElementById('ssid').value = network.ssid;

            // Highlight selected
            document.querySelectorAll('.network-item').forEach((item, i) => {
                if (i === index) {
                    item.classList.add('selected');
                } else {
                    item.classList.remove('selected');
                }
            });

            // Focus password field
            document.getElementById('password').focus();
        }

        function saveConfig(event) {
            // Prevent the form to trigger default behaviour (pt. 1)
            event.preventDefault();

            const formData = new FormData(event.target);
            const submitBtn = document.getElementById('submitBtn');

            submitBtn.disabled = true;
            submitBtn.textContent = 'Saving...';
            showStatus('Saving configuration...', 'info');

            fetch('/save', {
                method: 'POST',
                body: new URLSearchParams(formData)
            })
            .then(response => {
                if (response.ok) {
                    showStatus('Configuration saved! Device is rebooting...', 'success');
                    setTimeout(() => {
                        showStatus('You can close this page and reconnect to your network.', 'info');
                    }, 3000);
                } else {
                    throw new Error('Save failed');
                }
            })
            .catch(error => {
                showStatus('Failed to save configuration. Please try again.', 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = 'Save Configuration';
            });

            // Prevent the form to trigger default behaviour (pt. 2)
            return false;
        }

        // Auto-scan on load
        window.addEventListener('load', function() {
            setTimeout(scanNetworks, 500);
        });
    </script>
</body>
</html>
)";

    return html;
}