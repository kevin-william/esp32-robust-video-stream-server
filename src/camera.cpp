#include "app.h"
#include "config.h"
#include "camera_pins.h"
#include "diagnostics.h"
#include <esp_camera.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <WiFi.h>

bool initCamera() {
    g_camera_diag.init_attempts++;
    g_camera_diag.last_init_attempt = millis();
    Serial.println("\n========================================");
    Serial.println("Camera Initialization");
    Serial.println("========================================");
    
    #ifdef CAMERA_MODEL_WROVER_KIT
        Serial.println("Camera Model: WROVER_KIT");
    #elif defined(CAMERA_MODEL_AI_THINKER)
        Serial.println("Camera Model: AI_THINKER");
    #else
        Serial.println("Camera Model: UNKNOWN");
    #endif
    
    Serial.println("\nPin Configuration:");
    Serial.printf("  PWDN:  GPIO %d\n", PWDN_GPIO_NUM);
    Serial.printf("  RESET: GPIO %d\n", RESET_GPIO_NUM);
    Serial.printf("  XCLK:  GPIO %d\n", XCLK_GPIO_NUM);
    Serial.printf("  SIOD:  GPIO %d\n", SIOD_GPIO_NUM);
    Serial.printf("  SIOC:  GPIO %d\n", SIOC_GPIO_NUM);
    Serial.printf("  Y9:    GPIO %d\n", Y9_GPIO_NUM);
    Serial.printf("  PCLK:  GPIO %d\n", PCLK_GPIO_NUM);
    Serial.println("========================================\n");
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // Init with balanced specs - performance vs resource usage
    if (psramFound()) {
        #ifdef CAMERA_MODEL_WROVER_KIT
            // WROVER has 8MB PSRAM - can handle higher resolution
            config.frame_size = FRAMESIZE_SVGA;  // 800x600 - better quality for WROVER
            config.jpeg_quality = 12;  // Better quality with more PSRAM
            config.fb_count = 3;  // Triple buffering for smoother streaming
        #else
            // AI-Thinker has 4MB PSRAM
            config.frame_size = FRAMESIZE_QVGA;  // 320x240 - optimized for streaming
            config.jpeg_quality = 18;  // Higher number = lower quality = faster processing
            config.fb_count = 3;  // Triple buffering for smoother streaming
        #endif
        config.fb_location = CAMERA_FB_IN_PSRAM;  // Explicitly allocate buffers in PSRAM
        config.grab_mode = CAMERA_GRAB_LATEST;  // Always get latest frame
        Serial.printf("PSRAM found, using resolution with %d frame buffers in PSRAM\n", config.fb_count);
    } else {
        config.frame_size = FRAMESIZE_QVGA;  // 320x240 - maximum performance
        config.jpeg_quality = 20;
        config.fb_count = 2;  // 2 buffers for systems without PSRAM
        config.fb_location = CAMERA_FB_IN_DRAM;  // Use DRAM when PSRAM not available
        config.grab_mode = CAMERA_GRAB_LATEST;  // Always get latest frame
        Serial.println("PSRAM not found, using QVGA (320x240) with 2 frame buffers in DRAM");
    }
    
    // Camera init
    Serial.println("Calling esp_camera_init()...");
    Serial.flush();
    
    esp_err_t err = esp_camera_init(&config);
    
    Serial.printf("esp_camera_init() returned: 0x%x\n", err);
    Serial.flush();
    
    if (err != ESP_OK) {
        Serial.printf("❌ Camera init failed with error 0x%x\n", err);
        
        // Record diagnostic info
        g_camera_diag.init_failures++;
        g_camera_diag.last_init_success = false;
        g_camera_diag.last_error_code = err;
        g_camera_diag.sensor_detected = false;
        
        // Print and store detailed error
        if (err == 0x105) {
            g_camera_diag.last_error_msg = "ESP_ERR_NOT_FOUND - Camera sensor not detected. Check: camera connection, pin config, power supply";
            Serial.println("  Error: ESP_ERR_NOT_FOUND - Camera sensor not detected");
            Serial.println("  Possible causes:");
            Serial.println("    - Camera not connected properly");
            Serial.println("    - Wrong pin configuration");
            Serial.println("    - Camera power issue");
        } else if (err == 0x20001 || err == 0x107) {
            g_camera_diag.last_error_msg = "I2C communication failed. Check: SIOD/SIOC pins (GPIO" + String(SIOD_GPIO_NUM) + "/" + String(SIOC_GPIO_NUM) + "), camera power";
            Serial.println("  Error: I2C communication failed");
            Serial.println("  Possible causes:");
            Serial.println("    - Wrong SIOD/SIOC pins");
            Serial.println("    - Camera not powered");
        } else if (err == 0x103) {
            g_camera_diag.last_error_msg = "ESP_ERR_INVALID_ARG - Pin configuration issue";
            Serial.println("  Error: ESP_ERR_INVALID_ARG");
            Serial.println("  Pin configuration issue");
        } else if (err == 0x101) {
            g_camera_diag.last_error_msg = "ESP_ERR_NO_MEM - Out of memory. PSRAM: " + String(psramFound() ? "Found" : "NOT FOUND");
            Serial.println("  Error: ESP_ERR_NO_MEM - Out of memory");
            Serial.printf("  Free heap: %d, PSRAM: %s\n", ESP.getFreeHeap(), psramFound() ? "Found" : "NOT FOUND");
        } else {
            g_camera_diag.last_error_msg = "Unknown error 0x" + String(err, HEX);
            Serial.printf("  Unknown error code: 0x%x\n", err);
        }
        
        return false;
    }
    
    Serial.println("✓ Camera driver initialized");
    
    // Apply configuration settings
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        Serial.println("❌ Failed to get camera sensor");
        g_camera_diag.init_failures++;
        g_camera_diag.last_init_success = false;
        g_camera_diag.last_error_code = ESP_FAIL;
        g_camera_diag.last_error_msg = "Failed to get camera sensor after init";
        g_camera_diag.sensor_detected = false;
        esp_camera_deinit();
        return false;
    }
    
    Serial.println("✓ Camera sensor acquired");
    
    // Record sensor info
    g_camera_diag.sensor_detected = true;
    g_camera_diag.sensor_id = "PID:0x" + String(s->id.PID, HEX) + " VER:0x" + String(s->id.VER, HEX) + " MIDL:0x" + String(s->id.MIDL, HEX);
    Serial.printf("  Sensor ID: %s\n", g_camera_diag.sensor_id.c_str());
    
    camera_initialized = true;
    camera_sleeping = false;
    camera_init_time = millis();
    
    // Flush first few frames - they are often corrupted/bad quality
    Serial.println("Flushing initial frames...");
    int flushed = 0;
    for (int i = 0; i < 5; i++) {
        delay(150); // Give camera time to stabilize
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            Serial.printf("  ✓ Flushed frame %d: %u bytes (%dx%d)\n", i+1, fb->len, fb->width, fb->height);
            esp_camera_fb_return(fb);
            flushed++;
        } else {
            Serial.printf("  ⚠ Frame %d: capture failed\n", i+1);
        }
    }
    Serial.printf("Camera warmup complete (%d/%d frames)\n", flushed, 5);
    
    // Record flush stats
    g_camera_diag.frames_flushed = flushed;
    
    // Now apply custom configuration settings
    if (s) {
        Serial.println("Applying custom camera settings...");
        s->set_framesize(s, (framesize_t)g_config.camera.framesize);
        s->set_quality(s, g_config.camera.quality);
        s->set_brightness(s, g_config.camera.brightness);
        s->set_contrast(s, g_config.camera.contrast);
        s->set_saturation(s, g_config.camera.saturation);
        s->set_gainceiling(s, (gainceiling_t)g_config.camera.gainceiling);
        s->set_colorbar(s, g_config.camera.colorbar);
        s->set_whitebal(s, g_config.camera.awb);
        s->set_gain_ctrl(s, g_config.camera.agc);
        s->set_exposure_ctrl(s, g_config.camera.aec);
        s->set_hmirror(s, g_config.camera.hmirror);
        s->set_vflip(s, g_config.camera.vflip);
        s->set_awb_gain(s, g_config.camera.awb_gain);
        s->set_agc_gain(s, g_config.camera.agc_gain);
        s->set_aec_value(s, g_config.camera.aec_value);
        s->set_special_effect(s, g_config.camera.special_effect);
        s->set_wb_mode(s, g_config.camera.wb_mode);
        s->set_ae_level(s, g_config.camera.ae_level);
        s->set_dcw(s, g_config.camera.dcw);
        s->set_bpc(s, g_config.camera.bpc);
        s->set_wpc(s, g_config.camera.wpc);
        s->set_raw_gma(s, g_config.camera.raw_gma);
        s->set_lenc(s, g_config.camera.lenc);
        Serial.println("✓ Settings applied");
    }
    
    // Mark successful initialization
    g_camera_diag.last_init_success = true;
    g_camera_diag.last_init_success_time = millis();
    g_camera_diag.last_error_code = ESP_OK;
    g_camera_diag.last_error_msg = "";
    
    // Apply streaming optimizations
    optimizeCameraForStreaming();
    
    return true;
}

