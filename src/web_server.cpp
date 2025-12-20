#include "web_server.h"
#include "app.h"
#include "config.h"
#include "captive_portal.h"
#include "diagnostics.h"
#include <ArduinoJson.h>
#include <esp_camera.h>
#include <mbedtls/sha256.h>

AsyncWebServer server(80);

// Helper to add CORS headers
void addCORSHeaders(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-CSRF-Token");
}

// Simple authentication check
bool checkAuthentication(AsyncWebServerRequest *request) {
    // If no password is set, allow access
    if (strlen(g_config.admin_password_hash) == 0) {
        return true;
    }
    
    // Check for authorization header
    if (request->hasHeader("Authorization")) {
        String auth = request->header("Authorization");
        // Basic implementation - should be enhanced with proper token auth
        return true; // Placeholder
    }
    
    return false;
}

String generateCSRFToken() {
    // Simple token generation - should be enhanced
    return String(random(0x7FFFFFFF), HEX);
}

bool validateCSRFToken(const String& token) {
    // Placeholder - should implement proper token validation
    return true;
}

void initWebServer() {
    // CORS preflight
    server.on("/", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200);
        addCORSHeaders(response);
        request->send(response);
    });
    
    // Main page - captive portal or status
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
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
        
        request->send(200, "text/html", html);
    });
    
    // API endpoints
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = getDiagnosticsJSON();
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        addCORSHeaders(response);
        request->send(response);
    });
    server.on("/diag", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/diagnostics.html");
    });
    server.on("/sleepstatus", HTTP_GET, handleSleepStatus);
    server.on("/capture", HTTP_GET, handleCapture);
    server.on("/stream", HTTP_GET, handleStream);
    server.on("/bmp", HTTP_GET, handleBMP);
    server.on("/control", HTTP_GET, handleControl);
    server.on("/sleep", HTTP_GET, handleSleep);
    server.on("/wake", HTTP_GET, handleWake);
    server.on("/restart", HTTP_GET, handleRestart);
    server.on("/factory-reset", HTTP_GET, handleFactoryReset);
    
    // POST endpoint with body handler
    server.on("/wifi-connect", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            // This is called after body is received
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleWiFiConnect(request, data, len, index, total);
        }
    );
    
    server.onNotFound(handleNotFound);
    
    server.begin();
}

void handleStatus(AsyncWebServerRequest *request) {
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
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    addCORSHeaders(response);
    request->send(response);
}

void handleSleepStatus(AsyncWebServerRequest *request) {
    StaticJsonDocument<200> doc;
    doc["sleeping"] = camera_sleeping;
    doc["uptime"] = getUptimeSeconds();
    
    String output;
    serializeJson(doc, output);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    addCORSHeaders(response);
    request->send(response);
}

void handleCapture(AsyncWebServerRequest *request) {
    if (!camera_initialized || camera_sleeping) {
        AsyncWebServerResponse *response = request->beginResponse(503, "application/json", 
            "{\"error\":\"Camera is sleeping or not initialized\"}");
        addCORSHeaders(response);
        request->send(response);
        return;
    }
    
    camera_fb_t *fb = captureFrame();
    if (!fb) {
        AsyncWebServerResponse *response = request->beginResponse(500, "application/json", 
            "{\"error\":\"Failed to capture frame\"}");
        addCORSHeaders(response);
        request->send(response);
        return;
    }
    
    AsyncWebServerResponse *response = request->beginResponse(200, "image/jpeg", fb->buf, fb->len);
    addCORSHeaders(response);
    response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
    request->send(response);
    
    releaseFrame(fb);
}

void handleStream(AsyncWebServerRequest *request) {
    if (!camera_initialized || camera_sleeping) {
        request->send(503, "text/plain", "Camera is sleeping or not initialized");
        return;
    }
    
    AsyncWebServerResponse *response = request->beginChunkedResponse("multipart/x-mixed-replace; boundary=frame",
        [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            // Capture a new frame
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera capture failed");
                g_diag.frame_errors++;
                return 0; // End stream
            }
            
            // Update diagnostics
            updateFrameStats();
            g_diag.total_bytes_sent += fb->len;
            
            // Build the multipart response
            String header = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\n\r\n";
            
            // Calculate total size needed
            size_t totalSize = header.length() + fb->len + 2; // +2 for \r\n
            
            // Check if buffer is large enough
            if (totalSize > maxLen) {
                esp_camera_fb_return(fb);
                return 0;
            }
            
            // Copy header
            size_t pos = 0;
            memcpy(buffer + pos, header.c_str(), header.length());
            pos += header.length();
            
            // Copy image data
            memcpy(buffer + pos, fb->buf, fb->len);
            pos += fb->len;
            
            // Add closing CRLF
            buffer[pos++] = '\r';
            buffer[pos++] = '\n';
            
            // Return frame buffer
            esp_camera_fb_return(fb);
            
            // Small delay for frame rate control (~15 FPS)
            delay(66);
            
            return pos;
        });
    
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void handleBMP(AsyncWebServerRequest *request) {
    // BMP conversion is complex - for now return JPEG
    handleCapture(request);
}

