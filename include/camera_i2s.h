#ifndef CAMERA_I2S_H
#define CAMERA_I2S_H

#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>

// DMA buffer configuration constants
#define DMA_BUFFER_COUNT 4    // Number of DMA buffers
#define DMA_BUFFER_SIZE 1024  // Size of each DMA buffer in bytes

// Frame buffer structure (compatible with existing code interface)
typedef struct {
    uint8_t *buf;        // Pointer to image data
    size_t len;          // Length of image data
    size_t width;        // Image width in pixels
    size_t height;       // Image height in pixels
    uint32_t timestamp;  // Timestamp when frame was captured
    void *priv;          // Private data for buffer management
} camera_fb_t;

// Frame size definitions
typedef enum {
    FRAMESIZE_QVGA,  // 320x240
    FRAMESIZE_CIF,   // 352x288
    FRAMESIZE_HVGA,  // 480x320
    FRAMESIZE_VGA,   // 640x480
    FRAMESIZE_SVGA,  // 800x600
    FRAMESIZE_UXGA,  // 1600x1200
    FRAMESIZE_INVALID
} framesize_t;

// JPEG quality (0-63, lower is better quality)
#define JPEG_QUALITY_HIGH 10
#define JPEG_QUALITY_MEDIUM 20
#define JPEG_QUALITY_LOW 30

// Camera I2S DMA configuration
typedef struct {
    // Pin configuration
    int pin_d0;
    int pin_d1;
    int pin_d2;
    int pin_d3;
    int pin_d4;
    int pin_d5;
    int pin_d6;
    int pin_d7;
    int pin_xclk;
    int pin_pclk;
    int pin_vsync;
    int pin_href;
    int pin_sccb_sda;
    int pin_sccb_scl;
    int pin_pwdn;
    int pin_reset;

    // Timing configuration
    uint32_t xclk_freq_hz;

    // Frame configuration
    framesize_t frame_size;
    uint8_t jpeg_quality;

    // Buffer configuration
    size_t fb_count;  // Number of frame buffers (2 for double buffering)
    bool use_psram;   // Use PSRAM for frame buffers

    // I2S DMA configuration
    size_t dma_buffer_count;  // Number of DMA buffers
    size_t dma_buffer_size;   // Size of each DMA buffer
} camera_config_t;

// Camera sensor settings
typedef struct {
    int brightness;  // -2 to 2
    int contrast;    // -2 to 2
    int saturation;  // -2 to 2
    int sharpness;   // -2 to 2
    int denoise;     // 0 to 8
    bool hmirror;    // Horizontal mirror
    bool vflip;      // Vertical flip
    bool awb;        // Auto white balance
    bool agc;        // Auto gain control
    bool aec;        // Auto exposure control
} camera_sensor_settings_t;

// Function prototypes for custom I2S+DMA camera driver
esp_err_t camera_i2s_init(const camera_config_t *config);
esp_err_t camera_i2s_deinit(void);
camera_fb_t *camera_i2s_fb_get(void);
void camera_i2s_fb_return(camera_fb_t *fb);
esp_err_t camera_i2s_sensor_set(camera_sensor_settings_t *settings);

// OV2640 specific functions
esp_err_t ov2640_init(void);
esp_err_t ov2640_set_framesize(framesize_t framesize);
esp_err_t ov2640_set_quality(uint8_t quality);
esp_err_t ov2640_set_brightness(int level);
esp_err_t ov2640_set_contrast(int level);
esp_err_t ov2640_set_saturation(int level);
esp_err_t ov2640_set_hmirror(bool enable);
esp_err_t ov2640_set_vflip(bool enable);

#endif  // CAMERA_I2S_H
