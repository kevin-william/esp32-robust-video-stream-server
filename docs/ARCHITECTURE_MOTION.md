# Motion Sensor System Architecture

## Overview Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          ESP32-CAM System                               │
│                                                                         │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │                    HC-SR501 PIR Sensor                            │ │
│  │                    (Connected to GPIO 33/12)                      │ │
│  │                                                                   │ │
│  │   OUT ────────────► GPIO Pin (Interrupt on RISING edge)          │ │
│  │   VCC ────────────► 5V                                           │ │
│  │   GND ────────────► GND                                          │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│                              ▼                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │              Motion Sensor Module (motion_sensor.cpp)             │ │
│  │                                                                   │ │
│  │  ┌─────────────────────────────────────────────────────────────┐ │ │
│  │  │  ISR (Interrupt Service Routine)                            │ │ │
│  │  │  - Detects motion via GPIO interrupt                        │ │ │
│  │  │  - Debouncing (200ms default)                               │ │ │
│  │  │  - Sets motion_detected flag                                │ │ │
│  │  │  - Updates last_motion_time                                 │ │ │
│  │  └─────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│                              ▼                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │      Motion Monitoring Task (motionMonitoringTask)                │ │
│  │      Running on Core 1 (APP_CPU) - Priority 2                    │ │
│  │                                                                   │ │
│  │  ╔═══════════════════════════════════════════════════════════╗   │ │
│  │  ║              State Machine                                ║   │ │
│  │  ╠═══════════════════════════════════════════════════════════╣   │ │
│  │  ║                                                           ║   │ │
│  │  ║   ┌────────┐  Motion    ┌──────────────────┐            ║   │ │
│  │  ║   │  IDLE  │ Detected   │ MOTION_DETECTED  │            ║   │ │
│  │  ║   │        │ ────────►  │   (Init Camera)  │            ║   │ │
│  │  ║   └────────┘            └──────────────────┘            ║   │ │
│  │  ║       ▲                          │                      ║   │ │
│  │  ║       │                          ▼                      ║   │ │
│  │  ║       │                  ┌──────────────┐               ║   │ │
│  │  ║   ┌────────────┐         │  RECORDING   │               ║   │ │
│  │  ║   │  STOPPING  │ ◄────── │ (Capture &   │               ║   │ │
│  │  ║   │ (Finalize) │ Timeout │  Write ~5fps)│               ║   │ │
│  │  ║   └────────────┘         └──────────────┘               ║   │ │
│  │  ║                              │     ▲                     ║   │ │
│  │  ║                              └─────┘                     ║   │ │
│  │  ║                          Motion continues                ║   │ │
│  │  ║                          (timer resets)                  ║   │ │
│  │  ╚═══════════════════════════════════════════════════════════╝   │ │
│  │                                                                   │ │
│  │  Operations:                                                     │ │
│  │  - initCamera() / deinitCamera()                                │ │
│  │  - captureFrame() / releaseFrame()                              │ │
│  │  - initVideoRecording()                                         │ │
│  │  - writeFrameToVideo()                                          │ │
│  │  - finalizeVideoRecording()                                     │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│                              ▼                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │              Storage Module (storage.cpp)                         │ │
│  │                                                                   │ │
│  │  Video Recording Pipeline:                                       │ │
│  │  1. Create file: /recordings/motion_TIMESTAMP.mjpeg             │ │
│  │  2. For each frame:                                              │ │
│  │     - Write 4-byte size header                                   │ │
│  │     - Write JPEG frame data                                      │ │
│  │  3. Close and flush file                                         │ │
│  │                                                                   │ │
│  │  MJPEG Format:                                                   │ │
│  │  [4-byte size][JPEG frame 1]                                     │ │
│  │  [4-byte size][JPEG frame 2]                                     │ │
│  │  [4-byte size][JPEG frame 3]                                     │ │
│  │  ...                                                             │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│                              ▼                                          │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │                    SD Card Storage                                │ │
│  │                                                                   │ │
│  │  /recordings/                                                     │ │
│  │    ├── motion_1641234567.mjpeg   (234 KB)                        │ │
│  │    ├── motion_1641234789.mjpeg   (456 KB)                        │ │
│  │    └── motion_1641234890.mjpeg   (123 KB)                        │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                         │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │              Web API (Core 0 - PRO_CPU)                           │ │
│  │                                                                   │ │
│  │  GET /motion/status                                               │ │
│  │  ├─ Returns: enabled, active, recording, time_since_motion       │ │
│  │                                                                   │ │
│  │  GET /motion/enable                                               │ │
│  │  ├─ Requires: SD card mounted                                    │ │
│  │  └─ Effect: Enable + save config + requires restart              │ │
│  │                                                                   │ │
│  │  GET /motion/disable                                              │ │
│  │  └─ Effect: Disable + save config + requires restart             │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                                                                         │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │           Configuration (config.cpp)                              │ │
│  │                                                                   │ │
│  │  {                                                                │ │
│  │    "motion": {                                                    │ │
│  │      "enabled": true,                                             │ │
│  │      "recording_duration_sec": 5,                                 │ │
│  │      "debounce_ms": 200                                           │ │
│  │    }                                                              │ │
│  │  }                                                                │ │
│  │                                                                   │ │
│  │  Storage: /config/config.json (SD) + NVS (fallback)              │ │
│  └───────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

## Data Flow

