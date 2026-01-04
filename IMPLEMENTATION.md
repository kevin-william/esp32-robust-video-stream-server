# ESP32-CAM Robust Video Stream Server - Implementation Details

## Overview

This document describes the implementation of a high-performance, energy-efficient ESP32-CAM video streaming server designed for the AI-Thinker ESP32-CAM module with OV2640 camera sensor.

## Architecture

### Dual-Core FreeRTOS Design

The system leverages ESP32's dual-core architecture with strict task pinning:

#### Core 0 (PRO_CPU) - Network Operations
- **Web Server Task**: Handles all HTTP requests via AsyncWebServer
- **Watchdog Task**: Monitors system health and memory usage
- **WiFi Stack**: ESP32's WiFi driver automatically runs on this core
- **Priority**: Dedicated to network I/O and HTTP serving

#### Core 1 (APP_CPU) - Camera Operations
- **Camera Task**: Manages frame capture and sensor control
- **Image Processing**: Handles MJPEG encoding (via hardware)
- **Priority**: Dedicated to camera operations and image data

### Task Synchronization

- **Camera Mutex**: `cameraMutex` protects access to camera hardware between cores
- **Config Mutex**: `configMutex` protects configuration data structures
- **Event Queue**: `eventQueue` for inter-task communication (WiFi events, config updates, restart requests)

## Camera Driver Configuration

### I2S Parallel Mode with DMA

The camera driver uses I2S interface in parallel mode with DMA for high-speed data transfer:

```cpp
config.pixel_format = PIXFORMAT_JPEG;  // MJPEG format
config.fb_count = 2;                   // Double buffering
config.fb_location = CAMERA_FB_IN_PSRAM; // Use PSRAM for buffers
config.grab_mode = CAMERA_GRAB_LATEST;   // Always fresh frames
```

### Frame Buffer Management

- **Count**: 2 buffers for double buffering (eliminates bottlenecks)
- **Location**: PSRAM (requires ESP32 with PSRAM)
- **Mode**: `CAMERA_GRAB_LATEST` ensures fresh frames, discards old ones
- **Zero-Copy**: Frame data stays in PSRAM, copied only once to HTTP response

### MJPEG Streaming

- **Format**: JPEG compressed frames (hardware accelerated on ESP32)
- **Boundary**: Multipart HTTP response with `boundary=frame`
- **Frame Rate**: Controlled by delay in streaming callback (~15 FPS)
- **Optimization**: Minimal CPU overhead as JPEG encoding is done by camera hardware

## Power Management

### Camera Power Control

The OV2640 sensor supports hardware power-down via PWDN pin (GPIO 32):

```cpp
// Wake up camera
gpio_set_level((gpio_num_t)PWDN_GPIO_NUM, 0);  // Active LOW

// Power down camera
gpio_set_level((gpio_num_t)PWDN_GPIO_NUM, 1);  // Active HIGH
```

### WiFi Power Modes

- **Streaming Mode**: `WIFI_PS_NONE` - Full power for minimal latency
- **Idle Mode**: `WIFI_PS_MIN_MODEM` - Power save when camera stopped
- **Control**: Automatically managed by `/start` and `/stop` endpoints

### Power Consumption Scenarios

1. **Active Streaming**: WiFi full power, camera active, ~200-300mA
2. **Camera Stopped**: WiFi power save, camera in PWDN, ~80-120mA
3. **Deep Sleep**: Not implemented (would lose WiFi connection)

## HTTP Endpoints

### Standard Endpoints

- `GET /status` - System status and metrics
- `GET /capture` - Single JPEG image capture
- `GET /stream` - MJPEG video stream
- `GET /control` - Camera parameter adjustment

### Power Management Endpoints

#### GET /stop
Stops camera service and enters low-power mode:
1. Deinitializes camera driver (`esp_camera_deinit()`)
2. Sets PWDN pin HIGH (power down OV2640)
3. Enables WiFi power save (`WIFI_PS_MIN_MODEM`)
4. Frees all frame buffer memory
5. WiFi remains active for receiving commands

**Power Savings**: Reduces consumption by ~50-60%

#### GET /start
Starts or restarts camera service:
1. Disables WiFi power save (`WIFI_PS_NONE`)
2. Sets PWDN pin LOW (wake OV2640)
3. If camera running, forces complete reinitialization
4. If camera stopped, initializes normally
5. Allocates frame buffers in PSRAM

**Use Case**: Resume streaming after power-down

#### GET /reset
Performs hard reset with cleanup:
1. Sends HTTP response
2. Deinitializes camera to free resources
3. Waits for cleanup to complete
4. Calls `esp_restart()` for complete system reset

**Use Case**: Recover from errors, apply firmware updates

## Memory Management

### Heap Management

- **Total Heap**: ~320KB on ESP32
- **PSRAM**: 4MB (when available)
- **Frame Buffers**: Allocated in PSRAM (2 × ~60KB for VGA)
- **Configuration**: Limited to 2KB JSON
- **Monitoring**: Watchdog task checks free heap every 5 seconds