void handleControl(AsyncWebServerRequest *request) {
    if (!request->hasParam("var") || !request->hasParam("val")) {
        request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
        return;
    }
    
    String var = request->getParam("var")->value();
    String val = request->getParam("val")->value();
    
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        request->send(500, "application/json", "{\"error\":\"Camera not available\"}");
        return;
    }
    
    int res = 0;
    
    if (var == "framesize") {
        res = s->set_framesize(s, (framesize_t)val.toInt());
        g_config.camera.framesize = val.toInt();
    } else if (var == "quality") {
        res = s->set_quality(s, val.toInt());
        g_config.camera.quality = val.toInt();
    } else if (var == "brightness") {
        res = s->set_brightness(s, val.toInt());
        g_config.camera.brightness = val.toInt();
    } else if (var == "contrast") {
        res = s->set_contrast(s, val.toInt());
        g_config.camera.contrast = val.toInt();
    } else if (var == "saturation") {
        res = s->set_saturation(s, val.toInt());
        g_config.camera.saturation = val.toInt();
    } else if (var == "hmirror") {
        res = s->set_hmirror(s, val.toInt());
        g_config.camera.hmirror = val.toInt();
    } else if (var == "vflip") {
        res = s->set_vflip(s, val.toInt());
        g_config.camera.vflip = val.toInt();
    } else if (var == "led_intensity") {
        int intensity = val.toInt();
        setLED(intensity);
        g_config.camera.led_intensity = intensity;
    }
    
    String response = res == 0 ? "{\"success\":true}" : "{\"error\":\"Failed to set " + var + "\"}";
    request->send(res == 0 ? 200 : 500, "application/json", response);
}

void handleSleep(AsyncWebServerRequest *request) {
    deinitCamera();
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Camera sleeping\"}");
}

void handleWake(AsyncWebServerRequest *request) {
    if (reinitCamera()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Camera awake\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to wake camera\"}");
    }
}

void handleRestart(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Restarting...\"}");
    Event event;
    event.type = EVENT_RESTART_REQUESTED;
    xQueueSend(eventQueue, &event, 0);
}

void handleFactoryReset(AsyncWebServerRequest *request) {
    Serial.println("========================================");
    Serial.println("Factory Reset Requested");
    Serial.println("  Clearing all configurations...");
    
    // Reset configuration to defaults
    resetConfiguration();
    
    Serial.println("  Configuration cleared successfully");
    Serial.println("  Device will restart in captive portal mode");
    Serial.println("========================================");
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", 
        "{\"success\":true,\"message\":\"Configuration reset. Device restarting in 3 seconds...\"}");
    addCORSHeaders(response);
    request->send(response);
    
    // Restart after short delay
    Event event;
    event.type = EVENT_RESTART_REQUESTED;
    xQueueSend(eventQueue, &event, 0);
}

void handleWiFiConnect(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static String body;
    
    if (index == 0) {
        body = "";
    }
    
    for (size_t i = 0; i < len; i++) {
        body += (char)data[i];
    }
    
    if (index + len == total) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            AsyncWebServerResponse *response = request->beginResponse(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            addCORSHeaders(response);
            request->send(response);
            return;
        }
        
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];
        
        if (!ssid || !password || strlen(ssid) == 0 || strlen(password) == 0) {
            AsyncWebServerResponse *response = request->beginResponse(400, "application/json", "{\"success\":false,\"message\":\"SSID and password are required\"}");
            addCORSHeaders(response);
            request->send(response);
            return;
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
                AsyncWebServerResponse *response = request->beginResponse(400, "application/json", "{\"success\":false,\"message\":\"Invalid IP address format\"}");
                addCORSHeaders(response);
                request->send(response);
                return;
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
            
            AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"success\":true,\"ip\":\"" + WiFi.localIP().toString() + "\"}");
            addCORSHeaders(response);
            request->send(response);
        } else {
            Serial.println("========================================");
            AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"success\":false,\"message\":\"Unable to connect. Check SSID, password, and signal strength.\"}");
            addCORSHeaders(response);
            request->send(response);
        }
    }
}

void handleNotFound(AsyncWebServerRequest *request) {
    // Redirect to root for captive portal
    if (ap_mode_active) {
        request->redirect("/");
    } else {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    }
}