void optimizeCameraForStreaming() {
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        Serial.println("❌ Failed to get camera sensor for optimization");
        return;
    }
    
    Serial.println("\n========================================");
    Serial.println("Optimizing Camera for Streaming");
    Serial.println("========================================");
    
    // Disable special effects for faster processing
    s->set_special_effect(s, 0);  // No special effect
    Serial.println("✓ Special effects disabled");
    
    // Enable automatic controls for better streaming quality
    s->set_whitebal(s, 1);        // Enable auto white balance
    s->set_awb_gain(s, 1);        // Enable AWB gain
    s->set_gain_ctrl(s, 1);       // Enable AGC
    s->set_exposure_ctrl(s, 1);   // Enable AEC
    Serial.println("✓ Automatic controls enabled (AWB, AGC, AEC)");
    
    // Optimize AEC/AGC for low latency
    // AEC sensor (auto exposure) - use faster convergence
    s->set_aec2(s, 0);            // Disable AEC DSP for lower latency
    s->set_ae_level(s, 0);        // Normal AE level
    Serial.println("✓ AEC optimized for low latency");
    
    // AGC (auto gain) - moderate settings
    s->set_agc_gain(s, 0);        // Auto-determined gain
    s->set_gainceiling(s, (gainceiling_t)2);  // Moderate gain ceiling for less noise
    Serial.println("✓ AGC configured for balanced performance");
    
    // Enable lens correction for better image quality
    s->set_lenc(s, 1);            // Enable lens correction
    s->set_bpc(s, 1);             // Enable black pixel correction
    s->set_wpc(s, 1);             // Enable white pixel correction
    s->set_raw_gma(s, 1);         // Enable gamma correction
    Serial.println("✓ Image corrections enabled");
    
    // Disable downsize for full frame processing
    s->set_dcw(s, 0);             // Disable downsize
    Serial.println("✓ Downsize disabled for full resolution");
    
    Serial.println("========================================");
    Serial.println("Camera optimized for streaming performance");
    Serial.println("========================================\n");
}

