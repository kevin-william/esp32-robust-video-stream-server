#include "app.h"
#include "config.h"
#include "camera_pins.h"
#include <esp_camera.h>
#include <esp_system.h>
#include <esp_heap_caps.h>

bool initCamera() {
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
    
    // Init with high specs to pre-allocate larger buffers
    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
        Serial.println("PSRAM found, using high quality settings");
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        Serial.println("PSRAM not found, using lower quality settings");
    }
    
    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }
    
    // Apply configuration settings
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
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
    }
    
    camera_initialized = true;
    camera_sleeping = false;
    camera_init_time = millis();
    
    return true;
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
        return nullptr;
    }
    
    if (xSemaphoreTake(cameraMutex, portMAX_DELAY) == pdTRUE) {
        camera_fb_t *fb = esp_camera_fb_get();
        xSemaphoreGive(cameraMutex);
        return fb;
    }
    
    return nullptr;
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

// Camera task - handles frame capture and processing
void cameraTask(void* parameter) {
    Serial.println("Camera task started on core " + String(xPortGetCoreID()));
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 10 Hz
    
    while (true) {
        // This task can be used for periodic camera maintenance
        // or frame preprocessing if needed
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Web server task - handles HTTP requests
void webServerTask(void* parameter) {
    Serial.println("Web server task started on core " + String(xPortGetCoreID()));
    
    while (true) {
        // Web server runs asynchronously, just keep task alive
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Watchdog task - monitors system health
void watchdogTask(void* parameter) {
    Serial.println("Watchdog task started on core " + String(xPortGetCoreID()));
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(5000); // 5 seconds
    
    while (true) {
        // Monitor heap and system health
        if (ESP.getFreeHeap() < 10000) {
            Serial.println("WARNING: Low heap memory!");
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
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
