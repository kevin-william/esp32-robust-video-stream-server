# HC-SR501 Motion Sensor Integration

## Overview

This ESP32-CAM project now supports motion-activated recording using the HC-SR501 PIR (Passive Infrared) motion sensor. When motion monitoring is enabled, the camera remains off to save power and automatically starts recording when motion is detected.

## Features

- **Motion-Activated Recording**: Camera starts automatically when motion is detected
- **Power Efficient**: Camera stays off when no motion, reducing power consumption
- **Configurable Duration**: Set recording duration (default: 5 seconds)
- **Smart Timer**: Recording extends if motion continues during recording
- **SD Card Storage**: Videos saved to SD card in MJPEG format
- **REST API Control**: Enable/disable motion monitoring via web API

## Hardware Setup

### Pin Connections

#### ESP32-CAM (AI-Thinker)
- **PIR Sensor OUT → GPIO 33**
- **PIR VCC → 5V**
- **PIR GND → GND**

#### ESP32-WROVER-KIT
- **PIR Sensor OUT → GPIO 12**
- **PIR VCC → 5V**
- **PIR GND → GND**

### HC-SR501 Sensor Configuration

The HC-SR501 has two potentiometers:
- **Sensitivity (Sx)**: Adjust detection range (3-7 meters)
- **Time Delay (Tx)**: Set high-level output time (0.3s - 5min recommended: ~2-5s)

**Jumper Settings**:
- Set to **H** (repeatable trigger mode) for continuous motion detection
- Alternatively, use **L** (single trigger mode) for individual motion events

## Software Configuration

### Enable Motion Monitoring

Motion monitoring requires:
1. SD card inserted and mounted
2. Motion monitoring enabled in configuration

### Configuration Options

Add to your config file (`/config/config.json` on SD card):

```json
{
  "motion": {
    "enabled": true,
    "recording_duration_sec": 5,
    "debounce_ms": 200
  }
}
```

**Parameters**:
- `enabled`: Enable/disable motion monitoring (requires SD card)
- `recording_duration_sec`: Duration to record after last motion (default: 5 seconds)
- `debounce_ms`: Debounce time for motion detection (default: 200ms)

## REST API Endpoints

### Get Motion Status
```bash
GET /motion/status
```

Response:
```json
{
  "motion_monitoring_enabled": true,
  "motion_monitoring_active": true,
  "motion_recording_active": false,
  "sd_card_mounted": true,
  "recording_duration_sec": 5,
  "time_since_last_motion_ms": 1234
}
```

### Enable Motion Monitoring
```bash
GET /motion/enable
```

Response:
```json
{
  "success": true,
  "message": "Motion monitoring enabled. Please restart device for changes to take effect."
}
```

**Note**: Requires SD card to be mounted. Device restart needed to activate.

### Disable Motion Monitoring
```bash
GET /motion/disable
```

Response:
```json
{
  "success": true,
  "message": "Motion monitoring disabled. Please restart device for changes to take effect."
}
```

## How It Works

### State Machine

The motion monitoring system operates in four states:

1. **IDLE**: Waiting for motion detection
   - Camera is OFF (power saving)
   - Monitoring PIR sensor via interrupt
   
2. **MOTION_DETECTED**: Motion just detected
   - Initializing camera
   - Creating new video file
   
3. **RECORDING**: Active recording
   - Capturing frames at ~5 FPS
   - Writing to MJPEG file
   - Timer resets if new motion detected
   
4. **STOPPING**: Finalizing recording
   - Closing video file
   - Shutting down camera
   - Returning to IDLE

### Recording Behavior

- Recording starts when motion is detected
- Records for the configured duration (default: 5 seconds)
- **If motion continues**: Timer resets and recording continues in the same file
- **When no motion**: After duration expires, recording stops and camera powers down
- Videos are saved to `/recordings/motion_TIMESTAMP.mjpeg`

### Video Format

Videos are saved in MJPEG format:
- Each frame is a JPEG image
- Frame size header (4 bytes) before each JPEG
- Lightweight and optimized for ESP32
- Can be played with VLC or converted to other formats

## File Structure

New files added:
```
include/
  └── motion_sensor.h          # Motion sensor interface
src/
  ├── motion_sensor.cpp        # Motion sensor implementation
  └── motion_monitoring.cpp    # Motion monitoring task
```

Modified files:
```
include/
  ├── app.h                    # Added motion monitoring task
  ├── camera_pins.h            # Added PIR_SENSOR_PIN
  ├── config.h                 # Added MotionSettings
  ├── storage.h                # Added video recording functions
  └── web_server.h             # Added motion API endpoints
src/
  ├── config.cpp               # Motion settings handling
  ├── main.cpp                 # Motion monitoring integration
  ├── storage.cpp              # Video recording implementation
  └── web_server.cpp           # Motion API implementation
```

## System Architecture

When motion monitoring is enabled:
- **Core 0 (PRO_CPU)**: WiFi + HTTP Server + Watchdog
- **Core 1 (APP_CPU)**: Motion Monitoring Task (manages camera lifecycle)

The motion monitoring task runs on Core 1 to have direct access to camera operations without cross-core synchronization overhead.

## Example Usage

### Basic Setup

1. Insert SD card into ESP32-CAM
2. Connect HC-SR501 sensor to GPIO 33 (AI-Thinker) or GPIO 12 (WROVER)
3. Power on and configure WiFi via captive portal
4. Enable motion monitoring:
   ```bash
   curl http://ESP32-IP/motion/enable
   ```
5. Restart the device
6. Motion recordings will be saved to `/recordings/` on SD card

### Check Status

```bash
curl http://ESP32-IP/motion/status
```

### Retrieve Recordings

Connect SD card to computer and navigate to `/recordings/` folder. Video files are named with Unix timestamps (e.g., `motion_1234567890.mjpeg`).

## Troubleshooting

### Motion sensor not detecting
- Check wiring connections
- Verify GPIO pin configuration
- Adjust sensitivity potentiometer on HC-SR501
- Check sensor has warmed up (30-60 seconds after power on)

### No recordings on SD card
- Verify SD card is mounted (`/status` endpoint shows `sd_card_mounted: true`)
- Check `/recordings` directory exists
- Verify enough free space on SD card
- Check serial output for error messages

### False triggers
- Increase `debounce_ms` in configuration
- Adjust HC-SR501 sensitivity potentiometer
- Keep sensor away from heat sources and direct sunlight
- Ensure sensor is stable and not vibrating

### Camera not starting
- Check camera connections
- Verify camera works in normal mode (motion disabled)
- Check power supply is adequate (5V, 2A recommended)
- Review serial output for initialization errors

## Performance Notes

- **Power Consumption**: Camera off when idle saves ~100-150mA
- **Recording Frame Rate**: ~5 FPS (configurable via `frame_interval_ms`)
- **Video File Size**: ~50-150KB per second (depends on scene complexity and quality settings)
- **SD Card Speed**: Class 10 or higher recommended for reliable recording

## Future Enhancements

Potential improvements:
- [ ] Pre/post-motion buffering
- [ ] Multiple video format support (AVI, MP4)
- [ ] Motion detection zones
- [ ] Scheduled monitoring (time-based activation)
- [ ] Email/notification on motion detection
- [ ] Video thumbnail generation

## License

This feature is part of the ESP32-CAM Robust Video Stream Server project and follows the same Apache-2.0 license.