int adjustQualityBasedOnWiFi() {
    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping quality adjustment");
        return g_config.camera.quality;
    }
    
    int rssi = WiFi.RSSI();
    int quality = g_config.camera.quality;  // Start with current quality
    
    // Adjust JPEG quality based on WiFi signal strength
    // Lower quality (higher number) = smaller files = faster transmission on weak WiFi
    if (rssi > -50) {
        // Excellent signal: use best quality
        quality = 10;
    } else if (rssi > -60) {
        // Very good signal: high quality
        quality = 12;
    } else if (rssi > -70) {
        // Good signal: medium-high quality
        quality = 15;
    } else if (rssi > -80) {
        // Fair signal: medium quality
        quality = 18;
    } else {
        // Weak signal: lower quality for smaller file size
        quality = 22;
    }
    
    // Apply quality if changed
    sensor_t *s = esp_camera_sensor_get();
    if (s && quality != g_config.camera.quality) {
        s->set_quality(s, quality);
        g_config.camera.quality = quality;
        Serial.printf("Quality adjusted to %d (RSSI: %d dBm)\n", quality, rssi);
    }
    
    return quality;
}

bool adjustResolutionBasedOnPerformance(float current_fps) {
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        return false;
    }
    
    // Validate FPS value
    if (current_fps <= 0.0 || isnan(current_fps) || isinf(current_fps)) {
        Serial.printf("Invalid FPS value: %.2f, skipping resolution adjustment\n", current_fps);
        return false;
    }
    
    framesize_t current_size = (framesize_t)g_config.camera.framesize;
    framesize_t new_size = current_size;
    bool changed = false;
    
    // If FPS is too low, drop resolution
    if (current_fps < 5.0 && current_size > FRAMESIZE_QVGA) {
        // Drop down one resolution level
        if (current_size == FRAMESIZE_SVGA) {
            new_size = FRAMESIZE_VGA;  // 800x600 -> 640x480
        } else if (current_size == FRAMESIZE_VGA) {
            new_size = FRAMESIZE_CIF;  // 640x480 -> 400x296
        } else if (current_size == FRAMESIZE_CIF) {
            new_size = FRAMESIZE_QVGA;  // 400x296 -> 320x240
        }
        changed = true;
    } 
    // If FPS is very high, we can try to increase resolution
    else if (current_fps > 20.0 && current_size < FRAMESIZE_SVGA && psramFound()) {
        if (current_size == FRAMESIZE_QVGA) {
            new_size = FRAMESIZE_CIF;  // 320x240 -> 400x296
        } else if (current_size == FRAMESIZE_CIF) {
            new_size = FRAMESIZE_VGA;  // 400x296 -> 640x480
        } else if (current_size == FRAMESIZE_VGA) {
            new_size = FRAMESIZE_SVGA;  // 640x480 -> 800x600
        }
        changed = true;
    }
    
    if (changed && new_size != current_size) {
        int result = s->set_framesize(s, new_size);
        if (result == ESP_OK) {
            g_config.camera.framesize = new_size;
            Serial.printf("Resolution adjusted: %d -> %d (FPS: %.1f)\n", 
                         current_size, new_size, current_fps);
            return true;
        } else {
            Serial.printf("Failed to set framesize %d, error: %d\n", new_size, result);
            return false;
        }
    }
    
    return false;
}

