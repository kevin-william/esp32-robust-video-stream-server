#include "web_server.h"
#include "app.h"
#include "config.h"
#include "captive_portal.h"
#include "diagnostics.h"
#include "ota_update.h"
#include <ArduinoJson.h>
#include <esp_camera.h>
#include <mbedtls/sha256.h>

httpd_handle_t server = NULL;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Helper to add CORS headers
esp_err_t addCORSHeaders(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return ESP_OK;
}

esp_err_t handleRoot(httpd_req_t *req) {
    String html = "<!DOCTYPE html><html><head><title>ESP32-CAM</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
    html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;border-bottom:2px solid #007bff;padding-bottom:10px}";
    html += "button{background:#007bff;color:white;border:none;padding:10px 20px;border-radius:5px;cursor:pointer;margin:5px}";
    html += "button:hover{background:#0056b3}";
    html += "input,select{width:100%;padding:8px;margin:5px 0;border:1px solid #ddd;border-radius:4px}";
    html += ".status{background:#e7f3ff;padding:10px;border-radius:5px;margin:10px 0}";
    html += "img{max-width:100%;border:1px solid #ddd;margin:10px 0}</style></head><body>";
    html += "<div class='container'><h1>ESP32-CAM Control Panel</h1>";
    
    if (ap_mode_active) {
        html += "<div class='status'>Configuration Mode - Connect your WiFi network below</div>";
        html += "<h2>WiFi Setup</h2>";
        html += "<p><strong>Note:</strong> The device will remain in this mode until successfully connected.</p>";
        html += "<h3>WiFi Configuration</h3>";
        html += "<div style='background:#f8f9fa;padding:15px;border-radius:5px'>";
        html += "<input type='text' id='ssid' placeholder='WiFi SSID *' required>";
        html += "<input type='password' id='password' placeholder='WiFi Password *' required>";
        html += "<p style='margin:10px 0;color:#666'><strong>Optional:</strong> Static IP Configuration (leave blank for DHCP)</p>";
        html += "<input type='text' id='static_ip' placeholder='Static IP (e.g., 192.168.1.100)'>";
        html += "<input type='text' id='gateway' placeholder='Gateway (e.g., 192.168.1.1)'>";
        html += "<button onclick='connectWiFi()' style='margin-top:10px;width:100%;padding:12px;font-size:16px'>Connect to WiFi</button>";
        html += "<div id='status-message' style='margin-top:10px'></div>";
        html += "</div>";
    } else {
        html += "<div class='status'>Connected - IP: " + WiFi.localIP().toString() + "</div>";
        html += "<h2>Camera Controls</h2>";
        html += "<button onclick='capture()'>Capture Photo</button>";
        html += "<button onclick='location.href=\"/stream\"'>View Stream</button>";
        html += "<button onclick='sleep()'>Sleep Camera</button>";
        html += "<button onclick='wake()'>Wake Camera</button>";
        html += "<div id='image'></div>";
        html += "<h2>System</h2>";
        html += "<button onclick='location.href=\"/update\"' style='background:#4caf50'>🔄 OTA Update</button>";
        html += "<button onclick='restart()' style='background:#ff9800'>Restart Device</button>";
        html += "<button onclick='factoryReset()' style='background:#f44336'>Factory Reset</button>";
        html += "<p style='color:#666;font-size:12px'>Factory Reset will erase all WiFi networks and return to setup mode</p>";
        html += "<h2>Settings</h2>";
        html += "<label>Quality (0-63): <input type='range' id='quality' min='0' max='63' value='" + String(g_config.camera.quality) + "' onchange='setControl(\"quality\",this.value)'></label>";
        html += "<label>Brightness: <input type='range' id='brightness' min='-2' max='2' value='" + String(g_config.camera.brightness) + "' onchange='setControl(\"brightness\",this.value)'></label>";
        html += "<label>LED Intensity: <input type='range' id='led' min='0' max='255' value='" + String(g_config.camera.led_intensity) + "' onchange='setControl(\"led_intensity\",this.value)'></label>";
    }
    
    html += "</div><script>";
    html += "function connectWiFi(){const s=document.getElementById('ssid').value;const p=document.getElementById('password').value;if(!s||!p){alert('SSID and Password are required!');return;}const msg=document.getElementById('status-message');msg.innerHTML='<p style=\"color:#007bff\">Connecting to WiFi...</p>';const data={ssid:s,password:p};const sip=document.getElementById('static_ip').value;if(sip){const gw=document.getElementById('gateway').value;if(gw){data.use_static_ip=true;data.static_ip=sip;data.gateway=gw;}}fetch('/wifi-connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)}).then(r=>r.json()).then(d=>{msg.innerHTML='<p style=\"color:'+(d.success?'green':'red')+';font-size:16px\">'+d.message+'</p>';}).catch(e=>{msg.innerHTML='<p style=\"color:red\">Request failed. Check credentials and try again.</p>'})}";
    html += "function capture(){fetch('/capture').then(r=>r.blob()).then(b=>{const url=URL.createObjectURL(b);document.getElementById('image').innerHTML='<img src=\"'+url+'\">';});}";
    html += "function setControl(v,val){fetch('/control?var='+v+'&val='+val).then(r=>r.json()).then(d=>console.log(d));}";
    html += "function sleep(){fetch('/sleep').then(r=>r.json()).then(d=>alert(d.message));}";
    html += "function wake(){fetch('/wake').then(r=>r.json()).then(d=>alert(d.message));}";
    html += "function restart(){if(confirm('Restart device?')){fetch('/restart').then(r=>r.json()).then(d=>{alert('Device restarting... Please wait 30 seconds.');setTimeout(()=>location.reload(),30000);});}}";
    html += "function factoryReset(){if(confirm('WARNING: This will erase ALL WiFi configurations and return to setup mode.\\n\\nAre you sure?')){if(confirm('This action cannot be undone. Continue?')){fetch('/factory-reset').then(r=>r.json()).then(d=>{alert(d.message);setTimeout(()=>location.href='http://192.168.4.1',5000);}).catch(e=>alert('Reset initiated'));}}}";
    html += "</script></body></html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}

esp_err_t handleStatus(httpd_req_t *req) {
    StaticJsonDocument<1024> doc;
    
    doc["camera_initialized"] = camera_initialized;
    doc["camera_sleeping"] = camera_sleeping;
    doc["uptime"] = getUptimeSeconds();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["min_free_heap"] = ESP.getMinFreeHeap();
    
    if (psramFound()) {
        doc["free_psram"] = ESP.getFreePsram();
    }
    
    doc["wifi_connected"] = wifi_connected;
    if (wifi_connected) {
        doc["ip_address"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
    }
    
    doc["ap_mode"] = ap_mode_active;
    doc["reset_reason"] = getResetReason();
    
    JsonArray networks = doc.createNestedArray("known_networks");
    for (int i = 0; i < g_config.network_count; i++) {
        networks.add(g_config.networks[i].ssid);
    }
    
    String output;
    serializeJson(doc, output);
    
    httpd_resp_set_type(req, "application/json");
    addCORSHeaders(req);
    httpd_resp_send(req, output.c_str(), output.length());
    return ESP_OK;
}

esp_err_t handleDiagnostics(httpd_req_t *req) {
    String json = getDiagnosticsJSON();
    httpd_resp_set_type(req, "application/json");
    addCORSHeaders(req);
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

esp_err_t handleSleepStatus(httpd_req_t *req) {
    StaticJsonDocument<200> doc;
    doc["sleeping"] = camera_sleeping;
    doc["uptime"] = getUptimeSeconds();
    
    String output;
    serializeJson(doc, output);
    
    httpd_resp_set_type(req, "application/json");
    addCORSHeaders(req);
    httpd_resp_send(req, output.c_str(), output.length());
    return ESP_OK;
}

esp_err_t handleCapture(httpd_req_t *req) {
    if (!camera_initialized || camera_sleeping) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Camera is sleeping or not initialized\"}", -1);
        return ESP_FAIL;
    }
    
    camera_fb_t *fb = captureFrame();
    if (!fb) {
        g_diag.frame_errors++;
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Failed to capture frame\"}", -1);
        return ESP_FAIL;
    }
    
    // Update frame stats
    g_diag.frame_count++;
    g_diag.total_frames_sent++;
    g_diag.total_bytes_sent += fb->len;
    g_diag.last_frame_time = millis();
    updateFrameStats();
    
    httpd_resp_set_type(req, "image/jpeg");
    addCORSHeaders(req);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    
    esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    releaseFrame(fb);
    
    return res;
}

