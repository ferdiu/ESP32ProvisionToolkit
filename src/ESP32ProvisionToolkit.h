/*
 * ESP32ProvisionToolkit.h
 * Production-ready Wi-Fi provisioning and recovery system for ESP32
 *
 * Features:
 * - Non-blocking captive portal configuration
 * - Multiple reset mechanisms (hardware, HTTP, authenticated)
 * - Persistent NVS storage
 * - Configurable retry and timeout logic
 * - Optional UX enhancements (LED, mDNS, double-reboot detection)
 * - Callback hooks for application integration
 *
 * Copyright (c) 2026
 * License: MIT
 */

#ifndef ESP32_WIFI_PROVISIONER_H
#define ESP32_WIFI_PROVISIONER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#include <functional>
#include <vector>

// Library version
#define WIFI_PROVISIONER_VERSION "1.0.1"

// Default configuration values
#define DEFAULT_AP_NAME "ESP32-Config"
#define DEFAULT_AP_PASSWORD ""  // Open network
#define DEFAULT_MAX_RETRIES 10
#define DEFAULT_RETRY_DELAY_MS 3000
#define DEFAULT_AP_TIMEOUT_MS 300000  // 5 minutes
#define DEFAULT_RESET_BUTTON_DURATION_MS 5000
#define DEFAULT_DOUBLE_REBOOT_WINDOW_MS 10000
#define DNS_PORT 53
#define WEB_SERVER_PORT 80

// Logging levels
enum LogLevel {
    LOG_NONE = 0,
    LOG_ERROR = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3
};

// Connection states
enum ProvisionerState {
    STATE_INIT,
    STATE_LOAD_CONFIG,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_RETRY_WAIT,
    STATE_PROVISIONING,
    STATE_PROVISIONING_ACTIVE
};

// Reset result codes
enum ResetResult {
    RESET_SUCCESS = 0,
    RESET_AUTH_FAILED = 1,
    RESET_DISABLED = 2,
    RESET_ERROR = 3
};

// Custom routes scopes
enum HttpRouteScope {
    ROUTE_PROVISIONING_ONLY,
    ROUTE_CONNECTED_ONLY,
    ROUTE_BOTH
};

// Route descriptor
typedef std::function<void(WebServer&)> HttpRouteHandler;

struct HttpRoute {
    String path;
    HTTPMethod method;
    HttpRouteHandler handler;
    HttpRouteScope scope;
    bool requiresAuth;
};

// Configuration structure
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

    // Constructor with defaults
    WiFiProvisionerConfig() :
        apName(DEFAULT_AP_NAME),
        apPassword(DEFAULT_AP_PASSWORD),
        apTimeout(DEFAULT_AP_TIMEOUT_MS),
        maxRetries(DEFAULT_MAX_RETRIES),
        retryDelay(DEFAULT_RETRY_DELAY_MS),
        autoWipeOnMaxRetries(true),
        hardwareResetEnabled(false),
        resetButtonPin(-1),
        resetButtonDuration(DEFAULT_RESET_BUTTON_DURATION_MS),
        resetButtonActiveLow(true),
        httpResetEnabled(false),
        httpResetAuthRequired(false),
        ledEnabled(false),
        ledPin(-1),
        ledActiveLow(false),
        mdnsEnabled(false),
        mdnsName("esp32"),
        doubleRebootDetectEnabled(false),
        doubleRebootWindow(DEFAULT_DOUBLE_REBOOT_WINDOW_MS),
        logLevel(LOG_INFO)
    {}
};

// Callback function types
typedef void (*WiFiConnectedCallback)();
typedef void (*WiFiFailedCallback)(uint8_t retryCount);
typedef void (*APModeCallback)(const char* ssid, const char* ip);
typedef void (*ResetCallback)();

class ESP32ProvisionToolkit {
public:
    ESP32ProvisionToolkit();
    ~ESP32ProvisionToolkit();

    // ===== Configuration API (Fluent Interface) =====

    // AP Settings
    ESP32ProvisionToolkit& setAPName(const String& name);
    ESP32ProvisionToolkit& setAPPassword(const String& password);
    ESP32ProvisionToolkit& setAPTimeout(uint32_t milliseconds);

    // Connection Settings
    ESP32ProvisionToolkit& setMaxRetries(uint8_t retries);
    ESP32ProvisionToolkit& setRetryDelay(uint32_t milliseconds);
    ESP32ProvisionToolkit& setAutoWipeOnMaxRetries(bool enable);

    // Hardware Reset
    ESP32ProvisionToolkit& enableHardwareReset(int8_t pin, uint32_t durationMs = DEFAULT_RESET_BUTTON_DURATION_MS, bool activeLow = true);
    ESP32ProvisionToolkit& disableHardwareReset();

    // Software Reset
    ESP32ProvisionToolkit& enableHttpReset(bool enable);
    ESP32ProvisionToolkit& enableAuthenticatedHttpReset(bool enable);

