#ifndef APP_H
#define APP_H

#include <Arduino.h>

#include "camera_i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// Task handles
extern TaskHandle_t cameraTaskHandle;
extern TaskHandle_t webServerTaskHandle;
extern TaskHandle_t watchdogTaskHandle;
extern TaskHandle_t sdTaskHandle;
extern TaskHandle_t motionMonitoringTaskHandle;

// Synchronization primitives
extern SemaphoreHandle_t cameraMutex;
extern SemaphoreHandle_t configMutex;
extern QueueHandle_t eventQueue;

// Camera state
extern bool camera_initialized;
extern bool camera_sleeping;
extern unsigned long camera_init_time;

// Motion monitoring state
extern bool motion_monitoring_active;
extern bool motion_recording_active;

// System state
extern unsigned long system_start_time;
extern bool ap_mode_active;
extern bool wifi_connected;

// Event types for inter-task communication
enum EventType {
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_CONFIG_UPDATED,
    EVENT_CAMERA_ERROR,
    EVENT_SD_ERROR,
    EVENT_OTA_START,
    EVENT_OTA_PROGRESS,
    EVENT_OTA_COMPLETE,
    EVENT_RESTART_REQUESTED
};

struct Event {
    EventType type;
    int data;
    void* ptr;
};

// Task functions
void cameraTask(void* parameter);
void webServerTask(void* parameter);
void watchdogTask(void* parameter);
void sdCardTask(void* parameter);
void motionMonitoringTask(void* parameter);

// Camera functions
bool initCamera();
void deinitCamera();
bool reinitCamera();
camera_fb_t* captureFrame();
void releaseFrame(camera_fb_t* fb);

// LED functions
void initLED();
void setLED(uint8_t intensity);

// Memory management
void printMemoryInfo();
size_t getFreeHeap();
size_t getMinFreeHeap();

// Utility functions
unsigned long getUptimeSeconds();
const char* getResetReason();

#endif  // APP_H
