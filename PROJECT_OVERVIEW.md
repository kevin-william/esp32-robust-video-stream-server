# Project Overview

## ESP32-CAM Robust Video Stream Server

A complete, production-ready firmware for ESP32-CAM boards with multi-core architecture, captive portal WiFi setup, REST API, MJPEG streaming, and comprehensive documentation.

## System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32-CAM Device                         │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    Core 1 (APP_CPU)                      │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │          Camera Task (Priority 2)                  │  │  │
│  │  │  • Frame Capture                                   │  │  │
│  │  │  • Image Processing                                │  │  │
│  │  │  • Buffer Management                               │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    Core 0 (PRO_CPU)                      │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │      Web Server Task (Priority 2)                  │  │  │
│  │  │  • HTTP Request Handling                           │  │  │
│  │  │  • API Endpoints                                   │  │  │
│  │  │  • MJPEG Streaming                                 │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │       Watchdog Task (Priority 3)                   │  │  │
│  │  │  • Health Monitoring                               │  │  │
│  │  │  • Memory Tracking                                 │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                  Storage & Network                       │  │
│  │  • SD Card (Primary Config)                              │  │
│  │  • NVS Flash (Fallback Config)                           │  │
│  │  • WiFi Station/AP Mode                                  │  │
│  │  • Captive Portal (DNS)                                  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Synchronization Primitives                  │  │
│  │  • Camera Mutex                                          │  │
│  │  • Config Mutex                                          │  │
│  │  • Event Queue (WiFi, Config, Errors)                   │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                         Network Layer                           │
│                                                                 │
│  WiFi AP Mode (Setup)          WiFi Station Mode (Normal)      │
│  ┌──────────────────┐          ┌──────────────────────────┐    │
│  │  192.168.4.1     │          │  DHCP IP Address         │    │
│  │  Captive Portal  │          │  REST API Endpoints      │    │
│  │  DNS Server      │          │  MJPEG Stream            │    │
│  └──────────────────┘          └──────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                         Client Layer                            │
│                                                                 │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌──────────┐ │
│  │  Web UI    │  │  Mobile    │  │   curl     │  │  Custom  │ │
│  │  Browser   │  │  App       │  │   Scripts  │  │  Client  │ │
│  └────────────┘  └────────────┘  └────────────┘  └──────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## Key Features

### 🎯 Core Functionality
- **Multi-core Architecture**: Camera on Core 1, Network on Core 0
- **MJPEG Streaming**: Real-time video at 10-20 FPS
- **REST API**: 11+ endpoints for control and monitoring
- **Configuration Persistence**: SD card + NVS fallback
- **Captive Portal**: Easy WiFi setup without hardcoding

### 🔧 Camera Control
- Resolution: QQVGA to UXGA (160x120 to 1600x1200)
- Quality: Adjustable JPEG compression (0-63)
- Settings: Brightness, contrast, saturation, mirror, flip
- LED Flash: PWM-controlled intensity (0-255)
- Power Management: Sleep/wake functionality

### 🌐 Network Features
- WiFi Client: Connect to saved networks with priority
- Access Point: Captive portal for first-time setup
- DNS Server: Redirect all requests to config page
- Multi-network: Store up to 3 WiFi credentials
- Auto-reconnect: Fallback to AP if connection fails

### 💾 Storage
- **SD Card**: Primary configuration storage
- **NVS Flash**: Fallback when SD unavailable
- **JSON Config**: Human-readable, easy to edit
- **Validation**: Schema checking on load

### 🔐 Security
- Password Hashing: SHA256 for admin credentials
- CSRF Protection: Token generation framework
- Authentication: Basic auth with extensibility
- CORS: Enabled for web client integration

### 📊 Monitoring
- System Status: Uptime, memory, WiFi, camera state
- Health Checks: Watchdog monitoring
- Error Reporting: HTTP status codes, error messages
- Diagnostics: Reset reason, free heap, PSRAM usage

