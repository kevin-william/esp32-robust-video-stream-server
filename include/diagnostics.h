#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Diagnostic counters
struct DiagnosticCounters {
    unsigned long frame_count;
    unsigned long frame_errors;
    unsigned long last_frame_time;
    unsigned long fps_calculation_start;
    float current_fps;
    unsigned long total_bytes_sent;
    unsigned long wifi_reconnects;
    unsigned long task_overruns;
};

extern DiagnosticCounters g_diag;

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
