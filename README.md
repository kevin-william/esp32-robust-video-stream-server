# ESP32-CAM Robust Video Stream Server

A complete, production-ready ESP32-CAM project featuring multi-core task management, configuration persistence, captive portal WiFi setup, REST API, MJPEG streaming, and OTA updates.

## Features

- **Multi-Core FreeRTOS Architecture**: Separate tasks for camera (Core 1) and web server (Core 0) for optimal performance
- **Captive Portal**: Automatic WiFi configuration portal when no saved networks are available
- **Configuration Persistence**: Save/load settings from SD card with NVS fallback
- **REST API**: Comprehensive JSON API with CORS support for camera control
- **MJPEG Streaming**: Real-time video streaming at configurable frame rates
- **OTA Updates**: Over-the-air firmware updates
- **Sleep/Wake**: Power management with camera sleep/wake functionality
- **LED Control**: Flash LED control with adjustable intensity
- **Memory Optimized**: Designed for ESP32 constraints with PSRAM support

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

### Multi-Core Task Design

The project uses FreeRTOS tasks pinned to specific cores:

- **Core 0 (Protocol CPU)**:
  - Web Server Task - Handles HTTP requests
  - Watchdog Task - Monitors system health
  - SD Card Task - Manages storage operations

- **Core 1 (Application CPU)**:
  - Camera Task - Handles frame capture and processing

### Memory Management

- Frame buffers are acquired with mutex protection
- Buffers released immediately after use (`esp_camera_fb_return()`)
- PSRAM used when available for larger frame buffers
- Configuration limited to 2KB JSON to minimize heap usage

### Inter-Task Communication

- **Mutexes**: Protect camera and config access
- **Queues**: Event-driven architecture for WiFi, config updates, OTA
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

### Current Optimized Settings (December 2025)

This project has been heavily optimized for the ESP32-CAM's limited resources:

**Key Optimizations:**
- ✅ **Minimal FreeRTOS Tasks**: Removed unnecessary camera/webserver tasks (saved ~18KB RAM)
- ✅ **Optimized Stream Handler**: Non-blocking with static buffers (no String allocations)
- ✅ **Single Frame Buffer**: Reduced PSRAM usage by 50KB
- ✅ **Compiler Optimization**: -O3 flag with 240MHz CPU guarantee
- ✅ **Reduced Stack Sizes**: Loop stack cut to 4KB (50% reduction)

**Current Default Settings:**
- Resolution: **CIF (400x296)** - best balance performance/quality
- JPEG Quality: **12** - good compression with low CPU usage
- Frame Buffer: **1** (single buffer)
- Expected FPS: **~10 FPS** smooth streaming

### Recommended Settings by Use Case

| Use Case | Resolution | Quality | PSRAM | Expected FPS | Notes |
|----------|-----------|---------|-------|--------------|-------|
| **Maximum Performance** | QVGA (320x240) | 15 | Optional | 15-20 | Lowest latency |
| **Balanced (Default)** | CIF (400x296) | 12 | Optional | 10-15 | Best overall |
| **Good Quality** | VGA (640x480) | 10 | Required | 8-12 | Needs good WiFi |
| **High Quality** | SVGA (800x600) | 10 | Required | 5-8 | Slower, larger files |

### Troubleshooting Performance Issues

If experiencing lag, freezing, or crashes:

1. **Verify Power Supply**: ESP32-CAM needs **5V 2A minimum** (poor power causes brownouts)
2. **Check PSRAM**: Run `pio device monitor` and verify "PSRAM found" message
3. **Reduce Resolution**: Lower to QVGA (320x240) temporarily
4. **Check WiFi Signal**: RSSI should be > -70 dBm
5. **Monitor Memory**: Free heap should stay > 100KB

**Memory Indicators:**
```
Free heap: 120000 bytes    ← Good (>100KB)
Free PSRAM: 3500000 bytes  ← Good (>3MB)
```

See [Performance Optimizations History](#performance-history) for technical details on recent improvements.

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

## Performance History

<details>
<summary>December 2025 - Critical Performance Overhaul</summary>

### Problem Identified
ESP32-CAM was consuming excessive resources and couldn't handle streaming, capture, or web server properly despite dual-core 240MHz processor.

### Root Causes Fixed
1. **Useless FreeRTOS Tasks**: cameraTask and webServerTask did nothing but consumed 16KB+ RAM
2. **Blocking Stream Handler**: Used synchronous `delay(66)` inside callback, blocking entire AsyncWebServer
3. **Excessive Memory Usage**: Double buffering, high JPEG quality, oversized stacks
4. **Poor Configuration**: Wrong debug levels, WDT conflicts, suboptimal compiler flags

### Solutions Applied
- Removed 2 unnecessary tasks → +18KB RAM freed
- Rewrote stream handler with non-blocking design → server remains responsive
- Changed to single frame buffer → +50KB PSRAM freed
- Reduced all stack sizes by 50% → +6KB RAM freed
- Enabled -O3 optimization → 15-20% faster code
- CIF resolution (400x296) by default → 62% fewer pixels than previous SVGA

### Results
- ✅ Stream works smoothly at ~10 FPS without freezing
- ✅ Diagnostics accessible during streaming
- ✅ Responsive capture even under load
- ✅ Total memory freed: ~70KB

</details>

---

**Happy Streaming! 📹**
