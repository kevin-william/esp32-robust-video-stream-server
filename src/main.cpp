/*
 * ESP32-CAM Robust Video Stream Server
 *
 * A complete ESP32-CAM project with:
 * - Dual-core FreeRTOS task management (Core 0: WiFi/HTTP, Core 1: Camera)
 * - I2S parallel mode with DMA for camera
 * - 2 Frame Buffers in PSRAM for zero-copy optimization
 * - Configuration persistence (SD Card / NVS)
 * - Captive portal for WiFi setup
 * - REST API for camera control
 * - MJPEG streaming with power management
 * - Maximum energy efficiency
 *
 * Author: ESP32-CAM Project
 * License: Apache-2.0
 */

#include <Arduino.h>
#include <esp_log.h>

#include "app.h"
#include "camera_pins.h"
#include "captive_portal.h"
#include "config.h"
#include "storage.h"
#include "web_server.h"

static const char* TAG = "MAIN";

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
    Serial.println("====================================");
    Serial.println("ESP32-CAM Robust Video Stream Server");
    Serial.println("High Performance | Energy Efficient");
    Serial.println("====================================");

    ESP_LOGI(TAG, "System initialization started");
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);

    system_start_time = millis();

    ESP_LOGI(TAG, "Initializing synchronization primitives...");
    // Initialize synchronization primitives (Mutex for inter-core communication)
    cameraMutex = xSemaphoreCreateMutex();
    configMutex = xSemaphoreCreateMutex();
    eventQueue = xQueueCreate(10, sizeof(Event));

    if (!cameraMutex || !configMutex || !eventQueue) {
        ESP_LOGE(TAG, "Failed to create synchronization primitives");
        Serial.println("ERROR: Failed to create synchronization primitives");
        ESP.restart();
    }
    ESP_LOGI(TAG, "Synchronization primitives created successfully");

    // Initialize GPIO 0 for factory reset button
    // pinMode(0, INPUT_PULLUP);  // DISABLED - GPIO 0 detection issues
    // Serial.println("Factory reset: Hold GPIO 0 button for 5 seconds");

    // Initialize LED
    ESP_LOGI(TAG, "Initializing LED...");
    initLED();
    setLED(0);  // LED off initially

    // Try to mount SD card (optional - system works without it)
    ESP_LOGI(TAG, "Attempting to mount SD card...");
    bool sdMounted = initSDCard();
    if (sdMounted) {
        ESP_LOGI(TAG, "SD Card mounted successfully");
        Serial.println("SD Card mounted successfully");
    } else {
        ESP_LOGI(TAG, "SD Card not available - using NVS only");
        Serial.println("SD Card not available - using NVS only (this is normal)");
    }

    // Load configuration
    ESP_LOGI(TAG, "Loading configuration...");
    setDefaultConfiguration();
    if (!loadConfiguration()) {
        ESP_LOGI(TAG, "No valid configuration found, using defaults");
        Serial.println("No valid configuration found, using defaults");
    } else {
        ESP_LOGI(TAG, "Configuration loaded successfully");
        Serial.println("Configuration loaded successfully");
        g_config_loaded = true;
    }

    // Print memory info
    printMemoryInfo();

    ESP_LOGI(TAG, "Attempting to connect to saved WiFi networks...");
    // Try to connect to saved WiFi networks
    bool connected = tryConnectSavedNetworks();

    if (!connected) {
        ESP_LOGI(TAG, "Could not connect to any saved network");
        Serial.println("Could not connect to any saved network");
        Serial.println("Starting captive portal for WiFi configuration");

        if (startCaptivePortal()) {
            ap_mode_active = true;
            ESP_LOGI(TAG, "Captive portal started successfully");
            Serial.println("Captive portal started successfully");
            Serial.print("Connect to AP: ");
            Serial.println(DEFAULT_AP_SSID);
            Serial.println("Navigate to http://192.168.4.1 to configure");
        } else {
            ESP_LOGE(TAG, "Failed to start captive portal");
            Serial.println("ERROR: Failed to start captive portal");
        }
    } else {
        wifi_connected = true;
        ESP_LOGI(TAG, "Connected to WiFi successfully! IP: %s", WiFi.localIP().toString().c_str());
        Serial.println("Connected to WiFi successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        // Initialize camera only after WiFi connection
        ESP_LOGI(TAG, "Initializing camera subsystem...");
        Serial.println("Initializing camera...");
        if (initCamera()) {
            ESP_LOGI(TAG, "Camera initialized successfully!");
            Serial.println("Camera initialized successfully!");
            camera_initialized = true;
        } else {
            ESP_LOGE(TAG, "Camera initialization failed - check connections");
            Serial.println("ERROR: Camera initialization failed - check connections");
            camera_initialized = false;
        }
    }

    // Initialize web server (always needed for captive portal or normal operation)
    ESP_LOGI(TAG, "Initializing web server...");
    initWebServer();
    ESP_LOGI(TAG, "Web server started on port 80");
    Serial.println("Web server started");

    ESP_LOGI(TAG, "Creating FreeRTOS tasks with dual-core pinning...");
    // Create FreeRTOS tasks pinned to specific cores
    // Core 0 (PRO_CPU): Dedicated to WiFi stack and HTTP server
    // Core 1 (APP_CPU): Dedicated to camera capture and image processing

    xTaskCreatePinnedToCore(cameraTask, "CameraTask", 8192, NULL, CAMERA_TASK_PRIORITY,
                            &cameraTaskHandle,
                            CAMERA_CORE  // Core 1 (APP_CPU)
    );
    ESP_LOGI(TAG, "Camera task created on Core %d", CAMERA_CORE);

    xTaskCreatePinnedToCore(webServerTask, "WebServerTask", 8192, NULL, WEB_TASK_PRIORITY,
                            &webServerTaskHandle,
                            WEB_CORE  // Core 0 (PRO_CPU)
    );
    ESP_LOGI(TAG, "Web server task created on Core %d", WEB_CORE);

    xTaskCreatePinnedToCore(watchdogTask, "WatchdogTask", 4096, NULL, WATCHDOG_TASK_PRIORITY,
                            &watchdogTaskHandle,
                            WEB_CORE  // Core 0 (PRO_CPU)
    );
    ESP_LOGI(TAG, "Watchdog task created on Core %d", WEB_CORE);

    ESP_LOGI(TAG, "All FreeRTOS tasks created successfully");
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "Architecture:");
    ESP_LOGI(TAG, "  Core 0 (PRO_CPU): WiFi + HTTP Server");
    ESP_LOGI(TAG, "  Core 1 (APP_CPU): Camera + Image Processing");
    ESP_LOGI(TAG, "====================================");

    ESP_LOGI(TAG, "System initialization complete");

    printMemoryInfo();
}

void loop() {
    // Check for factory reset button (GPIO 0 held for 5 seconds)
    // DISABLED - GPIO 0 detection issues causing false triggers
    /*
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
    */

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

    delay(10);  // Small delay to prevent watchdog issues
}