## API Endpoints Quick Reference

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/status` | GET | System status and metrics |
| `/sleepstatus` | GET | Camera sleep state |
| `/capture` | GET | Single JPEG image |
| `/stream` | GET | MJPEG video stream |
| `/control` | GET | Adjust camera parameters |
| `/sleep` | GET | Put camera to sleep |
| `/wake` | GET | Wake camera from sleep |
| `/restart` | GET | Restart device |
| `/wifi-scan` | GET | Scan available networks |
| `/wifi-connect` | POST | Connect to WiFi |
| `/` | GET | Web control panel |

## Technology Stack

### Framework & Platform
- **Arduino Core**: ESP32 Arduino framework
- **FreeRTOS**: Multi-tasking and synchronization
- **ESP-IDF**: Low-level ESP32 APIs

### Libraries
- **esp32-camera**: Camera driver
- **ESPAsyncWebServer**: High-performance web server
- **ArduinoJson**: JSON parsing and generation
- **AsyncTCP**: Async network stack
- **Preferences**: NVS storage wrapper

### Build Systems
- **PlatformIO**: Primary (recommended)
- **Arduino IDE**: Compatible
- **Arduino CLI**: Supported

### Development Tools
- **GitHub Actions**: CI/CD pipeline
- **clang-format**: Code formatting
- **Git**: Version control

## File Structure

```
esp32-robust-video-stream-server/
├── src/                    # Application source code
│   ├── main.cpp           # Main entry point
│   ├── camera.cpp         # Camera operations
│   ├── web_server.cpp     # HTTP server & API
│   ├── config.cpp         # Configuration management
│   ├── storage.cpp        # SD/NVS storage
│   └── captive_portal.cpp # WiFi setup portal
├── include/               # Header files
│   ├── app.h             # Main application
│   ├── camera_pins.h     # Pin definitions
│   ├── config.h          # Configuration structures
│   ├── web_server.h      # Web server interface
│   ├── storage.h         # Storage interface
│   └── captive_portal.h  # Portal interface
├── docs/                  # Documentation
│   ├── API.md            # API reference
│   ├── ARCHITECTURE.md   # Design documentation
│   └── BUILD.md          # Build instructions
├── data/                  # Data files
│   ├── config/           # Configuration examples
│   └── www/              # Web UI files
├── scripts/              # Utility scripts
│   └── build-verify.sh   # Build verification
├── platformio.ini        # PlatformIO config
├── esp32-cam-server.ino  # Arduino IDE wrapper
├── README.md             # Main documentation
├── QUICKSTART.md         # Quick start guide
├── CONTRIBUTING.md       # Contribution guidelines
├── CHANGELOG.md          # Version history
└── LICENSE               # Apache 2.0 license
```

## Development Status

### ✅ Completed (Milestones 1-3)
- Project skeleton and build system
- Multi-core task architecture
- Camera initialization and control
- Configuration persistence (SD + NVS)
- Captive portal WiFi setup
- REST API endpoints
- MJPEG streaming
- Web UI control panel
- Comprehensive documentation

### 🚧 In Progress (Milestone 4)
- OTA firmware updates
- Advanced authentication

### 📋 Planned (Milestone 5+)
- HTTPS support
- Motion detection
- Face detection
- Time-lapse recording
- Unit tests
- OpenAPI specification

## Performance Metrics

| Metric | Typical Value |
|--------|--------------|
| Boot Time | 3-5 seconds |
| WiFi Connect | 5-10 seconds |
| Camera Init | 1-2 seconds |
| Frame Capture | 50-200ms |
| MJPEG FPS | 10-20 @ SVGA |
| Memory Usage | 60-80% heap |
| PSRAM Usage | 10-20% (if available) |

## Hardware Support

### Tested Boards
- ✅ ESP32-CAM (AI-Thinker) - Primary target
- ⚠️  ESP32-WROVER-KIT - Pin definitions included
- ⚠️  ESP-EYE - Pin definitions included

### Memory Requirements
- **Minimum**: ESP32 without PSRAM (SVGA max)
- **Recommended**: ESP32 with 4MB PSRAM (UXGA capable)

## License

Apache License 2.0 - Free for personal and commercial use

## Support & Community

- 📖 **Documentation**: Comprehensive guides in `docs/`
- 🐛 **Issues**: GitHub issue tracker
- 💬 **Discussions**: GitHub discussions
- 🤝 **Contributing**: See CONTRIBUTING.md

---

**Project Status**: ✅ Production Ready (v0.1.0)

**Last Updated**: December 2024
