# Architecture Documentation

## Overview

The ESP32-CAM Robust Video Stream Server is designed as a multi-core, multi-tasking application leveraging FreeRTOS to maximize performance and responsiveness on the ESP32 dual-core architecture.

## System Architecture

### Core Assignment Strategy

**Core 0 (PRO_CPU - Protocol CPU):**
- Handles network operations (WiFi, HTTP server)
- Runs web server task
- Executes watchdog and monitoring tasks
- Manages SD card I/O

**Core 1 (APP_CPU - Application CPU):**
- Dedicated to camera operations
- Frame capture and preprocessing
- Image processing tasks
- Isolated from network interrupts

### Task Hierarchy

```
┌─────────────────────────────────────────────────────┐
│                    Main Loop                        │
│   - Event Queue Processing                          │
│   - Captive Portal DNS Handling                     │
│   - System Coordination                             │
└─────────────────────────────────────────────────────┘
                         │
        ┌────────────────┼────────────────┐
        │                │                │
┌───────▼──────┐  ┌──────▼───────┐  ┌────▼──────────┐
│ Camera Task  │  │ Web Server   │  │ Watchdog Task │
│  (Core 1)    │  │  Task        │  │  (Core 0)     │
│  Priority: 2 │  │  (Core 0)    │  │  Priority: 3  │
│              │  │  Priority: 2 │  │               │
└──────────────┘  └──────────────┘  └───────────────┘
```

## Component Architecture

### 1. Camera Module (`camera.cpp`)

**Responsibilities:**
- Initialize ESP32 camera with appropriate settings
- Manage camera lifecycle (init/deinit/sleep/wake)
- Provide thread-safe frame capture
- Configure sensor parameters dynamically
- LED flash control

**Key Functions:**
- `initCamera()`: Configure and initialize camera with PSRAM detection
- `deinitCamera()`: Safe shutdown and memory release
- `captureFrame()`: Mutex-protected frame acquisition
- `releaseFrame()`: Immediate buffer release to prevent memory leaks

**Memory Strategy:**
- Uses PSRAM when available (4MB for larger buffers)
- Frame buffers acquired only when needed
- Immediate release after transmission
- Pre-allocated buffers based on PSRAM availability

### 2. Configuration Manager (`config.cpp`)

**Responsibilities:**
- Load/save configuration from SD card or NVS
- Validate configuration schema
- Provide default configuration
- Manage WiFi credentials
- Handle camera settings persistence

**Configuration Hierarchy:**
1. SD Card (`/config/config.json`) - Primary storage
2. NVS Flash - Fallback when SD unavailable
3. Defaults - Used on first boot

**Configuration Schema:**
```json
{
  "networks": [],        // WiFi credentials with priority
  "camera": {},          // Camera sensor settings
  "admin_password_hash": "", // SHA256 hash
  "ota_enabled": false,
  "log_level": 2,
  "server_port": 80
}
```

### 3. Storage Module (`storage.cpp`)

**Responsibilities:**
- SD card mounting and management
- File I/O operations
- NVS (Non-Volatile Storage) operations
- Error handling for storage failures

**SD Card Strategy:**
- Separate SPI bus from camera
- FAT32 filesystem
- Graceful degradation if SD fails
- Automatic fallback to NVS

### 4. Captive Portal (`captive_portal.cpp`)

**Responsibilities:**
- Access Point mode management
- DNS server for captive portal
- WiFi scanning
- Network connection management
- Multi-network priority handling

**Captive Portal Flow:**
```
┌─────────────┐
│  Power On   │
└──────┬──────┘
       │
       ▼
┌──────────────────┐      Yes    ┌─────────────────┐
│ Saved Networks?  ├─────────────►│ Connect to WiFi │
└──────┬───────────┘              └────────┬────────┘
       │ No                                │
       ▼                                   │ Success
┌──────────────────┐                       │
│  Start AP Mode   │                       │
│  DNS Server      │                       │
└──────┬───────────┘                       │
       │                                   │
       ▼                                   │
┌──────────────────┐                       │
│ Serve Config UI  │                       │
│ (5 min timeout)  │                       │
└──────┬───────────┘                       │
       │                                   │
       ▼                                   │
┌──────────────────┐                       │
│ User Configures  ├───────────────────────┘
└──────────────────┘
```

