/*
 * ============================================================================
 * ESP32-CAM Motion Sensor Module (HC-SR501)
 * ============================================================================
 *
 * This module provides interface for HC-SR501 PIR motion sensor.
 * The sensor detects infrared changes indicating motion.
 *
 * Features:
 * - Interrupt-based detection for efficient CPU usage
 * - Debouncing to prevent false triggers
 * - Timer tracking for motion event management
 *
 * Author: ESP32-CAM Project
 * ============================================================================
 */

#include "motion_sensor.h"

#include <esp_log.h>

#include "camera_pins.h"

static const char* TAG = "MOTION";

// Motion sensor state variables
volatile bool motion_detected = false;
volatile unsigned long last_motion_time = 0;

// Debounce settings
#define MOTION_DEBOUNCE_MS 200
static volatile unsigned long last_trigger_time = 0;

/**
 * ISR - Interrupt Service Routine for motion detection
 * Called when PIR sensor pin changes state (rising edge = motion detected)
 * Must be in IRAM for fast execution
 */
void IRAM_ATTR motionDetectedISR() {
    unsigned long now = millis();
    
    // Debounce: ignore triggers within debounce period
    if (now - last_trigger_time < MOTION_DEBOUNCE_MS) {
        return;
    }
    
    last_trigger_time = now;
    motion_detected = true;
    last_motion_time = now;
}

/**
 * Initialize the HC-SR501 PIR motion sensor
 * Configures GPIO pin as input with interrupt on rising edge
 *
 * @return true if successful, false otherwise
 */
bool initMotionSensor() {
    ESP_LOGI(TAG, "Initializing HC-SR501 PIR motion sensor on GPIO %d", PIR_SENSOR_PIN);
    
    // Configure pin as input with internal pull-down
    pinMode(PIR_SENSOR_PIN, INPUT_PULLDOWN);
    
    // Attach interrupt on rising edge (motion detected)
    attachInterrupt(digitalPinToInterrupt(PIR_SENSOR_PIN), motionDetectedISR, RISING);
    
    // Reset state
    motion_detected = false;
    last_motion_time = 0;
    last_trigger_time = 0;
    
    ESP_LOGI(TAG, "Motion sensor initialized successfully");
    return true;
}

/**
 * Deinitialize the motion sensor
 * Detaches interrupt and resets state
 */
void deinitMotionSensor() {
    ESP_LOGI(TAG, "Deinitializing motion sensor");
    detachInterrupt(digitalPinToInterrupt(PIR_SENSOR_PIN));
    motion_detected = false;
    last_motion_time = 0;
}

/**
 * Check if motion is currently detected
 * Also clears the motion_detected flag after reading
 *
 * @return true if motion was detected since last check
 */
bool isMotionDetected() {
    bool detected = motion_detected;
    if (detected) {
        motion_detected = false;  // Clear flag after reading
        ESP_LOGI(TAG, "Motion detected!");
    }
    return detected;
}

/**
 * Reset the motion timer to current time
 * Used to extend recording when continuous motion is detected
 */
void resetMotionTimer() {
    last_motion_time = millis();
    ESP_LOGD(TAG, "Motion timer reset");
}

/**
 * Get time elapsed since last motion detection
 *
 * @return milliseconds since last motion, or ULONG_MAX if never detected
 */
unsigned long getTimeSinceLastMotion() {
    if (last_motion_time == 0) {
        return ULONG_MAX;
    }
    return millis() - last_motion_time;
}
