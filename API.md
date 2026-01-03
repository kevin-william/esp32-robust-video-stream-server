# ESP32-CAM API Documentation

## Base URL

All API endpoints are accessible at:
```
http://<ESP32-IP-ADDRESS>/
```

Replace `<ESP32-IP-ADDRESS>` with your ESP32's IP address (shown in serial monitor or `/status` response).

## Quick Reference

| Endpoint | Method | Description | Power Impact |
|----------|--------|-------------|--------------|
| `/status` | GET | System status and metrics | None |
| `/capture` | GET | Capture single JPEG image | None |
| `/stream` | GET | MJPEG video stream | None |
| `/control` | GET | Adjust camera parameters | None |
| `/reset` | GET | Hard reset with cleanup | System restart |
| `/stop` | GET | Stop camera, enter low-power | -50% power |
| `/start` | GET | Start/restart camera | +50% power |
| `/sleep` | GET | Alias for `/stop` | -50% power |
| `/wake` | GET | Alias for `/start` | +50% power |

## Status & Information

### GET /status

Get comprehensive system status including camera state, memory usage, WiFi info, and uptime.

**Request:**
```bash
curl http://192.168.1.100/status
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
  "known_networks": ["MyWiFi", "OfficeWiFi"]
}
```

**Fields:**
- `camera_initialized` (boolean): Whether camera driver is initialized
- `camera_sleeping` (boolean): Whether camera is in power-down mode
- `uptime` (number): System uptime in seconds
- `free_heap` (number): Free heap memory in bytes
- `min_free_heap` (number): Minimum free heap since boot
- `free_psram` (number): Free PSRAM in bytes (if available)
- `wifi_connected` (boolean): WiFi connection status
- `ip_address` (string): Current IP address
- `rssi` (number): WiFi signal strength in dBm
- `ap_mode` (boolean): Whether in AP mode (captive portal)
- `reset_reason` (string): Reason for last reset
- `known_networks` (array): List of saved WiFi networks

---

### GET /sleepstatus

Get camera sleep status and uptime (lightweight endpoint).

**Request:**
```bash
curl http://192.168.1.100/sleepstatus
```

**Response:**
```json
{
  "sleeping": false,
  "uptime": 3600
}
```

---

## Camera Control

### GET /capture

Capture a single high-resolution JPEG image.

**Request:**
```bash
curl http://192.168.1.100/capture -o photo.jpg
```

**Response:**
- **Success**: JPEG image data (Content-Type: image/jpeg)
- **Error**: JSON error message

**Response Headers:**
```
Content-Type: image/jpeg
Content-Disposition: inline; filename=capture.jpg
Access-Control-Allow-Origin: *
```

**Error Responses:**

503 Service Unavailable (Camera not available):
```json
{
  "error": "Camera is sleeping or not initialized"
}
```

500 Internal Server Error (Capture failed):
```json
{
  "error": "Failed to capture frame"
}
```

**Usage Example (wget):**
```bash
wget http://192.168.1.100/capture -O snapshot.jpg
```

---

### GET /stream

Start MJPEG video stream (multipart/x-mixed-replace).

**Request:**
```bash
# View in browser
http://192.168.1.100/stream

# Stream with ffplay
curl http://192.168.1.100/stream --output - | ffplay -

# Stream with VLC
vlc http://192.168.1.100/stream
```

**Response:**
- **Content-Type**: `multipart/x-mixed-replace; boundary=frame`
- **Streaming Protocol**: Continuous JPEG frames with HTTP multipart

**Stream Format:**
```
--frame
Content-Type: image/jpeg
Content-Length: 12345

<JPEG data>
--frame
Content-Type: image/jpeg
Content-Length: 12456

<JPEG data>
--frame
...
```

**Performance:**
- Frame Rate: ~15 FPS (configurable)
- Resolution: Depends on camera settings (default: VGA 640×480)
- Latency: ~100ms

**Error Responses:**

503 Service Unavailable:
```
Camera is sleeping or not initialized
```

**Notes:**
- Stream continues until client disconnects
- Uses zero-copy optimization with PSRAM
- Multiple clients supported (limited by WiFi bandwidth)

---

### GET /control

Adjust camera parameters in real-time.

**Parameters:**
- `var` (required): Parameter name
- `val` (required): Parameter value

**Supported Parameters:**

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `framesize` | 0-13 | 8 (VGA) | Resolution (see table below) |
| `quality` | 0-63 | 10 | JPEG quality (0=best, 63=worst) |
| `brightness` | -2 to 2 | 0 | Image brightness |
| `contrast` | -2 to 2 | 0 | Image contrast |
| `saturation` | -2 to 2 | 0 | Color saturation |
| `hmirror` | 0, 1 | 0 | Horizontal mirror |
| `vflip` | 0, 1 | 0 | Vertical flip |
| `led_intensity` | 0-255 | 0 | Flash LED brightness |