**Timeout Behavior:**
- 5-minute window for configuration
- Automatic restart if no configuration
- Prevents indefinite AP mode (battery preservation)

### 5. Web Server (`web_server.cpp`)

**Responsibilities:**
- Asynchronous HTTP server (ESPAsyncWebServer)
- REST API endpoint implementation
- CORS header management
- Request routing
- Response streaming (MJPEG)

**API Design Principles:**
- RESTful endpoints
- JSON responses
- Proper HTTP status codes
- CORS enabled for web client integration
- Chunked transfer for streaming

**Streaming Architecture:**
```
Client Request → AsyncWebServer → Chunked Response Handler
                                         │
                    ┌────────────────────┘
                    ▼
            ┌───────────────┐
            │ Acquire Frame │ (with mutex)
            └───────┬───────┘
                    │
                    ▼
            ┌───────────────┐
            │ Build Headers │
            └───────┬───────┘
                    │
                    ▼
            ┌───────────────┐
            │  Send Buffer  │
            └───────┬───────┘
                    │
                    ▼
            ┌───────────────┐
            │ Release Frame │
            └───────┬───────┘
                    │
                    └──────► Repeat
```

## Synchronization Mechanisms

### Mutexes

1. **cameraMutex**: Protects camera frame buffer access
   - Acquired before `esp_camera_fb_get()`
   - Released after buffer pointer obtained
   - Prevents concurrent frame captures

2. **configMutex**: Protects configuration read/write
   - Used during configuration updates
   - Prevents race conditions on config changes
   - Guards file I/O operations

### Event Queue

**Purpose**: Inter-task communication without tight coupling

**Event Types:**
- `EVENT_WIFI_CONNECTED`: WiFi successfully connected
- `EVENT_WIFI_DISCONNECTED`: WiFi connection lost
- `EVENT_CONFIG_UPDATED`: Configuration changed, save required
- `EVENT_CAMERA_ERROR`: Camera failure, reinit needed
- `EVENT_RESTART_REQUESTED`: System restart triggered

**Flow:**
```
Task A                  Event Queue              Main Loop
   │                         │                        │
   ├──── Enqueue Event ─────►│                        │
   │                         │                        │
   │                         │◄──── Dequeue Event ────┤
   │                         │                        │
   │                         │                   Handle Event
```

## Memory Management

### Heap Allocation Strategy

**Static Allocations:**
- Task stacks (8KB for camera/web, 4KB for watchdog)
- Configuration structure (< 4KB)
- Synchronization primitives

**Dynamic Allocations:**
- Camera frame buffers (managed by esp_camera)
- HTTP response buffers (managed by AsyncWebServer)
- JSON parsing (ArduinoJson with bounded allocators)

### PSRAM Utilization

When PSRAM is detected:
- Frame buffer: UXGA (1600x1200) @ 2 buffers
- Quality: Higher quality (lower compression)
- Frame count: 2 (double buffering)

Without PSRAM:
- Frame buffer: SVGA (800x600) @ 1 buffer
- Quality: Lower quality (higher compression)
- Frame count: 1 (single buffering)

### Memory Monitoring

Watchdog task monitors:
- Free heap (alert if < 10KB)
- Minimum free heap (lifetime low)
- PSRAM usage (if available)
- Memory leak detection (decreasing minimums)

## Boot Sequence

