/*
 * ============================================================================
 * ESP32-CAM Motion Monitoring Task
 * ============================================================================
 *
 * This module implements the motion-activated recording functionality.
 * When motion is detected, it starts the camera and records video for
 * a configurable duration, extending the recording if motion continues.
 *
 * Features:
 * - State machine: IDLE -> MOTION_DETECTED -> RECORDING -> IDLE
 * - Camera auto-start on motion detection
 * - Recording duration reset on continuous motion
 * - Automatic camera shutdown when recording ends
 *
 * Author: ESP32-CAM Project
 * ============================================================================
 */

#include <esp_log.h>

#include "app.h"
#include "config.h"
#include "motion_sensor.h"
#include "storage.h"

static const char* TAG = "MOTION_TASK";

// Recording state machine
enum RecordingState {
    STATE_IDLE,            // Waiting for motion
    STATE_MOTION_DETECTED, // Motion detected, starting camera
    STATE_RECORDING,       // Active recording
    STATE_STOPPING         // Finalizing recording
};

static RecordingState current_state = STATE_IDLE;
static unsigned long recording_start_time = 0;
static unsigned long last_motion_reset_time = 0;
static char current_video_filename[64];

/**
 * Motion Monitoring Task
 * Runs on Core 1 (APP_CPU) for camera access
 * Monitors motion sensor and manages video recording
 */
void motionMonitoringTask(void* parameter) {
    ESP_LOGI(TAG, "Motion monitoring task started on core %d", xPortGetCoreID());
    Serial.println("Motion monitoring task started on core " + String(xPortGetCoreID()));

    const TickType_t xFrequency = pdMS_TO_TICKS(100);  // 10 Hz check rate
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    unsigned long frame_interval_ms = 200;  // ~5 FPS for recording
    unsigned long last_frame_time = 0;

    while (true) {
        // Only run if motion monitoring is enabled and SD card is mounted
        if (!g_config.motion.enabled || !isSDCardMounted()) {
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));  // Check every second
            continue;
        }

        unsigned long now = millis();

        // State machine
        switch (current_state) {
            case STATE_IDLE:
                // Wait for motion detection
                if (isMotionDetected()) {
                    ESP_LOGI(TAG, "Motion detected! Transitioning to recording mode");
                    Serial.println("MOTION DETECTED - Starting camera and recording");
                    
                    current_state = STATE_MOTION_DETECTED;
                    recording_start_time = now;
                    last_motion_reset_time = now;
                    
                    // Generate filename with timestamp
                    snprintf(current_video_filename, sizeof(current_video_filename),
                             "/recordings/motion_%lu.mjpeg", now / 1000);
                }
                break;

            case STATE_MOTION_DETECTED:
                // Initialize camera if not already initialized
                if (!camera_initialized) {
                    ESP_LOGI(TAG, "Initializing camera for recording");
                    if (initCamera()) {
                        ESP_LOGI(TAG, "Camera initialized successfully");
                        camera_initialized = true;
                    } else {
                        ESP_LOGE(TAG, "Failed to initialize camera, returning to IDLE");
                        current_state = STATE_IDLE;
                        break;
                    }
                }
                
                // Start recording
                if (initVideoRecording(current_video_filename)) {
                    ESP_LOGI(TAG, "Video recording started: %s", current_video_filename);
                    current_state = STATE_RECORDING;
                    last_frame_time = 0;  // Force immediate first frame
                } else {
                    ESP_LOGE(TAG, "Failed to start recording, returning to IDLE");
                    current_state = STATE_IDLE;
                }
                break;

            case STATE_RECORDING:
                // Check for new motion to reset timer
                if (isMotionDetected()) {
                    ESP_LOGI(TAG, "Motion continues - extending recording");
                    last_motion_reset_time = now;
                }
                
                // Capture and write frames at regular intervals
                if (now - last_frame_time >= frame_interval_ms) {
                    camera_fb_t* fb = captureFrame();
                    if (fb) {
                        if (writeFrameToVideo(fb->buf, fb->len)) {
                            ESP_LOGD(TAG, "Frame written to video");
                        } else {
                            ESP_LOGW(TAG, "Failed to write frame to video");
                        }
                        releaseFrame(fb);
                    } else {
                        ESP_LOGW(TAG, "Failed to capture frame");
                    }
                    last_frame_time = now;
                }
                
                // Check if recording duration has elapsed without new motion
                unsigned long time_since_last_motion = now - last_motion_reset_time;
                unsigned long recording_duration_ms = g_config.motion.recording_duration_sec * 1000;
                
                if (time_since_last_motion >= recording_duration_ms) {
                    ESP_LOGI(TAG, "Recording duration elapsed, stopping recording");
                    current_state = STATE_STOPPING;
                }
                break;

            case STATE_STOPPING:
                // Finalize recording
                if (finalizeVideoRecording()) {
                    ESP_LOGI(TAG, "Recording finalized successfully");
                    Serial.printf("Recording saved: %s\n", current_video_filename);
                } else {
                    ESP_LOGW(TAG, "Failed to finalize recording properly");
                }
                
                // Deinitialize camera to save power
                ESP_LOGI(TAG, "Stopping camera to save power");
                deinitCamera();
                camera_initialized = false;
                
                // Return to idle state
                current_state = STATE_IDLE;
                ESP_LOGI(TAG, "Returning to IDLE state, waiting for next motion");
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