**Frame Size Values:**

| Value | Resolution | Name |
|-------|------------|------|
| 0 | 96×96 | QQVGA |
| 1 | 160×120 | QQVGA2 |
| 2 | 176×144 | QCIF |
| 3 | 240×176 | HQVGA |
| 4 | 240×240 | 240×240 |
| 5 | 320×240 | QVGA |
| 6 | 400×296 | CIF |
| 7 | 480×320 | HVGA |
| 8 | 640×480 | VGA |
| 9 | 800×600 | SVGA |
| 10 | 1024×768 | XGA |
| 11 | 1280×1024 | SXGA |
| 12 | 1600×1200 | UXGA |

**Request:**
```bash
# Set quality to 10 (good balance)
curl "http://192.168.1.100/control?var=quality&val=10"

# Enable horizontal mirror
curl "http://192.168.1.100/control?var=hmirror&val=1"

# Set resolution to VGA
curl "http://192.168.1.100/control?var=framesize&val=8"

# Turn on LED at 50% brightness
curl "http://192.168.1.100/control?var=led_intensity&val=128"
```

**Response:**

Success:
```json
{
  "success": true
}
```

Error:
```json
{
  "error": "Failed to set quality"
}
```

**Notes:**
- Changes take effect immediately
- Settings are saved to configuration
- Some settings may require camera restart

---

## Power Management

### GET /stop

Stop camera service and enter power-down mode. Reduces power consumption by ~50%.

**What it does:**
1. Deinitializes camera driver
2. Frees frame buffer memory
3. Sets PWDN pin HIGH (hardware power-down)
4. Enables WiFi power save mode
5. WiFi remains active for receiving commands

**Request:**
```bash
curl http://192.168.1.100/stop
```

**Response:**
```json
{
  "success": true,
  "message": "Camera service stopped and sensor in power-down mode. WiFi remains active."
}
```

**Power Consumption:**
- Before: ~250mA (streaming)
- After: ~100mA (WiFi only)
- Savings: ~60%

**Use Cases:**
- Battery operation (extend runtime)
- Scheduled recording (stop between captures)
- Thermal management (reduce heat)

**Notes:**
- Camera can be restarted with `/start`
- WiFi remains connected
- Web server continues serving requests

---

### GET /start

Start or restart camera service. Exits power-down mode.

**What it does:**
1. Disables WiFi power save mode
2. Sets PWDN pin LOW (wake camera)
3. If camera running: forces complete reinitialization
4. If camera stopped: initializes normally
5. Allocates 2 frame buffers in PSRAM

**Request:**
```bash
curl http://192.168.1.100/start
```

**Response:**

Success:
```json
{
  "success": true,
  "message": "Camera service started and ready for streaming"
}
```

Error:
```json
{
  "error": "Failed to start camera service",
  "message": "Check camera connections and power supply"
}
```

**When to use:**
- Resume after `/stop`
- Fix camera errors (forces reinit)
- Apply camera configuration changes

**Notes:**
- Takes ~2 seconds to initialize
- Clears any camera errors
- Resets all camera settings

---

### GET /sleep

Alias for `/stop`. Put camera to sleep.

**Request:**
```bash
curl http://192.168.1.100/sleep
```

**Response:**
```json
{
  "success": true,
  "message": "Camera sleeping"
}
```

---

### GET /wake

Alias for `/start`. Wake camera from sleep.

**Request:**
```bash
curl http://192.168.1.100/wake
```

**Response:**

Success:
```json
{
  "success": true,
  "message": "Camera awake"
}
```

Error:
```json
{
  "error": "Failed to wake camera"
}
```

---

## System Management

### GET /reset

Perform hard/brute reset with proper cleanup.

**What it does:**
1. Sends HTTP response
2. Deinitializes camera (frees resources)
3. Closes all active sockets
4. Waits for cleanup to complete
5. Calls `esp_restart()` (complete system reset)

**Request:**
```bash
curl http://192.168.1.100/reset
```

**Response:**
```json
{
  "success": true,
  "message": "Performing hard reset with cleanup..."
}
```

**Behavior:**
- Device reboots immediately after response
- Clean restart (no memory corruption)
- WiFi reconnects automatically
- Camera reinitializes on boot

**When to use:**
- Recover from errors
- Apply firmware updates
- Clear system state
- Fix memory leaks

**Notes:**
- Takes ~30 seconds to complete
- All connections will be dropped
- Configuration is preserved

---

### GET /restart

Alias for `/reset`. Restart the device.

**Request:**
```bash
curl http://192.168.1.100/restart
```

**Response:**
```json
{
  "success": true,
  "message": "Restarting..."
}
```

---

## WiFi Management

### POST /wifi-connect

