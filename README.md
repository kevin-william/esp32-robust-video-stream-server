# ESP32-CAM Robust Video Stream Server

A complete, production-ready ESP32-CAM project featuring dual-core FreeRTOS architecture, I2S+DMA camera driver, configuration persistence, captive portal WiFi setup, REST API, MJPEG streaming with zero-copy optimization, and comprehensive power management.

## Features

- **Dual-Core FreeRTOS Architecture**: Core 0 dedicated to WiFi/HTTP, Core 1 dedicated to camera capture for maximum performance
- **High-Performance Camera Driver**: I2S parallel mode with DMA, 2 frame buffers in PSRAM for zero-copy optimization
- **MJPEG Streaming**: Real-time video streaming optimized for minimal CPU usage
- **Power Management**: Camera power-down mode (PWDN pin), WiFi power save modes, energy-efficient operation
- **Advanced Endpoints**: /stream, /capture, /reset (hard reset with cleanup), /stop (power-down), /start (reinit)
- **Captive Portal**: Automatic WiFi configuration portal when no saved networks are available
- **Configuration Persistence**: Save/load settings from SD card with NVS fallback
- **REST API**: Comprehensive JSON API with CORS support for camera control
- **Detailed Logging**: ESP_LOGI logging throughout for debugging and monitoring
- **LED Control**: Flash LED control with adjustable intensity
- **Memory Optimized**: Strict memory management to prevent leaks, designed for ESP32 constraints with PSRAM support

## Hardware Requirements

### Supported Boards
- ESP32-CAM (AI-Thinker) - **Primary target**
- ESP32-WROVER-KIT
- ESP-EYE
- Other ESP32 boards with camera support

### Recommended Specs
- ESP32 with PSRAM (for higher resolutions)
- MicroSD card (optional, for configuration persistence)
- Flash LED (built-in on ESP32-CAM)

### Pin Configuration (ESP32-CAM AI-Thinker)

**Camera Pins:**
- PWDN: GPIO 32
- RESET: -1 (not used)
- XCLK: GPIO 0
- SIOD (SDA): GPIO 26
- SIOC (SCL): GPIO 27
- Y9-Y2: GPIOs 35, 34, 39, 36, 21, 19, 18, 5
- VSYNC: GPIO 25
- HREF: GPIO 23
- PCLK: GPIO 22

**SD Card Pins:**
- CS: GPIO 13
- MOSI: GPIO 15
- MISO: GPIO 2
- SCK: GPIO 14

**LED:**
- Flash LED: GPIO 4

## Quick Start

### Installation

#### Option 1: PlatformIO (Recommended)

```bash
# Clone the repository
git clone https://github.com/kevin-william/esp32-robust-video-stream-server.git
cd esp32-robust-video-stream-server

# Install dependencies and build
pio run

# Upload to ESP32-CAM
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

#### Option 2: Arduino IDE

1. Install Arduino IDE
2. Add ESP32 board support:
   - Open File > Preferences
   - Add to "Additional Board Manager URLs": 
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools > Board > Board Manager
   - Search for "esp32" and install

3. Install required libraries:
   - ESP32-Camera
   - ArduinoJson (v6.21.0+)
   - AsyncTCP
   - ESPAsyncWebServer

4. Open `src/main.cpp` in Arduino IDE
5. Select Board: "AI Thinker ESP32-CAM"
6. Select Port and Upload

### First Boot - WiFi Configuration

1. **Power on the ESP32-CAM** (via USB-to-Serial adapter or 5V power)
2. **Connect to the AP**: Look for WiFi network named `ESP32-CAM-Setup` (password: `12345678`)
3. **Open browser** and navigate to `http://192.168.4.1`
4. **Scan for networks** and select your WiFi network
5. **Enter password** and click Connect
6. **Device will connect** to your WiFi network and automatically disable the captive portal

**Note**: The captive portal remains active indefinitely until a successful WiFi connection is established. The device will not restart automatically or timeout - it stays accessible for configuration until properly connected to a network. Camera and video streaming services are only activated after WiFi connection is successful.

### After WiFi Connection

Once connected to WiFi, the device will display its IP address in the serial monitor. Access the web interface at:

```
http://<ESP32-IP-ADDRESS>/
```

## API Endpoints

### Camera Control

#### GET /status
Get system status including camera state, memory, WiFi info, and uptime.

```bash
curl http://<ESP32-IP>/status
```

**Response:**
```json
{
  "camera_initialized": true,
  "camera_sleeping": false,
  "uptime": 3600,
  "free_heap": 120000,
  "min_free_heap": 100000,
  "free_psram": 4000000,
  "wifi_connected": true,
  "ip_address": "192.168.1.100",
  "rssi": -45,
  "ap_mode": false,
  "reset_reason": "Power-on",
  "known_networks": ["MyWiFi"]
}
```

#### GET /sleepstatus
Get camera sleep status and uptime.