### Memory Leak Prevention

1. **Strict Cleanup**: All resources freed in `deinitCamera()`
2. **Immediate Release**: Frame buffers released immediately after use
3. **Mutex Protection**: Prevents race conditions on shared resources
4. **No Fragmentation**: Large allocations (frame buffers) use PSRAM

### Memory Regions

- **DRAM**: Stack, heap, global variables
- **IRAM**: Interrupt handlers, frequently called functions
- **PSRAM**: Frame buffers, large data structures
- **Flash**: Program code, constants

## Logging System

### ESP_LOG Macros

The system uses ESP-IDF logging throughout:

```cpp
ESP_LOGI(TAG, "Information message");   // Info
ESP_LOGW(TAG, "Warning message");       // Warning
ESP_LOGE(TAG, "Error message");         // Error
ESP_LOGD(TAG, "Debug message");         // Debug (verbose)
```

### Log Levels

- **ERROR**: Critical errors (camera init failure, etc.)
- **WARN**: Warnings (low memory, frame capture failure)
- **INFO**: Normal operation (initialization, endpoint access)
- **DEBUG**: Verbose details (frame size, memory stats)

### Log Tags

- `MAIN`: Main application flow
- `CAMERA`: Camera operations
- `WEB_SERVER`: HTTP endpoint handling

## Best Practices Implemented

### 1. Thread Safety
- Mutexes protect shared resources
- Task pinning eliminates race conditions
- Atomic operations for flags

### 2. Error Handling
- All API calls checked for errors
- Graceful degradation on failure
- Detailed error logging

### 3. Resource Management
- Resources acquired and released in pairs
- No memory leaks in start/stop cycles
- Proper cleanup before restart

### 4. Performance
- Zero-copy optimization where possible
- Hardware JPEG encoding
- Double buffering eliminates bottlenecks
- Dedicated cores prevent contention

### 5. Energy Efficiency
- Hardware power-down when idle
- WiFi power save modes
- Minimal CPU overhead in streaming

## Configuration Parameters

### Camera Settings (defaults)

```cpp
framesize: FRAMESIZE_VGA (640×480)
quality: 10 (JPEG quality, 0-63)
fb_count: 2 (double buffering)
fb_location: CAMERA_FB_IN_PSRAM
grab_mode: CAMERA_GRAB_LATEST
xclk_freq: 20MHz
```

### Task Priorities

```cpp
CAMERA_TASK_PRIORITY: 2
WEB_TASK_PRIORITY: 2
WATCHDOG_TASK_PRIORITY: 3
```

### Core Assignment

```cpp
CAMERA_CORE: 1 (APP_CPU)
WEB_CORE: 0 (PRO_CPU)
WATCHDOG_CORE: 0 (PRO_CPU)
```

## Testing Recommendations

### Functional Testing

1. **Camera Initialization**
   - Verify PSRAM detection
   - Check frame buffer allocation
   - Test sensor configuration

2. **Streaming**
   - Test `/stream` endpoint
   - Verify frame rate (~15 FPS)
   - Check memory stability

3. **Power Management**
   - Test `/stop` and `/start` cycle
   - Measure power consumption
   - Verify WiFi power save

4. **Reset**
   - Test `/reset` endpoint
   - Verify clean reboot
   - Check memory after restart

### Performance Testing

1. **Frame Rate**: Monitor FPS under different resolutions
2. **Memory**: Check heap usage over extended streaming
3. **WiFi**: Test signal strength impact on streaming
4. **CPU**: Monitor core utilization

### Stress Testing

1. **Continuous Streaming**: Run for 24+ hours
2. **Start/Stop Cycles**: Test 100+ cycles
3. **Multiple Clients**: Test concurrent stream viewers
4. **WiFi Reconnection**: Test network interruption recovery

## Troubleshooting

### Common Issues

1. **Camera Init Failed**
   - Check PSRAM enabled in platformio.ini
   - Verify camera module connections
   - Ensure adequate power supply (2A minimum)

2. **Low FPS**
   - Reduce frame size
   - Increase JPEG quality number (lower quality)
   - Check WiFi signal strength

3. **Memory Errors**
   - Verify PSRAM detection
   - Check for memory leaks in logs
   - Reduce frame buffer count if needed

4. **WiFi Disconnections**
   - Check power supply stability
   - Verify router compatibility
   - Test WiFi power save modes

## Future Enhancements

1. **Motion Detection**: Frame differencing for event triggers
2. **Image Storage**: Save frames to SD card
3. **Time-Lapse**: Scheduled capture and storage
4. **Face Detection**: AI-powered person detection
5. **HTTPS**: Secure streaming over TLS

## References

- [ESP32-Camera Library](https://github.com/espressif/esp32-camera)
- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [OV2640 Datasheet](https://www.uctronics.com/download/cam_module/OV2640DS.pdf)