esp_err_t handleStream(httpd_req_t *req) {
    if (!camera_initialized || camera_sleeping) {
        httpd_resp_send(req, "Camera not ready", -1);
        return ESP_FAIL;
    }
    
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t jpg_buf_len = 0;
    uint8_t *jpg_buf = NULL;
    char part_buf[64];
    
    // Set streaming content type
    httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    addCORSHeaders(req);
    
    // Add optimized headers for streaming
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    
    Serial.println("🎥 MJPEG Stream started");
    
    unsigned long last_frame_time = 0;
    unsigned long frame_count = 0;
    unsigned long total_frame_time = 0;
    int consecutive_errors = 0;
    const int max_consecutive_errors = 5;
    
    // Adaptive frame rate based on WiFi signal strength
    int target_delay_ms = 30;  // Default ~30 FPS target
    int rssi = WiFi.RSSI();
    
    if (rssi > -60) {
        target_delay_ms = 10;  // Strong signal: aim for high FPS (~100ms = 10 FPS effective)
    } else if (rssi > -70) {
        target_delay_ms = 20;  // Good signal: aim for medium FPS  
    } else if (rssi > -80) {
        target_delay_ms = 30;  // Fair signal: conservative FPS
    } else {
        target_delay_ms = 50;  // Weak signal: reduce frame rate
    }
    
    Serial.printf("Streaming with adaptive delay: %dms (RSSI: %d dBm)\n", target_delay_ms, rssi);
    
    while(true) {
        unsigned long frame_start = millis();
        
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            g_diag.frame_errors++;
            consecutive_errors++;
            
            if (consecutive_errors >= max_consecutive_errors) {
                Serial.printf("Too many consecutive errors (%d), terminating stream\n", consecutive_errors);
                res = ESP_FAIL;
                break;
            }
            
            delay(100);  // Brief delay before retry
            continue;
        }
        
        consecutive_errors = 0;  // Reset error counter on success
        jpg_buf_len = fb->len;
        jpg_buf = fb->buf;
        
        // Send boundary
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        
        // Send JPEG header
        if(res == ESP_OK){
            size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, jpg_buf_len);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        
        // Send JPEG data
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_buf_len);
        }
        
        esp_camera_fb_return(fb);
        
        if(res != ESP_OK){
            Serial.println("Stream connection closed");
            break;
        }
        
        // Update stats
        frame_count++;
        g_diag.frame_count++;
        g_diag.total_frames_sent++;
        g_diag.total_bytes_sent += jpg_buf_len;
        g_diag.last_frame_time = millis();
        updateFrameStats();
        
        // Calculate actual frame time
        unsigned long frame_time = millis() - frame_start;
        total_frame_time += frame_time;
        
        // Log performance every 100 frames
        if (frame_count % 100 == 0) {
            float avg_frame_time = total_frame_time / (float)frame_count;
            float avg_fps = 1000.0 / avg_frame_time;
            Serial.printf("Stream stats: %lu frames, avg %.1fms/frame (%.1f FPS)\n", 
                         frame_count, avg_frame_time, avg_fps);
        }
        
        // Adaptive delay to control frame rate
        // Only delay if frame processing was faster than target
        if (frame_time < target_delay_ms) {
            delay(target_delay_ms - frame_time);
        }
        // If frame took longer than target, send immediately (no delay)
    }
    
    Serial.printf("Stream ended after %lu frames\n", frame_count);
    return res;
}

