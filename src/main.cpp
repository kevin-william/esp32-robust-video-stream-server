/*
 * ESP32-CAM Robust Video Stream Server
 * 
 * A complete ESP32-CAM project with:
 * - Multi-core FreeRTOS task management
 * - Configuration persistence (SD Card / NVS)
 * - Captive portal for WiFi setup
 * - REST API for camera control
 * - MJPEG streaming
 * - OTA updates
 * 
 * Author: ESP32-CAM Project
 * License: Apache-2.0
 */

#include <Arduino.h>
#include "app.h"
#include "config.h"
#include "camera_pins.h"
#include "storage.h"
#include "captive_portal.h"
#include "web_server.h"

// Global variables
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t watchdogTaskHandle = NULL;
TaskHandle_t sdTaskHandle = NULL;

SemaphoreHandle_t cameraMutex = NULL;
SemaphoreHandle_t configMutex = NULL;
QueueHandle_t eventQueue = NULL;

bool camera_initialized = false;
bool camera_sleeping = false;
unsigned long camera_init_time = 0;
unsigned long system_start_time = 0;
bool ap_mode_active = false;
bool wifi_connected = false;

SystemConfig g_config;
bool g_config_loaded = false;

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println("ESP32-CAM Robust Video Stream Server");
    Serial.println("====================================");
    
    system_start_time = millis();
    
    // Initialize synchronization primitives
    cameraMutex = xSemaphoreCreateMutex();
    configMutex = xSemaphoreCreateMutex();
    eventQueue = xQueueCreate(10, sizeof(Event));
    
    if (!cameraMutex || !configMutex || !eventQueue) {
        Serial.println("ERROR: Failed to create synchronization primitives");
        ESP.restart();
    }
    
    // Initialize GPIO 0 for factory reset button
    pinMode(0, INPUT_PULLUP);
    Serial.println("Factory reset: Hold GPIO 0 button for 5 seconds");
    
    // Initialize LED
    initLED();
    setLED(0); // LED off initially
    
    // Try to mount SD card (optional - system works without it)
    bool sdMounted = initSDCard();
    if (sdMounted) {
        Serial.println("SD Card mounted successfully");
    } else {
        Serial.println("SD Card not available - using NVS only (this is normal)");
    }
    
    // Load configuration
    setDefaultConfiguration();
    if (!loadConfiguration()) {
        Serial.println("No valid configuration found, using defaults");
    } else {
        Serial.println("Configuration loaded successfully");
        g_config_loaded = true;
    }
    
    // Print memory info
    printMemoryInfo();
    
    // Try to connect to saved WiFi networks
    bool connected = tryConnectSavedNetworks();
    
    if (!connected) {
        Serial.println("Could not connect to any saved network");
        Serial.println("Starting captive portal for WiFi configuration");
        
        if (startCaptivePortal()) {
            ap_mode_active = true;
            Serial.println("Captive portal started successfully");
            Serial.print("Connect to AP: ");
            Serial.println(DEFAULT_AP_SSID);
            Serial.println("Navigate to http://192.168.4.1 to configure");
        } else {
            Serial.println("ERROR: Failed to start captive portal");
        }
    } else {
        wifi_connected = true;
        Serial.println("Connected to WiFi successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        
        // Initialize camera only after WiFi connection
        Serial.println("Initializing camera...");
        if (initCamera()) {
            Serial.println("Camera initialized successfully!");
            camera_initialized = true;
        } else {
            Serial.println("ERROR: Camera initialization failed - check connections");
            camera_initialized = false;
        }
    }
    
    // Initialize web server (always needed for captive portal or normal operation)
    initWebServer();
    Serial.println("Web server started");
    
    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        cameraTask,
        "CameraTask",
        8192,
        NULL,
        CAMERA_TASK_PRIORITY,
        &cameraTaskHandle,
        CAMERA_CORE
    );
    
    xTaskCreatePinnedToCore(
        webServerTask,
        "WebServerTask",
        8192,
        NULL,
        WEB_TASK_PRIORITY,
        &webServerTaskHandle,
        WEB_CORE
    );
    
    xTaskCreatePinnedToCore(
        watchdogTask,
        "WatchdogTask",
        4096,
        NULL,
        WATCHDOG_TASK_PRIORITY,
        &watchdogTaskHandle,
        WEB_CORE
    );
    
    Serial.println("All tasks created successfully");
    Serial.println("System ready!");
    Serial.println("====================================");
    
    printMemoryInfo();
}

void loop() {
    // Check for factory reset button (GPIO 0 held for 5 seconds)
    static unsigned long button_press_start = 0;
    static bool button_was_pressed = false;
    
    if (digitalRead(0) == LOW) {  // GPIO 0 is pulled low (button pressed)
        if (!button_was_pressed) {
            button_press_start = millis();
            button_was_pressed = true;
        } else if (millis() - button_press_start > 5000) {
            Serial.println("========================================");
            Serial.println("FACTORY RESET - Button held for 5 seconds");
            Serial.println("  Resetting to factory defaults...");
            resetConfiguration();
            Serial.println("  Configuration cleared!");
            Serial.println("  Restarting in captive portal mode...");
            Serial.println("========================================");
            delay(1000);
            ESP.restart();
        }
    } else {
        button_was_pressed = false;
    }
    
    // Handle captive portal DNS if in AP mode
    if (ap_mode_active) {
        handleCaptivePortal();
        
        // No timeout - keep captive portal active until WiFi is configured
        // This ensures the device remains accessible for configuration
    }
    
    // Process events from queue
    Event event;
    if (xQueueReceive(eventQueue, &event, 0) == pdTRUE) {
        switch (event.type) {
            case EVENT_WIFI_CONNECTED:
                wifi_connected = true;
                if (ap_mode_active) {
                    Serial.println("========================================");
                    Serial.println("WiFi Connection Successful!");
                    Serial.println("  Stopping captive portal...");
                    stopCaptivePortal();
                    ap_mode_active = false;
                    
                    // Initialize camera now that we have WiFi connection
                    if (!camera_initialized) {
                        Serial.println("  Initializing camera...");
                        if (initCamera()) {
                            Serial.println("✓ Camera initialized successfully!");
                            Serial.println("  System is now fully operational");
                            camera_initialized = true;
                        } else {
                            Serial.println("✗ Camera initialization failed!");
                            Serial.println("  Check camera connections and power supply");
                            camera_initialized = false;
                        }
                    }
                    Serial.println("========================================");
                } else {
                    Serial.println("WiFi connected event received");
                }
                break;
                
            case EVENT_WIFI_DISCONNECTED:
                wifi_connected = false;
                Serial.println("WiFi disconnected event received");
                Serial.println("Note: Captive portal will NOT restart automatically");
                Serial.println("Device will attempt to reconnect to known networks");
                // Do not restart captive portal - maintain current operation mode
                break;
                
            case EVENT_CONFIG_UPDATED:
                Serial.println("Configuration updated");
                saveConfiguration();
                break;
                
            case EVENT_RESTART_REQUESTED:
                Serial.println("Restart requested, rebooting in 2 seconds...");
                delay(2000);
                ESP.restart();
                break;
                
            default:
                break;
        }
    }
    
    delay(10); // Small delay to prevent watchdog issues
}
