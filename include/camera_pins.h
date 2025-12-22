#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

// Camera models are defined via build flags in platformio.ini
// This file defines the pin mappings for each camera model

// AI Thinker ESP32-CAM Pin Map
#if defined(CAMERA_MODEL_AI_THINKER)
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

// ESP32-WROVER-KIT Pin Map (Official Espressif)
#elif defined(CAMERA_MODEL_WROVER_KIT)
    // Official WROVER-KIT pinout (Espressif reference design)
    #define PWDN_GPIO_NUM     -1
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM     21
    #define SIOD_GPIO_NUM     26
    #define SIOC_GPIO_NUM     27
    
    #define Y9_GPIO_NUM       35
    #define Y8_GPIO_NUM       34
    #define Y7_GPIO_NUM       39
    #define Y6_GPIO_NUM       36
    #define Y5_GPIO_NUM       19
    #define Y4_GPIO_NUM       18
    #define Y3_GPIO_NUM        5
    #define Y2_GPIO_NUM        4
    #define VSYNC_GPIO_NUM    25
    #define HREF_GPIO_NUM     23
    #define PCLK_GPIO_NUM     22
    
    // WROVER-KIT doesn't have a dedicated flash LED
    #define LED_GPIO_NUM      -1
    #define LED_LEDC_CHANNEL   2
    
    // SD Card pins (if available on your WROVER version)
    #define SD_CS_PIN         13
    #define SD_MOSI_PIN       15
    #define SD_MISO_PIN        2
    #define SD_SCK_PIN        14

#else
    #error "Camera model not defined! Please define CAMERA_MODEL_AI_THINKER or CAMERA_MODEL_WROVER_KIT in platformio.ini"
#endif

#endif // CAMERA_PINS_H