esp_err_t handleBMP(httpd_req_t *req) {
    // BMP conversion is complex - for now return JPEG
    return handleCapture(req);
}

esp_err_t handleControl(httpd_req_t *req) {
    char buf[100];
    size_t buf_len;
    
    // Get URL query string
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char var[32] = {0};
            char val[32] = {0};
            
            if (httpd_query_key_value(buf, "var", var, sizeof(var)) == ESP_OK &&
                httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
                
                sensor_t *s = esp_camera_sensor_get();
                if (!s) {
                    httpd_resp_set_type(req, "application/json");
                    httpd_resp_send(req, "{\"error\":\"Camera not available\"}", -1);
                    return ESP_FAIL;
                }
                
                int res = 0;
                String varStr = String(var);
                int value = atoi(val);
                
                if (varStr == "framesize") {
                    res = s->set_framesize(s, (framesize_t)value);
                    g_config.camera.framesize = value;
                } else if (varStr == "quality") {
                    res = s->set_quality(s, value);
                    g_config.camera.quality = value;
                } else if (varStr == "brightness") {
                    res = s->set_brightness(s, value);
                    g_config.camera.brightness = value;
                } else if (varStr == "contrast") {
                    res = s->set_contrast(s, value);
                    g_config.camera.contrast = value;
                } else if (varStr == "saturation") {
                    res = s->set_saturation(s, value);
                    g_config.camera.saturation = value;
                } else if (varStr == "hmirror") {
                    res = s->set_hmirror(s, value);
                    g_config.camera.hmirror = value;
                } else if (varStr == "vflip") {
                    res = s->set_vflip(s, value);
                    g_config.camera.vflip = value;
                } else if (varStr == "led_intensity") {
                    setLED(value);
                    g_config.camera.led_intensity = value;
                }
                
                httpd_resp_set_type(req, "application/json");
                const char* response = res == 0 ? "{\"success\":true}" : "{\"error\":\"Failed to set parameter\"}";
                httpd_resp_send(req, response, -1);
                return res == 0 ? ESP_OK : ESP_FAIL;
            }
        }
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"error\":\"Missing parameters\"}", -1);
    return ESP_FAIL;
}

