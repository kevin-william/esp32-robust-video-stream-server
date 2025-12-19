#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

// Camera models
#define CAMERA_MODEL_AI_THINKER 1
#define CAMERA_MODEL_WROVER_KIT 2
#define CAMERA_MODEL_ESP_EYE 3
#define CAMERA_MODEL_M5STACK_PSRAM 4
#define CAMERA_MODEL_M5STACK_V2_PSRAM 5
#define CAMERA_MODEL_M5STACK_WIDE 6

// Select Camera Model
#if !defined(CAMERA_MODEL_AI_THINKER) && !defined(CAMERA_MODEL_WROVER_KIT) && \
    !defined(CAMERA_MODEL_ESP_EYE) && !defined(CAMERA_MODEL_M5STACK_PSRAM)
    #define CAMERA_MODEL_AI_THINKER
#endif

// AI Thinker ESP32-CAM Pin Map
#ifdef CAMERA_MODEL_AI_THINKER
    #define PWDN_GPIO_NUM     32
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM      0
    #define SIOD_GPIO_NUM     26
    #define SIOC_GPIO_NUM     27
    
    #define Y9_GPIO_NUM       35
    #define Y8_GPIO_NUM       34
    #define Y7_GPIO_NUM       39
    #define Y6_GPIO_NUM       36
    #define Y5_GPIO_NUM       21
    #define Y4_GPIO_NUM       19
    #define Y3_GPIO_NUM       18
    #define Y2_GPIO_NUM        5
    #define VSYNC_GPIO_NUM    25
    #define HREF_GPIO_NUM     23
    #define PCLK_GPIO_NUM     22
    
    // Flash LED on GPIO 4
    #define LED_GPIO_NUM       4
    #define LED_LEDC_CHANNEL   2
    
    // SD Card pins (using default SPI pins)
    #define SD_CS_PIN         13
    #define SD_MOSI_PIN       15
    #define SD_MISO_PIN        2
    #define SD_SCK_PIN        14
#endif

// WROVER-KIT Pin Map
#ifdef CAMERA_MODEL_WROVER_KIT
    #define PWDN_GPIO_NUM    -1
    #define RESET_GPIO_NUM   -1
    #define XCLK_GPIO_NUM    21
    #define SIOD_GPIO_NUM    26
    #define SIOC_GPIO_NUM    27
    
    #define Y9_GPIO_NUM      35
    #define Y8_GPIO_NUM      34
    #define Y7_GPIO_NUM      39
    #define Y6_GPIO_NUM      36
    #define Y5_GPIO_NUM      19
    #define Y4_GPIO_NUM      18
    #define Y3_GPIO_NUM       5
    #define Y2_GPIO_NUM       4
    #define VSYNC_GPIO_NUM   25
    #define HREF_GPIO_NUM    23
    #define PCLK_GPIO_NUM    22
    
    #define LED_GPIO_NUM     -1
#endif

// ESP-EYE Pin Map
#ifdef CAMERA_MODEL_ESP_EYE
    #define PWDN_GPIO_NUM    -1
    #define RESET_GPIO_NUM   -1
    #define XCLK_GPIO_NUM     4
    #define SIOD_GPIO_NUM    18
    #define SIOC_GPIO_NUM    23
    
    #define Y9_GPIO_NUM      36
    #define Y8_GPIO_NUM      37
    #define Y7_GPIO_NUM      38
    #define Y6_GPIO_NUM      39
    #define Y5_GPIO_NUM      35
    #define Y4_GPIO_NUM      14
    #define Y3_GPIO_NUM      13
    #define Y2_GPIO_NUM      34
    #define VSYNC_GPIO_NUM    5
    #define HREF_GPIO_NUM    27
    #define PCLK_GPIO_NUM    25
    
    #define LED_GPIO_NUM     22
#endif

#endif // CAMERA_PINS_H
