#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Camera diagnostic info
struct CameraDiagnostics {
    bool last_init_success;
    unsigned long last_init_attempt;
    unsigned long last_init_success_time;
    esp_err_t last_error_code;
    String last_error_msg;
    int init_attempts;
    int init_failures;
    bool sensor_detected;
    String sensor_id;
    int frames_flushed;
};

// Diagnostic counters
struct DiagnosticCounters {
    unsigned long frame_count;          // Frames in current FPS window
    unsigned long frame_errors;         // Total errors since boot
    unsigned long total_frames_sent;    // Total successful frames since boot
    unsigned long last_frame_time;
    unsigned long fps_calculation_start;
    float current_fps;
    unsigned long total_bytes_sent;
    unsigned long wifi_reconnects;
    unsigned long task_overruns;
};

extern DiagnosticCounters g_diag;
extern CameraDiagnostics g_camera_diag;

// Diagnostic functions
void initDiagnostics();
void updateFrameStats();
String getDiagnosticsJSON();

// CPU and memory utilities
float getCPUFrequency();
uint32_t getCore0IdleTime();
uint32_t getCore1IdleTime();
size_t getTaskStackHighWaterMark(TaskHandle_t task);

#endif // DIAGNOSTICS_H
