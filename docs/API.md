# API Reference

Complete REST API documentation for the ESP32-CAM Robust Video Stream Server.

## Base URL

```
http://<ESP32-IP-ADDRESS>
```

## Common Headers

All endpoints support CORS:

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization, X-CSRF-Token
```

## Status Endpoints

### GET /status

Get comprehensive system status.

**Request:**
```bash
curl http://192.168.1.100/status
```

**Response:** `200 OK`
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
- `camera_initialized` (boolean): Camera successfully initialized
- `camera_sleeping` (boolean): Camera in sleep mode
- `uptime` (integer): System uptime in seconds
- `free_heap` (integer): Free heap memory in bytes
- `min_free_heap` (integer): Minimum free heap since boot
- `free_psram` (integer): Free PSRAM (if available)
- `wifi_connected` (boolean): WiFi connection status
- `ip_address` (string): Current IP address
- `rssi` (integer): WiFi signal strength in dBm
- `ap_mode` (boolean): Access Point mode active
- `reset_reason` (string): Last reset reason
- `known_networks` (array): List of saved WiFi SSIDs

---

### GET /sleepstatus

Get camera sleep status (lightweight endpoint).

**Request:**
```bash
curl http://192.168.1.100/sleepstatus
```

**Response:** `200 OK`
```json
{
  "sleeping": false,
  "uptime": 3600
}
```

**Fields:**
- `sleeping` (boolean): Camera sleep state
- `uptime` (integer): System uptime in seconds

---

## Camera Endpoints

### GET /capture

Capture a single JPEG image.

**Request:**
```bash
curl http://192.168.1.100/capture -o photo.jpg
```

**Response:** `200 OK`
- Content-Type: `image/jpeg`
- Content-Disposition: `inline; filename=capture.jpg`
- Body: JPEG image data

**Error Responses:**
- `503 Service Unavailable`: Camera is sleeping or not initialized
  ```json
  {"error": "Camera is sleeping or not initialized"}
  ```
- `500 Internal Server Error`: Frame capture failed
  ```json
  {"error": "Failed to capture frame"}
  ```

---

### GET /stream

MJPEG video stream (multipart response).

**Request:**
```bash
# View in browser
http://192.168.1.100/stream

# Stream to file
curl http://192.168.1.100/stream -o stream.mjpeg

# Stream to video player
curl http://192.168.1.100/stream --output - | ffplay -
```

**Response:** `200 OK`
- Content-Type: `multipart/x-mixed-replace; boundary=frame`

**Format:**
```
--frame
Content-Type: image/jpeg
Content-Length: <size>

<JPEG data>
--frame
Content-Type: image/jpeg
Content-Length: <size>

<JPEG data>
...
```

**Error Responses:**
- `503 Service Unavailable`: Camera is sleeping
  ```json
  {"error": "Camera is sleeping"}
  ```

**Notes:**
- Stream continues until client disconnects
- Frame rate limited by camera and network bandwidth
- Typical FPS: 10-20 depending on resolution

---

### GET /bmp

Capture image in BMP format.

**Request:**
```bash
curl http://192.168.1.100/bmp -o photo.bmp
```

**Response:** `200 OK`
- Content-Type: `image/jpeg` (currently returns JPEG)

**Note:** BMP conversion is planned for future release. Currently returns JPEG format.

---

### GET /control

Adjust camera parameters.

**Parameters:**
- `var` (required): Parameter name
- `val` (required): Parameter value

**Request:**
```bash
# Set quality to 10
curl "http://192.168.1.100/control?var=quality&val=10"

# Set brightness to 1
curl "http://192.168.1.100/control?var=brightness&val=1"

# Enable horizontal mirror
curl "http://192.168.1.100/control?var=hmirror&val=1"

# Set LED intensity to 128
curl "http://192.168.1.100/control?var=led_intensity&val=128"
```

**Response:** `200 OK`
```json
{"success": true}
```

**Error Response:** `400 Bad Request`
```json
{"error": "Missing parameters"}
```

**Error Response:** `500 Internal Server Error`
```json
{"error": "Failed to set quality"}
```

**Supported Parameters:**

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `framesize` | int | 0-13 | Resolution (see framesize_t) |
| `quality` | int | 0-63 | JPEG quality (lower = better) |
| `brightness` | int | -2 to 2 | Brightness adjustment |
| `contrast` | int | -2 to 2 | Contrast adjustment |
| `saturation` | int | -2 to 2 | Saturation adjustment |
| `hmirror` | int | 0 or 1 | Horizontal mirror |
| `vflip` | int | 0 or 1 | Vertical flip |
| `led_intensity` | int | 0-255 | Flash LED brightness |

**Framesize Values:**
- 0: QQVGA (160x120)
- 1: QCIF (176x144)
- 2: HQVGA (240x176)
- 3: QVGA (320x240)
- 4: CIF (400x296)
- 5: VGA (640x480)
- 6: SVGA (800x600)
- 7: XGA (1024x768)
- 8: SXGA (1280x1024)
- 9: UXGA (1600x1200)

---

### GET /sleep

Put camera to sleep (deinitialize, free memory).

**Request:**
```bash
curl http://192.168.1.100/sleep
```

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Camera sleeping"
}
```

**Effects:**
- Calls `esp_camera_deinit()`
- Frees camera buffers
- Camera unavailable until wake
- `/capture` and `/stream` return 503

---

### GET /wake

Wake camera from sleep (reinitialize).