```
┌─────────┐    GPIO      ┌──────────┐    Flag     ┌──────────────┐
│HC-SR501 │──Interrupt──►│   ISR    │──Set Flag──►│ Motion Task  │
│ Sensor  │              │(IRAM_ATTR)│             │  (Core 1)    │
└─────────┘              └──────────┘             └──────────────┘
                                                          │
                                                          │ Camera Control
                                                          ▼
                         ┌──────────────┐         ┌─────────────┐
                         │   Camera     │◄────────│   Camera    │
                         │   Hardware   │  I2S    │   Driver    │
                         └──────────────┘  DMA    └─────────────┘
                                │                        │
                                │ Frame                  │
                                ▼                        │
                         ┌──────────────┐               │
                         │  JPEG Frame  │               │
                         │    Buffer    │               │
                         └──────────────┘               │
                                │                        │
                                │ Write                  │
                                ▼                        │
                         ┌──────────────┐               │
                         │    Storage   │               │
                         │    Module    │               │
                         └──────────────┘               │
                                │                        │
                                ▼                        │
                         ┌──────────────┐               │
                         │   SD Card    │               │
                         │ /recordings/ │               │
                         └──────────────┘               │
                                                         │
                                                         ▼
                                                  ┌─────────────┐
                                                  │ Power Mgmt  │
                                                  │Camera OFF/ON│
                                                  └─────────────┘
```

## Power States

```
┌────────────────────────────────────────────────────────────────┐
│                         Power Management                       │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  IDLE State (No Motion)                                        │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  • Camera: OFF (PWDN)                  Power: ~50mA     │ │
│  │  • WiFi: ON                                             │ │
│  │  • Motion Sensor: Monitoring                            │ │
│  │  • Core 0: Web Server Active                            │ │
│  │  • Core 1: Motion Task (100ms polling)                  │ │
│  └──────────────────────────────────────────────────────────┘ │
│                              │                                 │
│                     Motion Detected!                           │
│                              ▼                                 │
│  RECORDING State                                               │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  • Camera: ON (Active)                 Power: ~200mA    │ │
│  │  • WiFi: ON                                             │ │
│  │  • Motion Sensor: Monitoring                            │ │
│  │  • Core 0: Web Server Active                            │ │
│  │  • Core 1: Capturing @ 5fps + Writing to SD            │ │
│  └──────────────────────────────────────────────────────────┘ │
│                              │                                 │
│                     5s No Motion                               │
│                              ▼                                 │
│  STOPPING State                                                │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  • Finalizing video file                                │ │
│  │  • Shutting down camera                                 │ │
│  │  • Returning to IDLE                                    │ │
│  └──────────────────────────────────────────────────────────┘ │
│                              │                                 │
│                              ▼                                 │
│                         Back to IDLE                           │
│                                                                │
│  Power Savings: ~100-150mA when idle (66% reduction)          │
└────────────────────────────────────────────────────────────────┘
```

## File Structure

```
esp32-robust-video-stream-server/
├── include/
│   ├── motion_sensor.h          ← NEW: Motion sensor interface
│   ├── app.h                    ← Modified: Added motion task handle
│   ├── camera_pins.h            ← Modified: Added PIR_SENSOR_PIN
│   ├── config.h                 ← Modified: Added MotionSettings
│   ├── storage.h                ← Modified: Video recording functions
│   └── web_server.h             ← Modified: Motion endpoints
├── src/
│   ├── motion_sensor.cpp        ← NEW: Sensor implementation
│   ├── motion_monitoring.cpp    ← NEW: Monitoring task
│   ├── main.cpp                 ← Modified: Integration
│   ├── config.cpp               ← Modified: Motion config
│   ├── storage.cpp              ← Modified: MJPEG recording
│   └── web_server.cpp           ← Modified: API endpoints
└── docs/
    ├── MOTION_SENSOR.md         ← NEW: English documentation
    ├── MOTION_SENSOR_PT.md      ← NEW: Portuguese documentation
    └── TESTING_MOTION_SENSOR.md ← NEW: Testing plan
```

## Timing Diagram

```
Time ──────────────────────────────────────────────────────────►

Motion:    ──┐           ┌──┐     ┌──┐              └────
Sensor:      └───────────┘  └─────┘  └──────────────

Camera:    OFF     ┌───────────────────────────┐     OFF
State:             │        ON & Recording     │
                   └───────────────────────────┘

Timer:     ─────   0   1   2   0   1   2   3   4   5

Recording: ────────┤ Start ├─────────────────┤ Stop ├──

File:      none    motion_1234567890.mjpeg    Saved
                   ▲                           ▲
                   │                           │
                   File Created                File Closed
                   234 KB                      Final: 456 KB
```

## Memory Layout

```
┌─────────────────────────────────────────────────────────────┐
│                       ESP32 Memory                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  PSRAM (4MB)                                                │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Frame Buffer 1 (640x480 JPEG)      ~30-60 KB      │   │
│  │  Frame Buffer 2 (640x480 JPEG)      ~30-60 KB      │   │
│  │  Available                           ~3.8 MB        │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  DRAM (Internal ~200KB)                                     │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  System Stack                        ~80 KB         │   │
│  │  Tasks Stacks                        ~28 KB         │   │
│  │    - Camera Task:        8 KB                       │   │
│  │    - Web Server Task:    8 KB                       │   │
│  │    - Motion Task:        8 KB        ← NEW          │   │
│  │    - Watchdog Task:      4 KB                       │   │
│  │  Motion Sensor State:    ~100 bytes  ← NEW          │   │
│  │  Recording State:        ~200 bytes  ← NEW          │   │
│  │  Available Heap:         ~90 KB                     │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```
