/*
 * ============================================================================
 * ESP32-CAM Camera Interface
 * ============================================================================
 * 
 * Este módulo fornece a interface de alto nível para controle da câmera
 * OV2640 usando driver customizado I2S+DMA.
 * 
 * Características:
 * - Driver I2S+DMA de baixo nível para máxima performance
 * - Comunicação direta com OV2640 via SCCB/I2C
 * - 2 frame buffers em PSRAM para double buffering
 * - Formato MJPEG para compressão eficiente
 * 
 * Autor: ESP32-CAM Project
 * Data: 2026
 * ============================================================================
 */

#include "app.h"
#include "config.h"
#include "camera_pins.h"
#include "camera_i2s.h"
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <driver/gpio.h>

static const char* TAG = "CAMERA";

/**
 * Inicializa a câmera com driver I2S+DMA customizado
 * 
 * Este função configura:
 * 1. Pinos da câmera (8 bits de dados paralelos)
 * 2. Clock XCLK usando LEDC
 * 3. Comunicação SCCB/I2C com sensor OV2640
 * 4. Driver I2S em modo câmera com DMA
 * 5. Buffers de frame em PSRAM (se disponível)
 * 
 * @return true se inicialização bem sucedida, false caso contrário
 */
bool initCamera() {
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Inicializando câmera com driver I2S+DMA...");
    ESP_LOGI(TAG, "============================================");
    
    // Preparar estrutura de configuração
    camera_config_t config = {0};
    
    // ========================================================================
    // CONFIGURAÇÃO DE PINOS - ESP32-CAM AI-Thinker
    // ========================================================================
    // Pinos de dados paralelos (D0-D7) - 8 bits
    config.pin_d0 = Y2_GPIO_NUM;    // GPIO 5  - D0
    config.pin_d1 = Y3_GPIO_NUM;    // GPIO 18 - D1
    config.pin_d2 = Y4_GPIO_NUM;    // GPIO 19 - D2
    config.pin_d3 = Y5_GPIO_NUM;    // GPIO 21 - D3
    config.pin_d4 = Y6_GPIO_NUM;    // GPIO 36 - D4
    config.pin_d5 = Y7_GPIO_NUM;    // GPIO 39 - D5
    config.pin_d6 = Y8_GPIO_NUM;    // GPIO 34 - D6
    config.pin_d7 = Y9_GPIO_NUM;    // GPIO 35 - D7
    
    // Pinos de sincronização
    config.pin_xclk = XCLK_GPIO_NUM;    // GPIO 0  - Master clock
    config.pin_pclk = PCLK_GPIO_NUM;    // GPIO 22 - Pixel clock
    config.pin_vsync = VSYNC_GPIO_NUM;  // GPIO 25 - Vertical sync
    config.pin_href = HREF_GPIO_NUM;    // GPIO 23 - Horizontal reference
    
    // Pinos de comunicação I2C/SCCB com sensor
    config.pin_sccb_sda = SIOD_GPIO_NUM;  // GPIO 26 - I2C SDA
    config.pin_sccb_scl = SIOC_GPIO_NUM;  // GPIO 27 - I2C SCL
    
    // Pinos de controle
    config.pin_pwdn = PWDN_GPIO_NUM;    // GPIO 32 - Power down
    config.pin_reset = RESET_GPIO_NUM;  // -1 (não usado)
    
    // ========================================================================
    // CONFIGURAÇÃO DE TIMING
    // ========================================================================
    config.xclk_freq_hz = 20000000;  // 20MHz - Clock master para OV2640
    
    // ========================================================================
    // CONFIGURAÇÃO DE FRAME
    // ========================================================================
    config.frame_size = FRAMESIZE_VGA;        // 640x480 - ótimo balanço qualidade/performance
    config.jpeg_quality = JPEG_QUALITY_HIGH;  // Qualidade 10 (0-63, menor = melhor)
    
    // ========================================================================
    // CONFIGURAÇÃO DE BUFFERS
    // ========================================================================
    // Double buffering: enquanto um buffer está sendo lido via HTTP,
    // o outro está sendo preenchido pelo DMA - elimina gargalos
    config.fb_count = 2;
    config.use_psram = psramFound();  // Detecta PSRAM automaticamente
    
    // ========================================================================
    // CONFIGURAÇÃO DE DMA
    // ========================================================================
    // DMA transfere dados da câmera para memória sem CPU
    // Múltiplos buffers pequenos são mais eficientes que um buffer grande
    config.dma_buffer_count = DMA_BUFFER_COUNT;  // 4 buffers
    config.dma_buffer_size = DMA_BUFFER_SIZE;    // 1024 bytes cada
    
    // ========================================================================
    // LOG DE CONFIGURAÇÃO
    // ========================================================================
    if (config.use_psram) {
        ESP_LOGI(TAG, "✓ PSRAM detectado - modo alta performance");
        ESP_LOGI(TAG, "  → Frame buffers: 2 (alocados em PSRAM)");
        ESP_LOGI(TAG, "  → Modo: I2S paralelo 8-bit + DMA");
        ESP_LOGI(TAG, "  → Formato: MJPEG (JPEG comprimido)");
        ESP_LOGI(TAG, "  → DMA: %d buffers × %d bytes", 
                 config.dma_buffer_count, config.dma_buffer_size);
    } else {
        ESP_LOGI(TAG, "⚠ PSRAM não encontrado - usando DRAM");
        ESP_LOGI(TAG, "  → Frame buffers: 1 (limitado por DRAM)");
        config.fb_count = 1;
    }
    
    // ========================================================================
    // INICIALIZAÇÃO DO DRIVER
    // ========================================================================
    ESP_LOGI(TAG, "Inicializando driver I2S+DMA...");
    esp_err_t err = camera_i2s_init(&config);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "✗ Falha na inicialização! Erro: 0x%x", err);
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }
    
    ESP_LOGI(TAG, "✓ Driver inicializado com sucesso");
    
    // ========================================================================
    // APLICAR CONFIGURAÇÕES DO SENSOR
    // ========================================================================
    ESP_LOGI(TAG, "Aplicando configurações do sensor OV2640...");
    
    camera_sensor_settings_t settings = {0};
    settings.brightness = g_config.camera.brightness;  // -2 a +2
    settings.contrast = g_config.camera.contrast;      // -2 a +2
    settings.saturation = g_config.camera.saturation;  // -2 a +2
    settings.hmirror = g_config.camera.hmirror;        // Espelhamento horizontal
    settings.vflip = g_config.camera.vflip;            // Inversão vertical
    settings.awb = g_config.camera.awb;                // Auto white balance
    settings.agc = g_config.camera.agc;                // Auto gain control
    settings.aec = g_config.camera.aec;                // Auto exposure control
    
    camera_i2s_sensor_set(&settings);
    ESP_LOGI(TAG, "✓ Configurações do sensor aplicadas");
    
    // ========================================================================
    // ATUALIZAR ESTADO GLOBAL
    // ========================================================================
    camera_initialized = true;
    camera_sleeping = false;
    camera_init_time = millis();
    
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "✓ Câmera pronta para captura!");
    ESP_LOGI(TAG, "  Tempo decorrido: %lu ms", millis() - camera_init_time);
    ESP_LOGI(TAG, "============================================");
    
    return true;
}

