# Changelog

All notable changes to the ESP32-CAM Robust Video Stream Server project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned Features
- HTTPS support with certificate management
- Motion detection with event triggers
- Image storage to SD card with rotation
- Time-lapse recording
- Face detection (PSRAM required)
- WebSocket support for bi-directional communication
- Multi-language web UI (English, Portuguese, Spanish)
- Mobile app integration (iOS/Android)
- Advanced OTA with rollback capability
- Cloud integration (AWS, Azure, Firebase)

## [0.1.0] - 2024-12-19

### Added - Milestone 1 (M1): Project Skeleton

- **Build System**
  - PlatformIO configuration with ESP32-CAM target
  - Arduino IDE compatible project structure
  - Support for both AI-Thinker and WROVER-KIT camera models
  - Debug build environment

- **Core Architecture**
  - Multi-core FreeRTOS task management
  - Camera task (Core 1) - dedicated to camera operations
  - Web server task (Core 0) - handles HTTP requests
  - Watchdog task (Core 0) - system health monitoring
  - Inter-task communication via queues and mutexes

- **Pin Definitions**
  - Complete pin mappings for ESP32-CAM AI-Thinker
  - Support for WROVER-KIT and ESP-EYE variants
  - Configurable LED flash control
  - SD card SPI pin configuration

- **Camera Module**
  - Camera initialization with PSRAM detection
  - Dynamic resolution and quality configuration
  - Thread-safe frame capture with mutex protection
  - Sleep/wake functionality (esp_camera_deinit/init)
  - LED flash control with PWM intensity
  - Comprehensive camera sensor parameter control

- **Configuration Management**
  - JSON-based configuration system
  - SD card primary storage (/config/config.json)
  - NVS fallback for SD-less operation
  - Configuration validation and schema enforcement
  - WiFi credentials with priority support (up to 3 networks)
  - Camera settings persistence
  - Default configuration generation

- **Storage Module**
  - SD card mounting with SPI interface
  - File I/O operations (read/write/delete)
  - Directory management
  - NVS (Preferences) integration
  - Graceful degradation on storage failure

- **WiFi & Captive Portal**
  - Automatic WiFi connection to saved networks
  - Priority-based network selection
  - Access Point mode with captive portal
  - DNS server for portal redirect
  - 5-minute timeout for AP mode
  - WiFi scanning and network discovery
  - Dynamic network connection

- **Web Server & REST API**
  - Asynchronous web server (ESPAsyncWebServer)
  - CORS headers for cross-origin requests
  - Comprehensive REST API endpoints:
    - `GET /status` - System status with memory, WiFi, camera info
    - `GET /sleepstatus` - Lightweight camera sleep status
    - `GET /capture` - Single JPEG image capture
    - `GET /stream` - MJPEG multipart streaming
    - `GET /control` - Camera parameter adjustment
    - `GET /sleep` - Put camera to sleep mode
    - `GET /wake` - Wake camera from sleep
    - `GET /restart` - System restart
    - `GET /wifi-scan` - Scan available networks
    - `POST /wifi-connect` - Connect to WiFi network
  - Web UI for configuration and control
  - Proper HTTP status codes (200, 400, 500, 503)

- **Documentation**
  - Comprehensive README with quick start guide
  - API reference with all endpoints documented
  - Architecture documentation explaining design decisions
  - Example configuration file
  - Hardware pinout diagrams
  - Troubleshooting guide

- **Build & Development**
  - .gitignore for build artifacts
  - Example configuration files
  - Directory structure for future expansion

### Security

- Admin password hashing (SHA256)
- CSRF token generation framework
- Authentication middleware (basic implementation)
- Secure configuration storage

### Performance

- PSRAM detection and automatic configuration
- Frame buffer optimization (immediate release)
- Memory monitoring and leak detection
- Optimized task priorities for responsiveness
- Frame rate limiting to prevent CPU saturation

### Known Limitations

- BMP conversion returns JPEG (planned for future)
- OTA update endpoints defined but not implemented
- Authentication is basic (production needs JWT/OAuth)
- No HTTPS support yet (performance trade-off)
- Single concurrent stream recommended
- No rate limiting on API endpoints

## [0.0.1] - 2024-12-18

### Added
- Initial repository setup
- Project title and basic README

---

## Version History

- **0.1.0**: Milestone 1 - Complete project skeleton with core functionality
- **0.0.1**: Initial repository creation

---

## Migration Guide

### From 0.0.1 to 0.1.0

This is the first functional release. No migration needed.

---

## Contributors

Thank you to all contributors who have helped make this project possible!

- Project maintainers
- ESP32 community
- Library authors (esp-camera, ESPAsyncWebServer, ArduinoJson)

---

## Support

For questions, bug reports, or feature requests:
- Open an issue on GitHub
- Check documentation in `/docs`
- Review existing issues and discussions

---

**Note**: This project is in active development. APIs may change between versions. Check the changelog before upgrading.