```bash
curl http://<ESP32-IP>/sleepstatus
```

**Response:**
```json
{
  "sleeping": false,
  "uptime": 3600
}
```

#### GET /capture
Capture a single JPEG image.

```bash
curl http://<ESP32-IP>/capture -o photo.jpg
```

#### GET /stream
MJPEG video stream (multipart/x-mixed-replace).

```bash
# View in browser
http://<ESP32-IP>/stream

# Stream with curl
curl http://<ESP32-IP>/stream --output - | ffplay -
```

#### GET /control?var=<variable>&val=<value>
Adjust camera parameters.

**Parameters:**
- `framesize`: 0-13 (see framesize_t enum)
- `quality`: 0-63 (lower is better, 10-12 recommended)
- `brightness`: -2 to 2
- `contrast`: -2 to 2
- `saturation`: -2 to 2
- `hmirror`: 0 or 1
- `vflip`: 0 or 1
- `led_intensity`: 0-255

```bash
# Set quality to 10
curl "http://<ESP32-IP>/control?var=quality&val=10"

# Enable horizontal mirror
curl "http://<ESP32-IP>/control?var=hmirror&val=1"

# Set LED intensity
curl "http://<ESP32-IP>/control?var=led_intensity&val=128"
```

#### GET /sleep
Put camera to sleep (deinitialize, free memory).

```bash
curl http://<ESP32-IP>/sleep
```

**Response:**
```json
{
  "success": true,
  "message": "Camera sleeping"
}
```

#### GET /wake
Wake camera from sleep (reinitialize).

```bash
curl http://<ESP32-IP>/wake
```

#### GET /reset
Perform a hard/brute reset with proper cleanup. This endpoint:
1. Deinitializes the camera to free resources
2. Closes all active sockets to prevent memory corruption
3. Calls `esp_restart()` to perform a complete system reset

```bash
curl http://<ESP32-IP>/reset
```

**Response:**
```json
{
  "success": true,
  "message": "Performing hard reset with cleanup..."
}
```

#### GET /stop
Stop the camera service and enter power-down mode. This endpoint:
1. Deinitializes the camera driver
2. Frees frame buffer memory
3. Puts the OV2640 sensor in power-down mode via PWDN pin
4. Enables WiFi power save mode to reduce consumption
5. WiFi remains active for receiving commands

```bash
curl http://<ESP32-IP>/stop
```

**Response:**
```json
{
  "success": true,
  "message": "Camera service stopped and sensor in power-down mode. WiFi remains active."
}
```

#### GET /start
Start or restart the camera service. This endpoint:
1. If camera is running, forces complete reinitialization (deinit + init)
2. If camera is stopped, initializes it normally
3. Disables WiFi power save mode for optimal streaming performance
4. Ensures hardware is properly reset

```bash
curl http://<ESP32-IP>/start
```

**Response:**
```json
{
  "success": true,
  "message": "Camera service started and ready for streaming"
}
```

### WiFi Management

#### GET /wifi-scan
Scan for available WiFi networks.

```bash
curl http://<ESP32-IP>/wifi-scan
```

**Response:**
```json
[
  {
    "ssid": "MyWiFi",
    "rssi": -45,
    "encryption": 3
  },
  {
    "ssid": "NeighborWiFi",
    "rssi": -72,
    "encryption": 3
  }
]
```

#### POST /wifi-connect
Connect to a WiFi network.

```bash
curl -X POST http://<ESP32-IP>/wifi-connect \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MyWiFi","password":"mypassword"}'
```

**Response:**
```json
{
  "success": true,
  "ip": "192.168.1.100"
}
```

### System Management

#### GET /restart
Restart the ESP32.

```bash
curl http://<ESP32-IP>/restart
```

## Configuration File

Configuration is stored in `/config/config.json` on the SD card, or in NVS flash if SD card is not available.

**Example configuration:**
```json
{
  "networks": [
    {
      "ssid": "YourWiFiSSID",
      "password": "YourWiFiPassword",
      "priority": 1
    }
  ],
  "camera": {
    "framesize": 7,
    "quality": 12,
    "brightness": 0,
    "contrast": 0,
    "saturation": 0,
    "led_intensity": 0
  },
  "admin_password_hash": "",
  "ota_enabled": false,
  "log_level": 2,
  "server_port": 80
}
```

## Architecture

### Dual-Core Task Design

The project uses FreeRTOS tasks with strict core pinning for maximum efficiency:

- **Core 0 (PRO_CPU) - Network & Services**:
  - Web Server Task - Handles all HTTP requests and responses
  - Watchdog Task - Monitors system health and memory
  - WiFi Stack - ESP32 WiFi driver (automatic)
  - **Purpose**: Dedicated exclusively to network operations and HTTP serving

- **Core 1 (APP_CPU) - Camera & Image Processing**:
  - Camera Task - Handles frame capture and sensor management
  - OV2640 Control - Camera sensor configuration and control
  - **Purpose**: Dedicated exclusively to camera operations and image processing