    // UX Features
    ESP32ProvisionToolkit& setLed(int8_t pin, bool activeLow = false);
    ESP32ProvisionToolkit& enableMDNS(const String& name);
    ESP32ProvisionToolkit& enableDoubleRebootDetect(uint32_t windowMs = DEFAULT_DOUBLE_REBOOT_WINDOW_MS);

    // Custom routes
    ESP32ProvisionToolkit& addHttpRoute(
        const String& path,
        HTTPMethod method,
        HttpRouteHandler handler,
        HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
        bool requiresAuth = false
    );
    ESP32ProvisionToolkit& addGet(
        const String& path,
        HttpRouteHandler handler,
        HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
        bool requiresAuth = false
    );
    ESP32ProvisionToolkit& addPost(
        const String& path,
        HttpRouteHandler handler,
        HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
        bool requiresAuth = false
    );
    ESP32ProvisionToolkit& addJsonRoute(
        const String& path,
        HTTPMethod method,
        std::function<String()> jsonProvider,
        HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
        bool requiresAuth = false
    );
    ESP32ProvisionToolkit& addGetJsonRoute(
        const String& path,
        std::function<String()> jsonProvider,
        HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
        bool requiresAuth = false
    );
    ESP32ProvisionToolkit& addPostJsonRoute(
        const String& path,
        std::function<String()> jsonProvider,
        HttpRouteScope scope = ROUTE_CONNECTED_ONLY,
        bool requiresAuth = false
    );

    // Logging
    ESP32ProvisionToolkit& setLogLevel(LogLevel level);

    // Callbacks
    ESP32ProvisionToolkit& onConnected(WiFiConnectedCallback callback);
    ESP32ProvisionToolkit& onFailed(WiFiFailedCallback callback);
    ESP32ProvisionToolkit& onAPMode(APModeCallback callback);
    ESP32ProvisionToolkit& onReset(ResetCallback callback);

    // ===== Core Control =====
    bool begin();
    void loop();
    void reset();  // Programmatic reset

    // ===== Status Query =====
    bool isConnected() const;
    bool isProvisioning() const;
    ProvisionerState getState() const;
    String getSSID() const;
    IPAddress getLocalIP() const;
    String getAPIP() const;

    // ===== Custom Route Introspection =====
    bool hasCustomRoutes() const;
    bool hasConnectedOnlyRoutes() const;
    bool hasProvisioningOnlyRoutes() const;

    // ===== Manual Control =====
    bool setCredentials(const String& ssid, const String& password, bool reboot = true);
    bool clearCredentials(bool reboot = true);

private:
    // Configuration
    WiFiProvisionerConfig _config;

    // State management
    ProvisionerState _state;
    uint8_t _retryCount;
    unsigned long _lastRetryTime;
    unsigned long _apStartTime;
    unsigned long _buttonPressStart;
    bool _buttonPressed;

    // Storage
    Preferences _preferences;
    String _storedSSID;
    String _storedPassword;
    String _resetPassword;

    // Network components
    DNSServer* _dnsServer;
    WebServer* _webServer;

    // Custom routes
    std::vector<HttpRoute> _customRoutes;

    // Callbacks
    WiFiConnectedCallback _onConnectedCallback;
    WiFiFailedCallback _onFailedCallback;
    APModeCallback _onAPModeCallback;
    ResetCallback _onResetCallback;

    // LED state
    unsigned long _lastLedToggle;
    bool _ledState;

    // ===== Internal Methods =====

    // Storage
    bool loadCredentials();
    bool saveCredentials(const String& ssid, const String& password);
    bool loadResetPassword();
    bool saveResetPassword(const String& password);
    void clearAllCredentials();

    // State machine
    void handleStateInit();
    void handleStateLoadConfig();
    void handleStateConnecting();
    void handleStateConnected();
    void handleStateRetryWait();
    void handleStateProvisioning();
    void handleStateProvisioningActive();

    // Connection
    bool connectToWiFi();
    void disconnectWiFi();

    // Provisioning
    void startProvisioningMode();
    void stopProvisioningMode();
    void setupWebServerProvisioningMode();
    void handleRoot();
    void handleScan();
    void handleSave();
    void handleSaveGet();
    void handleReset();
    void handleNotFound();

    // Reset mechanisms
    void checkHardwareReset();
    void checkDoubleReboot();
    void performReset(const char* reason);

    // Connected-mode web server
    void startConnectedWebServer();
    void stopWebServer();

    // Custom routes helpers
    void registerCustomRoutes(HttpRouteScope activeScope);

    // UX
    void updateLED();
    void setLEDPattern(uint32_t onTime, uint32_t offTime);

    // Utilities
    void log(LogLevel level, const char* format, ...);
    String getMACAddress();
    String hashPassword(const String& password);
    bool verifyPassword(const String& password, const String& hash);
    String generateHTML();

    // Static web server handlers (need access to instance)
    static ESP32ProvisionToolkit* _instance;
    static void staticHandleRoot();
    static void staticHandleScan();
    static void staticHandleSave();
    static void staticHandleSaveGet();
    static void staticHandleReset();
    static void staticHandleNotFound();
};

#endif // ESP32_WIFI_PROVISIONER_H