#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// Configuration file paths
#define CONFIG_FILE_PATH "/config/config.json"
#define CONFIG_BACKUP_PATH "/config/config.bak"

// Default AP mode settings
#define DEFAULT_AP_SSID "ESP32-CAM-Setup"
#define DEFAULT_AP_PASSWORD "12345678"
// Timeout removed - captive portal stays active until WiFi is configured

// Default camera settings
#define DEFAULT_FRAMESIZE FRAMESIZE_SVGA  // 800x600
#define DEFAULT_QUALITY 12                 // 0-63, lower is better
#define DEFAULT_BRIGHTNESS 0               // -2 to 2
#define DEFAULT_CONTRAST 0                 // -2 to 2
#define DEFAULT_SATURATION 0               // -2 to 2

// Memory and performance settings
#define MAX_WIFI_NETWORKS 3
#define CONFIG_JSON_SIZE 2048
#define STREAM_BOUNDARY "frame"
#define DEFAULT_FRAMERATE 10

// Task priorities and core affinity
#define CAMERA_TASK_PRIORITY 2
#define WEB_TASK_PRIORITY 2
#define SD_TASK_PRIORITY 1
#define WATCHDOG_TASK_PRIORITY 3

#define CAMERA_CORE 1
#define WEB_CORE 0
#define SD_CORE 0

// WiFi network configuration structure
struct WiFiNetwork {
    char ssid[32];
    char password[64];
    int priority;  // Higher is preferred
};

// Camera settings structure
struct CameraSettings {
    int framesize;     // framesize_t
    int quality;       // 0-63
    int brightness;    // -2 to 2
    int contrast;      // -2 to 2
    int saturation;    // -2 to 2
    int gainceiling;   // 0 to 6
    int colorbar;      // 0 or 1
    int awb;           // Auto white balance
    int agc;           // Auto gain control
    int aec;           // Auto exposure control
    int hmirror;       // Horizontal mirror
    int vflip;         // Vertical flip
    int awb_gain;      // AWB gain
    int agc_gain;      // AGC gain
    int aec_value;     // AEC value
    int special_effect;
    int wb_mode;
    int ae_level;
    int dcw;           // Downsize enable
    int bpc;           // Black pixel correct
    int wpc;           // White pixel correct
    int raw_gma;       // Gamma correction
    int lenc;          // Lens correction
    int led_intensity; // Flash LED 0-255
};

// System configuration structure
struct SystemConfig {
    WiFiNetwork networks[MAX_WIFI_NETWORKS];
    int network_count;
    CameraSettings camera;
    char admin_password_hash[65];  // SHA256 hash
    bool ota_enabled;
    char ota_password[32];
    int log_level;  // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG
    bool use_https;
    int server_port;
};

// Global configuration instance
extern SystemConfig g_config;
extern bool g_config_loaded;

// Configuration management functions
bool loadConfiguration();
bool saveConfiguration();
bool resetConfiguration();
bool validateConfiguration(const JsonDocument& doc);
void setDefaultConfiguration();

#endif // CONFIG_H