/**
 * Desinicializa a câmera e libera recursos
 * 
 * Esta função:
 * 1. Para o driver I2S+DMA
 * 2. Libera os frame buffers
 * 3. Coloca o sensor OV2640 em modo power-down
 * 4. Libera mutexes e recursos do FreeRTOS
 */
void deinitCamera() {
    if (camera_initialized) {
        ESP_LOGI(TAG, "Desligando câmera...");
        
        // Desinicializa driver customizado I2S+DMA
        camera_i2s_deinit();
        
        // Atualizar estado
        camera_initialized = false;
        camera_sleeping = true;
        
        ESP_LOGI(TAG, "✓ Câmera desligada e em modo sleep");
        Serial.println("Camera deinitialized");
    }
}

/**
 * Reinicializa a câmera (força deinit + init)
 * 
 * Útil para:
 * - Recuperar de erros
 * - Aplicar novas configurações que requerem reset
 * - Limpar estado da câmera
 * 
 * @return true se reinicialização bem sucedida
 */
bool reinitCamera() {
    ESP_LOGI(TAG, "Reinicializando câmera (força deinit + init)...");
    
    // Primeiro desinicializa completamente
    deinitCamera();
    
    // Aguarda limpeza de recursos
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Reinicializa do zero
    return initCamera();
}

