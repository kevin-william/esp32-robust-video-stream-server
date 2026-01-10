# Motion Sensor Testing Plan

## Hardware Testing Checklist

### 1. Basic Connectivity
- [ ] Connect HC-SR501 to GPIO 33 (AI-Thinker) or GPIO 12 (WROVER)
- [ ] Verify 5V and GND connections
- [ ] Power on and wait 30-60 seconds for sensor warm-up
- [ ] LED on HC-SR501 should blink during warm-up

### 2. Motion Detection
- [ ] Wave hand in front of sensor
- [ ] Verify GPIO pin goes HIGH (3.3V)
- [ ] Check serial output for "Motion detected!" message
- [ ] Verify debouncing works (no rapid false triggers)

### 3. Camera Integration
- [ ] Start with motion monitoring disabled
- [ ] Verify normal camera operation works
- [ ] Enable motion monitoring via API
- [ ] Restart device
- [ ] Verify camera doesn't initialize on boot
- [ ] Trigger motion
- [ ] Verify camera initializes automatically
- [ ] Check serial output for initialization messages

### 4. Video Recording
- [ ] Insert SD card
- [ ] Enable motion monitoring
- [ ] Trigger motion
- [ ] Wait for camera to start
- [ ] Verify `/recordings` folder is created
- [ ] Check file is created: `motion_XXXXX.mjpeg`
- [ ] Let recording run for 5+ seconds without motion
- [ ] Verify recording stops
- [ ] Verify camera shuts down

### 5. Continuous Motion
- [ ] Start recording with motion
- [ ] Continue moving during recording
- [ ] Verify timer resets (recording continues)
- [ ] Stop motion
- [ ] Verify recording stops after 5 seconds
- [ ] Check file contains extended footage

### 6. Video Playback
- [ ] Remove SD card
- [ ] Copy MJPEG file to computer
- [ ] Open in VLC Media Player
- [ ] Verify video plays correctly
- [ ] Check frame rate (~5 FPS)
- [ ] Verify no corruption

### 7. API Testing
```bash
# Check status
curl http://ESP32-IP/status
# Should show: motion_monitoring_enabled, sd_card_mounted

# Check motion status
curl http://ESP32-IP/motion/status

# Enable motion monitoring
curl http://ESP32-IP/motion/enable

# Disable motion monitoring
curl http://ESP32-IP/motion/disable

# Restart to apply changes
curl http://ESP32-IP/restart
```

### 8. Configuration Persistence
- [ ] Enable motion monitoring via API
- [ ] Restart device
- [ ] Verify still enabled after restart
- [ ] Check `/config/config.json` on SD card
- [ ] Verify motion settings present

### 9. Power Consumption
- [ ] Measure current with motion monitoring DISABLED (camera always on)
- [ ] Measure current with motion monitoring ENABLED (camera off)
- [ ] Trigger motion and measure during recording
- [ ] Verify ~100mA savings in idle mode

### 10. Stress Testing
- [ ] Multiple recordings in sequence
- [ ] Verify no memory leaks (check `/status` free_heap)
- [ ] Run for extended period (hours)
- [ ] Verify stability
- [ ] Check SD card for all recordings

### 11. Edge Cases
- [ ] Motion during camera startup
- [ ] Rapid on/off motion
- [ ] SD card full
- [ ] SD card removed during recording
- [ ] Network disconnect during recording
- [ ] Power loss during recording

## Serial Output Validation

Expected output when motion detected:
```
Motion detected! Transitioning to recording mode
Initializing camera for recording
Camera initialized successfully
Video recording started: /recordings/motion_XXXXX.mjpeg
Frame written to video
...
Recording duration elapsed, stopping recording
Recording finalized successfully
Recording saved: /recordings/motion_XXXXX.mjpeg
Stopping camera to save power
Returning to IDLE state, waiting for next motion
```

## Performance Benchmarks

| Metric | Target | Actual |
|--------|--------|--------|
| Motion detection latency | < 500ms | TBD |
| Camera startup time | < 2s | TBD |
| Recording start latency | < 500ms | TBD |
| Frame rate during recording | ~5 FPS | TBD |
| File size per second | 50-150 KB/s | TBD |
| Memory usage (idle) | < 50KB | TBD |
| Memory usage (recording) | < 100KB | TBD |
| Power consumption (idle) | ~50mA | TBD |
| Power consumption (recording) | ~200mA | TBD |

## Known Issues

Document any issues found during testing:

1. 
2. 
3. 

## Test Results Summary

- Date: ___________
- Tester: ___________
- Hardware: ESP32-CAM AI-Thinker / WROVER-KIT
- Firmware Version: ___________

| Test Category | Pass | Fail | Notes |
|---------------|------|------|-------|
| Basic Connectivity | ☐ | ☐ | |
| Motion Detection | ☐ | ☐ | |
| Camera Integration | ☐ | ☐ | |
| Video Recording | ☐ | ☐ | |
| Continuous Motion | ☐ | ☐ | |
| Video Playback | ☐ | ☐ | |
| API Testing | ☐ | ☐ | |
| Configuration Persistence | ☐ | ☐ | |
| Power Consumption | ☐ | ☐ | |
| Stress Testing | ☐ | ☐ | |
| Edge Cases | ☐ | ☐ | |

**Overall Result**: ☐ PASS  ☐ FAIL

**Comments**:
_______________________________________________
_______________________________________________
_______________________________________________
