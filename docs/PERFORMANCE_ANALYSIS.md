# ESP32-CAM Performance Analysis

## Hardware Capabilities

### ESP32 Specifications
- **CPU**: Dual-core Xtensa LX6, 240MHz (current config)
- **PSRAM**: 4MB available (1.13% usage - plenty free)
- **Heap**: 307KB (46% usage - healthy)
- **Camera**: OV2640, supports up to 1600x1200 @ JPEG

### Expected Performance (Based on Espressif Documentation)

#### Resolution vs FPS Benchmarks
| Resolution | Pixels | Expected FPS | Current FPS | Status |
|------------|--------|--------------|-------------|---------|
| QQVGA | 160x120 | 25+ FPS | - | Not tested |
| QVGA | 320x240 | 20-25 FPS | - | Not tested |
| **CIF** | **400x296** | **15-20 FPS** | **0.97 FPS** | **❌ FAILED** |
| VGA | 640x480 | 10-15 FPS | - | Not tested |
| SVGA | 800x600 | 8-12 FPS | - | Not tested |

**Current performance: 0.97 FPS at CIF (400x296) - 95% below expected!**

## Problem Analysis

### Current Issues
1. **Frame time: 1031ms** (should be ~66ms for 15 FPS)
2. **Last frame: 64 seconds ago** (stream completely stalled)
3. **AsyncWebServer beginChunkedResponse limitation** - callback only fires once
4. **Multiple canceled requests** in browser network tab

### Root Cause
The current MJPEG implementation using AsyncWebServer's `beginChunkedResponse` is **fundamentally broken**:
- Callback is called only ONCE, not continuously
- Each request triggers new state instead of continuous stream
- AsyncWebServer is NOT designed for continuous multipart streaming

## Proven Solution: Official Espressif Example

The official esp32-camera repository shows the correct approach using **httpd_resp_send_chunk** in a loop:

```cpp
esp_err_t jpg_stream_httpd_handler(httpd_req_t *req){
    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=123456789000000000000987654321");
    
    while(true){
        fb = esp_camera_fb_get();
        // Send boundary
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        // Send JPEG header
        res = httpd_resp_send_chunk(req, part_buf, hlen);
        // Send JPEG data
        res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_buf_len);
        esp_camera_fb_return(fb);
    }
}
```

**Key difference**: Uses native ESP-IDF HTTP server, NOT AsyncWebServer.

## Recommendation

### Option 1: Use Native ESP-IDF HTTP Server (BEST)
- Replace AsyncWebServer with esp_http_server (ESP-IDF native)
- Proven to work at 15-25 FPS for CIF resolution
- Example available in official Espressif repository
- **Effort**: High (major refactor)

### Option 2: Simple Polling with Optimized Settings (CURRENT)
- Keep AsyncWebServer for configuration pages
- Use simple JavaScript polling for stream display
- Optimize camera settings (reduce resolution/quality)
- **Target**: 5-10 FPS (achievable with optimizations)
- **Effort**: Low (configuration only)

### Option 3: Hybrid Approach
- AsyncWebServer for config endpoints
- Separate task with raw TCP socket for MJPEG stream
- **Effort**: Medium

## Immediate Actions

1. **Reduce JPEG quality** from 12 to 15-20 (less CPU processing)
2. **Lower resolution** to QVGA (320x240) temporarily
3. **Verify camera xclk_freq** is optimal (20MHz is correct)
4. **Test with simple single-frame requests** first before streaming

## Conclusion

**YES, ESP32-CAM CAN EASILY do 10+ FPS at CIF resolution.**

The problem is NOT the hardware - it's the AsyncWebServer library limitation. The official Espressif examples achieve 15-25 FPS using native ESP-IDF HTTP server with proper MJPEG multipart streaming.