**Request:**
```bash
curl http://192.168.1.100/wake
```

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Camera awake"
}
```

**Error Response:** `500 Internal Server Error`
```json
{"error": "Failed to wake camera"}
```

**Effects:**
- Reinitializes camera
- Restores previous settings
- Camera ready for capture/stream

---

## WiFi Management

### GET /wifi-scan

Scan for available WiFi networks.

**Request:**
```bash
curl http://192.168.1.100/wifi-scan
```

**Response:** `200 OK`
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
  },
  {
    "ssid": "OpenNetwork",
    "rssi": -68,
    "encryption": 0
  }
]
```

**Fields:**
- `ssid` (string): Network name
- `rssi` (integer): Signal strength in dBm (higher is better)
- `encryption` (integer): Encryption type
  - 0: Open
  - 1: WEP
  - 2: WPA
  - 3: WPA2
  - 4: WPA/WPA2
  - 5: WPA2 Enterprise

**Notes:**
- Scan takes 2-5 seconds
- Returns networks in order found
- Duplicate SSIDs possible (multiple APs)

---

### POST /wifi-connect

Connect to a WiFi network.

**Request:**
```bash
curl -X POST http://192.168.1.100/wifi-connect \
  -H "Content-Type: application/json" \
  -d '{
    "ssid": "MyWiFi",
    "password": "mypassword"
  }'
```

**Request Body:**
```json
{
  "ssid": "MyWiFi",
  "password": "mypassword"
}
```

**Response:** `200 OK`
```json
{
  "success": true,
  "ip": "192.168.1.100"
}
```

**Error Response:** `400 Bad Request`
```json
{"error": "Missing ssid or password"}
```

**Error Response:** `500 Internal Server Error`
```json
{
  "success": false,
  "message": "Connection failed"
}
```

**Effects:**
- Attempts WiFi connection (20 second timeout)
- If successful, saves credentials to config
- Stops captive portal if active
- Device obtains new IP via DHCP

---

## System Management

### GET /restart

Restart the ESP32 device.

**Request:**
```bash
curl http://192.168.1.100/restart
```

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Restarting..."
}
```

**Effects:**
- Queues restart event
- Device reboots after 2 seconds
- Connection lost during restart
- Device reconnects to WiFi after boot

---

## Configuration Management

### POST /config

Update system configuration (planned for future release).

**Request:**
```bash
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{
    "camera": {
      "quality": 10,
      "brightness": 1
    }
  }'
```

**Note:** Requires authentication. Implementation pending.

---

## OTA Updates

### POST /ota

Trigger OTA firmware update (planned for future release).

**Request:**
```bash
curl -X POST http://192.168.1.100/ota \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://example.com/firmware.bin"
  }'
```

**Note:** Implementation pending.

---

## Error Codes

| Code | Meaning | Common Causes |
|------|---------|---------------|
| 200 | OK | Success |
| 400 | Bad Request | Missing parameters, invalid JSON |
| 401 | Unauthorized | Authentication required |
| 404 | Not Found | Invalid endpoint |
| 500 | Internal Server Error | Camera error, system failure |
| 503 | Service Unavailable | Camera sleeping, not initialized |

---

## Rate Limiting

Currently no rate limiting is implemented. For production:
- Recommend max 10 requests/second per client
- Stream endpoint: 1 concurrent stream per client recommended
- Excessive requests may cause memory exhaustion

---

## Authentication

Currently authentication is minimal. For production use:

1. Set admin password in config:
   ```json
   {
     "admin_password_hash": "<SHA256 hash of password>"
   }
   ```

2. Include in requests:
   ```bash
   curl -H "Authorization: Bearer <token>" http://...
   ```

**Note:** Full JWT/OAuth implementation recommended for production.

---

## Example Usage

### Complete Workflow

```bash
# 1. Check status
curl http://192.168.1.100/status

# 2. Adjust camera settings
curl "http://192.168.1.100/control?var=quality&val=10"
curl "http://192.168.1.100/control?var=brightness&val=1"

# 3. Capture photo
curl http://192.168.1.100/capture -o photo.jpg

# 4. Start stream (in browser or video player)
# Browser: http://192.168.1.100/stream
# VLC: vlc http://192.168.1.100/stream

# 5. Sleep camera to save power
curl http://192.168.1.100/sleep

# 6. Wake camera when needed
curl http://192.168.1.100/wake

# 7. Scan for WiFi networks
curl http://192.168.1.100/wifi-scan

# 8. Restart device
curl http://192.168.1.100/restart
```

### Web Integration

```html
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Viewer</title>
</head>
<body>
    <h1>ESP32-CAM Live Stream</h1>
    <img id="stream" src="http://192.168.1.100/stream" width="800">
    <br>
    <button onclick="capture()">Capture Photo</button>
    <button onclick="sleep()">Sleep Camera</button>
    <button onclick="wake()">Wake Camera</button>
    
    <div id="photo"></div>
    
    <script>
        async function capture() {
            const response = await fetch('http://192.168.1.100/capture');
            const blob = await response.blob();
            const url = URL.createObjectURL(blob);
            document.getElementById('photo').innerHTML = 
                `<img src="${url}" width="400">`;
        }
        
        async function sleep() {
            const response = await fetch('http://192.168.1.100/sleep');
            const data = await response.json();
            alert(data.message);
        }
        
        async function wake() {
            const response = await fetch('http://192.168.1.100/wake');
            const data = await response.json();
            alert(data.message);
        }
    </script>
</body>
</html>
```

---

## OpenAPI Specification

For OpenAPI/Swagger documentation, see `docs/openapi.yaml` (planned for M5).

---

## Support

For API questions or issues:
- Check this documentation
- Review examples in `/docs`
- Open GitHub issue with request/response details