Connect to a WiFi network (used during initial setup).

**Request Body:**
```json
{
  "ssid": "MyWiFi",
  "password": "mypassword",
  "use_static_ip": false
}
```

**With Static IP:**
```json
{
  "ssid": "MyWiFi",
  "password": "mypassword",
  "use_static_ip": true,
  "static_ip": "192.168.1.100",
  "gateway": "192.168.1.1"
}
```

**Request:**
```bash
curl -X POST http://192.168.4.1/wifi-connect \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MyWiFi","password":"mypassword"}'
```

**Response:**

Success:
```json
{
  "success": true,
  "ip": "192.168.1.100"
}
```

Error:
```json
{
  "success": false,
  "message": "Unable to connect. Check SSID, password, and signal strength."
}
```

---

## CORS Headers

All endpoints include CORS headers for cross-origin requests:

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization, X-CSRF-Token
```

---

## Error Codes

| Code | Meaning | Common Causes |
|------|---------|---------------|
| 200 | OK | Success |
| 400 | Bad Request | Missing/invalid parameters |
| 404 | Not Found | Invalid endpoint |
| 500 | Internal Server Error | Camera/system error |
| 503 | Service Unavailable | Camera not initialized |

---

## Rate Limiting

No rate limiting is currently implemented. Recommendations:
- `/stream`: One connection per client
- `/capture`: Max 1 request/second
- `/control`: Max 10 requests/second
- `/start`, `/stop`, `/reset`: Max 1 request/minute

---

## Best Practices

### Streaming
1. Use single stream connection per client
2. Close stream when not viewing
3. Use `/stop` when not streaming (saves power)
4. Monitor WiFi signal strength

### Capture
1. Wait for previous capture to complete
2. Use appropriate resolution for use case
3. Check response for errors

### Power Management
1. Always `/stop` when not using camera
2. Use `/reset` if camera becomes unresponsive
3. Monitor memory with `/status`

### Configuration
1. Test settings with `/control` first
2. Save successful settings
3. Use `/start` to apply major changes

---

## Examples

### Python Client

```python
import requests
import cv2
import numpy as np

# Status check
response = requests.get('http://192.168.1.100/status')
print(response.json())

# Capture single image
response = requests.get('http://192.168.1.100/capture')
if response.status_code == 200:
    with open('photo.jpg', 'wb') as f:
        f.write(response.content)

# Stream processing
stream = requests.get('http://192.168.1.100/stream', stream=True)
bytes_data = bytes()
for chunk in stream.iter_content(chunk_size=1024):
    bytes_data += chunk
    a = bytes_data.find(b'\xff\xd8')  # JPEG start
    b = bytes_data.find(b'\xff\xd9')  # JPEG end
    if a != -1 and b != -1:
        jpg = bytes_data[a:b+2]
        bytes_data = bytes_data[b+2:]
        img = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)
        cv2.imshow('ESP32-CAM', img)
        if cv2.waitKey(1) == 27:  # ESC
            break
```

### JavaScript Client

```javascript
// Status check
fetch('http://192.168.1.100/status')
  .then(response => response.json())
  .then(data => console.log(data));

// Capture image
fetch('http://192.168.1.100/capture')
  .then(response => response.blob())
  .then(blob => {
    const url = URL.createObjectURL(blob);
    document.getElementById('image').src = url;
  });

// Stream in IMG tag
document.getElementById('stream').src = 'http://192.168.1.100/stream';

// Control camera
function setQuality(value) {
  fetch(`http://192.168.1.100/control?var=quality&val=${value}`)
    .then(response => response.json())
    .then(data => console.log(data));
}
```

### cURL Examples

```bash
# Complete workflow
curl http://192.168.1.100/status
curl http://192.168.1.100/start
curl http://192.168.1.100/capture -o photo.jpg
curl "http://192.168.1.100/control?var=quality&val=10"
curl http://192.168.1.100/stop

# Power management cycle
curl http://192.168.1.100/stop    # Enter low-power
sleep 300                         # Wait 5 minutes
curl http://192.168.1.100/start   # Resume

# Recovery from error
curl http://192.168.1.100/reset
```

---

## Troubleshooting

### Camera Not Responding
```bash
# Force restart camera
curl http://192.168.1.100/start

# If that fails, hard reset
curl http://192.168.1.100/reset
```

### Low Frame Rate
```bash
# Reduce resolution
curl "http://192.168.1.100/control?var=framesize&val=5"  # QVGA

# Increase quality (lower quality, faster)
curl "http://192.168.1.100/control?var=quality&val=15"
```

### Memory Issues
```bash
# Check memory status
curl http://192.168.1.100/status | grep heap

# Stop camera to free memory
curl http://192.168.1.100/stop

# Hard reset if needed
curl http://192.168.1.100/reset
```