### Camera Driver Configuration

- **Interface**: I2S parallel mode with DMA for high-speed data transfer
- **Frame Buffers**: 2 buffers allocated in PSRAM (when available)
- **Buffer Location**: `CAMERA_FB_IN_PSRAM` for zero-copy optimization
- **Grab Mode**: `CAMERA_GRAB_LATEST` to always get the freshest frame
- **Format**: MJPEG (JPEG compressed frames) for minimal CPU overhead
- **PWDN Control**: GPIO 32 used for hardware power-down of OV2640 sensor

### Memory Management

- Frame buffers acquired with mutex protection for thread safety
- Buffers released immediately after use (`esp_camera_fb_return()`)
- PSRAM used when available for 2 frame buffers (double buffering)
- Strict memory management to prevent leaks during start/stop cycles
- Configuration limited to 2KB JSON to minimize heap usage

### Power Management

- **Camera Power-Down**: PWDN pin (GPIO 32) controls OV2640 power state
- **WiFi Power Save**: `WIFI_PS_MIN_MODEM` when camera stopped, `WIFI_PS_NONE` when streaming
- **Memory Release**: All frame buffers freed when camera is stopped
- **Low Power Mode**: Complete shutdown of camera subsystem via `/stop` endpoint

### Inter-Task Communication

- **Camera Mutex**: Protects camera hardware access between cores
- **Config Mutex**: Protects configuration data access
- **Event Queue**: Event-driven architecture for WiFi, config updates, system events
- **Event Types**: WiFi connected/disconnected, config updated, restart requested

## Development

### Building from Source

```bash
# Clone repository
git clone https://github.com/kevin-william/esp32-robust-video-stream-server.git
cd esp32-robust-video-stream-server

# Build with PlatformIO
pio run

# Build debug version
pio run -e esp32cam-debug

# Clean build
pio run --target clean
```

### Serial Monitor

```bash
# PlatformIO
pio device monitor -b 115200

# Arduino IDE
Tools > Serial Monitor (set to 115200 baud)
```

### Customization

#### Change Camera Model

Edit `platformio.ini`:
```ini
build_flags = 
    -DCAMERA_MODEL_WROVER_KIT  # Instead of AI_THINKER
```

#### Adjust Task Priorities

Edit `include/config.h`:
```cpp
#define CAMERA_TASK_PRIORITY 2
#define WEB_TASK_PRIORITY 2
#define WATCHDOG_TASK_PRIORITY 3
```

## Troubleshooting

### Camera Initialization Failed
- Check camera module connection
- Verify pin definitions match your board
- Ensure sufficient power supply (5V 2A recommended)

### WiFi Connection Issues
- Verify SSID and password are correct
- Check 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Move closer to router for initial setup

### Low Memory Errors
- Reduce frame size in camera settings
- Increase quality number (lower quality, less memory)
- Ensure PSRAM is enabled if available

### SD Card Not Detected
- Verify SD card is formatted as FAT32
- Check SD card pins are correctly connected
- Try a different SD card (some cards are incompatible)

### Stream Lag or Low FPS
- Reduce frame size (SVGA or lower)
- Increase quality number (10-15 range)
- Check WiFi signal strength
- Close other connections to ESP32

## Security Considerations

⚠️ **Important Security Notes:**

- The current implementation is designed for **local/trusted networks only**
- Authentication is basic - enhance for production use
- No HTTPS by default (can be enabled but impacts performance)
- Change default AP password before deployment
- Admin password stored as hash, never plaintext
- Consider implementing proper JWT or OAuth for production

## Performance Optimization

### Recommended Settings by Resolution

| Resolution | Framesize | Quality | PSRAM | Expected FPS |
|------------|-----------|---------|-------|--------------|
| QVGA (320x240) | 5 | 12 | No | 20-25 |
| VGA (640x480) | 8 | 12 | No | 15-20 |
| SVGA (800x600) | 7 | 12 | Recommended | 15-20 |
| XGA (1024x768) | 10 | 10 | Required | 10-15 |
| UXGA (1600x1200) | 13 | 10 | Required | 5-10 |

## License

Apache License 2.0 - See LICENSE file for details

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes with clear commits
4. Test thoroughly on hardware
5. Submit a pull request

## Roadmap

- [ ] HTTPS support with certificates
- [ ] Motion detection
- [ ] Image storage to SD card
- [ ] Time-lapse recording
- [ ] Face detection (if PSRAM available)
- [ ] WebSocket support for bi-directional communication
- [ ] Multi-language web UI
- [ ] Mobile app integration

## Credits

Built using:
- [ESP32-Camera](https://github.com/espressif/esp32-camera) library
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ArduinoJson](https://arduinojson.org/)

## Support

For issues, questions, or contributions:
- Open an issue on GitHub
- Check existing issues and discussions
- Provide serial monitor output for debugging

---

**Happy Streaming! 📹**