/**
 * Captura um frame da câmera
 * 
 * Esta função:
 * 1. Verifica se câmera está inicializada
 * 2. Adquire mutex para thread-safety
 * 3. Obtém frame do driver I2S+DMA
 * 4. Libera mutex
 * 
 * @return Ponteiro para frame buffer ou nullptr se falha
 * 
 * IMPORTANTE: Após usar o frame, DEVE chamar releaseFrame()
 *             para devolver o buffer ao pool!
 */
camera_fb_t* captureFrame() {
    // Verificar se câmera está pronta
    if (!camera_initialized || camera_sleeping) {
        ESP_LOGW(TAG, "⚠ Não pode capturar - câmera não inicializada ou dormindo");
        return nullptr;
    }
    
    // Adquirir mutex para garantir acesso exclusivo (thread-safe)
    // Aguarda indefinidamente se necessário (portMAX_DELAY)
    if (xSemaphoreTake(cameraMutex, portMAX_DELAY) == pdTRUE) {
        
        // Obter frame do driver I2S+DMA
        camera_fb_t *fb = camera_i2s_fb_get();
        
        if (!fb) {
            ESP_LOGE(TAG, "✗ Falha ao capturar frame");
        }
        // Nota: Detalhes do frame (tamanho, dimensões) são logados apenas
        //       em builds de debug para evitar impacto na performance
        
        // Liberar mutex
        xSemaphoreGive(cameraMutex);
        
        return fb;
    }
    
    ESP_LOGW(TAG, "⚠ Falha ao adquirir mutex da câmera");
    return nullptr;
}

/**
 * Libera frame buffer de volta ao pool
 * 
 * CRÍTICO: Esta função DEVE ser chamada após usar um frame
 *          obtido com captureFrame(), caso contrário haverá
 *          vazamento de memória!
 * 
 * @param fb Ponteiro para frame buffer a ser liberado
 */
void releaseFrame(camera_fb_t* fb) {
    if (fb) {
        // Devolve buffer ao pool do driver I2S+DMA
        camera_i2s_fb_return(fb);
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
// Pinned to Core 1 (APP_CPU) for dedicated image processing
void cameraTask(void* parameter) {
    ESP_LOGI(TAG, "Camera task started on core %d (APP_CPU)", xPortGetCoreID());
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
// Pinned to Core 0 (PRO_CPU) along with WiFi stack
void webServerTask(void* parameter) {
    ESP_LOGI(TAG, "Web server task started on core %d (PRO_CPU)", xPortGetCoreID());
    Serial.println("Web server task started on core " + String(xPortGetCoreID()));
    
    while (true) {
        // Web server runs asynchronously via AsyncWebServer, just keep task alive
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Watchdog task - monitors system health
// Pinned to Core 0 (PRO_CPU)
void watchdogTask(void* parameter) {
    ESP_LOGI(TAG, "Watchdog task started on core %d (PRO_CPU)", xPortGetCoreID());
    Serial.println("Watchdog task started on core " + String(xPortGetCoreID()));
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(5000); // 5 seconds
    
    while (true) {
        // Monitor heap and system health
        size_t free_heap = ESP.getFreeHeap();
        if (free_heap < 10000) {
            ESP_LOGW(TAG, "Low heap memory: %d bytes", free_heap);
            Serial.println("WARNING: Low heap memory!");
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// SD card task - handles storage operations
void sdCardTask(void* parameter) {
    ESP_LOGI(TAG, "SD card task started on core %d", xPortGetCoreID());
    Serial.println("SD card task started on core " + String(xPortGetCoreID()));
    
    while (true) {
        // Handle SD card operations if needed
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