esp_err_t handleSleep(httpd_req_t *req) {
    deinitCamera();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true,\"message\":\"Camera sleeping\"}", -1);
    return ESP_OK;
}

esp_err_t handleWake(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    if (reinitCamera()) {
        httpd_resp_send(req, "{\"success\":true,\"message\":\"Camera awake\"}", -1);
        return ESP_OK;
    } else {
        httpd_resp_send(req, "{\"success\":false,\"message\":\"Failed to wake camera\"}", -1);
        return ESP_OK;  // Return ESP_OK to send the response, not ESP_FAIL
    }
}

esp_err_t handleRestart(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true,\"message\":\"Restarting...\"}", -1);
    
    // Give time for response to be sent before restarting
    delay(100);
    
    Event event;
    event.type = EVENT_RESTART_REQUESTED;
    // Use portMAX_DELAY to ensure event is sent
    if (xQueueSend(eventQueue, &event, portMAX_DELAY) != pdTRUE) {
        Serial.println("Failed to queue restart event, restarting directly");
        ESP.restart();  // Fallback: restart directly if queue fails
    }
    return ESP_OK;
}

esp_err_t handleFactoryReset(httpd_req_t *req) {
    Serial.println("========================================");
    Serial.println("Factory Reset Requested");
    Serial.println("  Clearing all configurations...");
    
    resetConfiguration();
    
    Serial.println("  Configuration cleared successfully");
    Serial.println("  Device will restart in captive portal mode");
    Serial.println("========================================");
    
    httpd_resp_set_type(req, "application/json");
    addCORSHeaders(req);
    httpd_resp_send(req, "{\"success\":true,\"message\":\"Configuration reset. Device restarting in 3 seconds...\"}", -1);
    
    Event event;
    event.type = EVENT_RESTART_REQUESTED;
    xQueueSend(eventQueue, &event, 0);
    return ESP_OK;
}

esp_err_t handleWiFiConnect(httpd_req_t *req) {
    char content[512];
    size_t recv_size = min((size_t)req->content_len, sizeof(content));
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    
    content[ret] = 0; // Null terminate
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, content);
    
    if (error) {
        httpd_resp_set_type(req, "application/json");
        addCORSHeaders(req);
        httpd_resp_send(req, "{\"success\":false,\"message\":\"Invalid JSON\"}", -1);
        return ESP_FAIL;
    }
    
    const char* ssid = doc["ssid"];
    const char* password = doc["password"];
    
    if (!ssid || !password || strlen(ssid) == 0 || strlen(password) == 0) {
        httpd_resp_set_type(req, "application/json");
        addCORSHeaders(req);
        httpd_resp_send(req, "{\"success\":false,\"message\":\"SSID and password are required\"}", -1);
        return ESP_FAIL;
    }
    
    bool use_static_ip = doc["use_static_ip"] | false;
    bool connected = false;
    
    Serial.println("========================================");
    Serial.println("WiFi Connection Request");
    Serial.printf("  SSID: %s\n", ssid);
    Serial.printf("  Static IP: %s\n", use_static_ip ? "Yes" : "No (DHCP)");
    
    if (use_static_ip && doc.containsKey("static_ip")) {
        IPAddress ip, gateway;
        
        if (ip.fromString(doc["static_ip"].as<String>()) &&
            gateway.fromString(doc["gateway"].as<String>())) {
            
            Serial.printf("  IP: %s\n", ip.toString().c_str());
            Serial.printf("  Gateway: %s\n", gateway.toString().c_str());
            
            connected = connectToWiFiWithStaticIP(ssid, password, ip, gateway, 20000);
        } else {
            httpd_resp_set_type(req, "application/json");
            addCORSHeaders(req);
            httpd_resp_send(req, "{\"success\":false,\"message\":\"Invalid IP address format\"}", -1);
            return ESP_FAIL;
        }
    } else {
        connected = connectToWiFi(ssid, password, 20000);
    }
    
    if (connected) {
        Serial.println("========================================");
        
        // Save to config
        if (g_config.network_count < MAX_WIFI_NETWORKS) {
            WiFiNetwork &net = g_config.networks[g_config.network_count];
            strncpy(net.ssid, ssid, 31);
            strncpy(net.password, password, 63);
            net.priority = g_config.network_count;
            net.use_static_ip = use_static_ip;
            
            if (use_static_ip) {
                IPAddress ip, gateway;
                ip.fromString(doc["static_ip"].as<String>());
                gateway.fromString(doc["gateway"].as<String>());
                
                for (int i = 0; i < 4; i++) {
                    net.static_ip[i] = ip[i];
                    net.gateway[i] = gateway[i];
                }
            }
            
            g_config.network_count++;
            saveConfiguration();
        }
        
        String response = "{\"success\":true,\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        httpd_resp_set_type(req, "application/json");
        addCORSHeaders(req);
        httpd_resp_send(req, response.c_str(), response.length());
        return ESP_OK;
    } else {
        Serial.println("========================================");
        httpd_resp_set_type(req, "application/json");
        addCORSHeaders(req);
        httpd_resp_send(req, "{\"success\":false,\"message\":\"Failed to connect to WiFi\"}", -1);
        return ESP_FAIL;
    }
}

void initWebServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 16;
    config.max_open_sockets = 7;
    config.stack_size = 8192;
    config.task_priority = 5;
    
    Serial.println("Starting HTTP Server...");
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handlers
        httpd_uri_t root_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = handleRoot,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        
        httpd_uri_t status_uri = {
            .uri       = "/status",
            .method    = HTTP_GET,
            .handler   = handleStatus,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &status_uri);
        
        httpd_uri_t diag_uri = {
            .uri       = "/diagnostics",
            .method    = HTTP_GET,
            .handler   = handleDiagnostics,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &diag_uri);
        
        httpd_uri_t sleep_status_uri = {
            .uri       = "/sleepstatus",
            .method    = HTTP_GET,
            .handler   = handleSleepStatus,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &sleep_status_uri);
        
        httpd_uri_t capture_uri = {
            .uri       = "/capture",
            .method    = HTTP_GET,
            .handler   = handleCapture,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &capture_uri);
        
        httpd_uri_t stream_uri = {
            .uri       = "/stream",
            .method    = HTTP_GET,
            .handler   = handleStream,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &stream_uri);
        
        httpd_uri_t bmp_uri = {
            .uri       = "/bmp",
            .method    = HTTP_GET,
            .handler   = handleBMP,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &bmp_uri);
        
        httpd_uri_t control_uri = {
            .uri       = "/control",
            .method    = HTTP_GET,
            .handler   = handleControl,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &control_uri);
        
        httpd_uri_t sleep_uri = {
            .uri       = "/sleep",
            .method    = HTTP_GET,
            .handler   = handleSleep,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &sleep_uri);
        
        httpd_uri_t wake_uri = {
            .uri       = "/wake",
            .method    = HTTP_GET,
            .handler   = handleWake,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &wake_uri);
        
        httpd_uri_t restart_uri = {
            .uri       = "/restart",
            .method    = HTTP_GET,
            .handler   = handleRestart,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &restart_uri);
        
        httpd_uri_t factory_reset_uri = {
            .uri       = "/factory-reset",
            .method    = HTTP_GET,
            .handler   = handleFactoryReset,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &factory_reset_uri);
        
        httpd_uri_t wifi_connect_uri = {
            .uri       = "/wifi-connect",
            .method    = HTTP_POST,
            .handler   = handleWiFiConnect,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &wifi_connect_uri);
        
        // Register OTA endpoints
        registerOTAEndpoints(server);
        
        Serial.println("✅ HTTP Server started successfully");
        Serial.println("   Registered endpoints:");
        Serial.println("   - / (root)");
        Serial.println("   - /status");
        Serial.println("   - /diagnostics");
        Serial.println("   - /capture");
        Serial.println("   - /stream (MJPEG multipart)");
        Serial.println("   - /control");
        Serial.println("   - /sleep, /wake");
        Serial.println("   - /restart, /factory-reset");
        Serial.println("   - /wifi-connect (POST)");
        Serial.println("   - /update (OTA firmware update)");
        Serial.println("   - /update/upload (POST)");
    } else {
        Serial.println("❌ Failed to start HTTP Server");
    }
}

void stopWebServer() {
    if (server) {
        httpd_stop(server);
        server = NULL;
        Serial.println("HTTP Server stopped");
    }
}