```
1. System Init
   ├── Serial console (115200 baud)
   ├── Create synchronization primitives
   └── Initialize LED (off)

2. Storage Init
   ├── Try mount SD card
   └── Open NVS namespace

3. Configuration
   ├── Set defaults
   ├── Load from SD or NVS
   └── Validate schema

4. Network Init
   ├── Try saved networks (priority order)
   ├── If fail: Start AP + captive portal
   └── Wait for connection or timeout

5. Camera Init
   ├── Detect PSRAM
   ├── Configure camera
   └── Apply saved settings

6. Web Server
   ├── Register routes
   ├── Start async server
   └── Begin listening

7. Task Creation
   ├── Camera task (Core 1)
   ├── Web server task (Core 0)
   └── Watchdog task (Core 0)

8. Main Loop
   └── Event processing
```

## Design Decisions & Rationale

### Why Multi-Core?

**Problem**: Camera operations are time-sensitive and can be blocked by network I/O.

**Solution**: Dedicate Core 1 to camera, Core 0 to networking.

**Benefit**: Consistent frame rate, no jitter from WiFi interrupts.

### Why Async Web Server?

**Problem**: Synchronous server blocks on slow clients, affecting other requests.

**Solution**: ESPAsyncWebServer handles multiple connections without blocking.

**Benefit**: Concurrent streams, lower latency, better UX.

### Why SD + NVS Dual Storage?

**Problem**: SD cards can be removed or fail; NVS has limited space.

**Solution**: Primary on SD, fallback to NVS.

**Benefit**: Flexibility and reliability.

### Why 5-Minute AP Timeout?

**Problem**: Indefinite AP mode drains battery, creates zombie APs.

**Solution**: Auto-restart after 5 minutes.

**Benefit**: Forces correct configuration, reduces support issues.

### Why Immediate Frame Release?

**Problem**: Holding frame buffers causes memory exhaustion.

**Solution**: `esp_camera_fb_return(fb)` immediately after copy/send.

**Benefit**: Stable memory usage, prevents crashes.

## Performance Characteristics

### Typical Metrics (ESP32-CAM with PSRAM)

- **Boot Time**: 3-5 seconds
- **WiFi Connection**: 5-10 seconds
- **Camera Init**: 1-2 seconds
- **Frame Capture**: 50-200ms (depends on resolution)
- **MJPEG Stream**: 10-20 FPS @ SVGA
- **Memory Usage**: 60-80% heap, 10-20% PSRAM

### Bottlenecks

1. **WiFi Bandwidth**: Limits max resolution/FPS
2. **JPEG Encoding**: CPU-intensive, affects frame rate
3. **I2C Bus**: Camera sensor config can block briefly

### Optimization Opportunities

- Hardware JPEG encoding (if sensor supports)
- Frame rate limiting to reduce CPU load
- Downsampling for lower bandwidth requirements
- WebP or H.264 encoding (requires significant work)

## Security Architecture

### Current Implementation

- **Authentication**: Basic (placeholder for production tokens)
- **CSRF**: Token generation available
- **Passwords**: SHA256 hashed, never plaintext
- **HTTPS**: Optional (disabled by default due to overhead)

### Recommended Enhancements

1. Implement JWT-based authentication
2. Add API rate limiting
3. Enable HTTPS with proper certificates
4. Add brute-force protection
5. Implement proper session management

## Error Handling Strategy

### Camera Errors
- Retry initialization up to 3 times
- Return 503 Service Unavailable if sleeping
- Log errors to serial console

### Network Errors
- Auto-reconnect on WiFi drop
- Fallback to AP mode if all networks fail
- Timeout long operations

### Storage Errors
- Graceful degradation (NVS fallback)
- Continue operation without SD
- Log warnings, don't crash

## Future Architectural Improvements

1. **Task Scheduler**: Custom scheduler for better camera timing
2. **DMA Transfers**: Reduce CPU involvement in data movement
3. **H.264 Encoding**: Hardware encoder for better compression
4. **Edge Computing**: On-device ML for motion/face detection
5. **Multi-Camera**: Support multiple camera modules

---

This architecture provides a solid foundation for a production ESP32-CAM application with room for future enhancements.