void deinitCamera() {
    if (camera_initialized) {
        esp_camera_deinit();
        camera_initialized = false;
        camera_sleeping = true;
        Serial.println("Camera deinitialized");
    }
}

bool reinitCamera() {
    deinitCamera();
    delay(100);
    return initCamera();
}

camera_fb_t* captureFrame() {
    if (!camera_initialized || camera_sleeping) {
        Serial.println("❌ captureFrame: camera not ready (initialized=" + String(camera_initialized) + 
                       ", sleeping=" + String(camera_sleeping) + ")");
        return nullptr;
    }
    
    // Direct capture without mutex (stream also uses direct access)
    Serial.println("Attempting frame capture...");
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ captureFrame: esp_camera_fb_get returned NULL");
        // Try to get sensor status
        sensor_t *s = esp_camera_sensor_get();
        if (s) {
            Serial.printf("   Sensor status - ID: 0x%x\n", s->id.PID);
        } else {
            Serial.println("   Sensor not available!");
        }
    } else {
        Serial.printf("✓ Frame captured: %u bytes, %dx%d\n", fb->len, fb->width, fb->height);
    }
    return fb;
}

void releaseFrame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

void initLED() {
#ifdef LED_GPIO_NUM
    if (LED_GPIO_NUM >= 0) {
        ledcSetup(LED_LEDC_CHANNEL, 5000, 8);
        ledcAttachPin(LED_GPIO_NUM, LED_LEDC_CHANNEL);
        ledcWrite(LED_LEDC_CHANNEL, 0);
    }
#endif
}

void setLED(uint8_t intensity) {
#ifdef LED_GPIO_NUM
    if (LED_GPIO_NUM >= 0) {
        ledcWrite(LED_LEDC_CHANNEL, intensity);
    }
#endif
}

void printMemoryInfo() {
    Serial.println("\n--- Memory Info ---");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min free heap: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Heap size: %d bytes\n", ESP.getHeapSize());
    
    if (psramFound()) {
        Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
        Serial.printf("Min free PSRAM: %d bytes\n", ESP.getMinFreePsram());
    } else {
        Serial.println("PSRAM: Not available");
    }
    Serial.println("-------------------\n");
}

size_t getFreeHeap() {
    return ESP.getFreeHeap();
}

size_t getMinFreeHeap() {
    return ESP.getMinFreeHeap();
}

unsigned long getUptimeSeconds() {
    return millis() / 1000;
}

const char* getResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_POWERON: return "Power-on";
        case ESP_RST_SW: return "Software reset";
        case ESP_RST_PANIC: return "Exception/panic";
        case ESP_RST_INT_WDT: return "Interrupt watchdog";
        case ESP_RST_TASK_WDT: return "Task watchdog";
        case ESP_RST_WDT: return "Other watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep sleep";
        case ESP_RST_BROWNOUT: return "Brownout";
        case ESP_RST_SDIO: return "SDIO";
        default: return "Unknown";
    }
}

// Camera task - REMOVED to save resources
void cameraTask(void* parameter) {
    // Not used - task creation skipped
    vTaskDelete(NULL);
}

// Web server task - REMOVED (AsyncWebServer is truly async)
void webServerTask(void* parameter) {
    // Not used - task creation skipped
    vTaskDelete(NULL);
}

// Watchdog task - lightweight monitoring
void watchdogTask(void* parameter) {
    Serial.println("Watchdog task started on core " + String(xPortGetCoreID()));
    
    while (true) {
        // Minimal monitoring - check every 30 seconds
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 10000) {
            Serial.printf("⚠ Low heap: %d bytes\n", freeHeap);
        }
        
        vTaskDelay(pdMS_TO_TICKS(30000));  // 30 seconds
    }
}

// SD card task - handles storage operations
void sdCardTask(void* parameter) {
    Serial.println("SD card task started on core " + String(xPortGetCoreID()));
    
    while (true) {
        // Handle SD card operations if needed
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
