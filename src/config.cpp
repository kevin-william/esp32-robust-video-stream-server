#include "config.h"
#include "storage.h"
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>

void setDefaultConfiguration() {
    // Clear network list
    g_config.network_count = 0;
    memset(g_config.networks, 0, sizeof(g_config.networks));
    
    // Camera defaults
    g_config.camera.framesize = DEFAULT_FRAMESIZE;
    g_config.camera.quality = DEFAULT_QUALITY;
    g_config.camera.brightness = DEFAULT_BRIGHTNESS;
    g_config.camera.contrast = DEFAULT_CONTRAST;
    g_config.camera.saturation = DEFAULT_SATURATION;
    g_config.camera.gainceiling = 0;
    g_config.camera.colorbar = 0;
    g_config.camera.awb = 1;
    g_config.camera.agc = 1;
    g_config.camera.aec = 1;
    g_config.camera.hmirror = 0;
    g_config.camera.vflip = 0;
    g_config.camera.awb_gain = 1;
    g_config.camera.agc_gain = 0;
    g_config.camera.aec_value = 0;
    g_config.camera.special_effect = 0;
    g_config.camera.wb_mode = 0;
    g_config.camera.ae_level = 0;
    g_config.camera.dcw = 1;
    g_config.camera.bpc = 0;
    g_config.camera.wpc = 1;
    g_config.camera.raw_gma = 1;
    g_config.camera.lenc = 1;
    g_config.camera.led_intensity = 0;
    
    // System defaults
    strcpy(g_config.admin_password_hash, "");
    g_config.ota_enabled = false;
    strcpy(g_config.ota_password, "");
    g_config.log_level = 2; // INFO
    g_config.use_https = false;
    g_config.server_port = 80;
}

bool validateConfiguration(const JsonDocument& doc) {
    // Basic validation
    if (!doc.containsKey("camera")) {
        Serial.println("Config validation failed: missing camera section");
        return false;
    }
    
    // Validate camera settings ranges
    JsonObjectConst camera = doc["camera"].as<JsonObjectConst>();
    if (camera.containsKey("quality")) {
        int quality = camera["quality"];
        if (quality < 0 || quality > 63) {
            Serial.println("Config validation failed: invalid quality");
            return false;
        }
    }
    
    return true;
}

bool loadConfiguration() {
    StaticJsonDocument<CONFIG_JSON_SIZE> doc;
    
    // Try SD card first
    if (isSDCardMounted()) {
        String configStr = readFile(CONFIG_FILE_PATH);
        if (configStr.length() > 0) {
            DeserializationError error = deserializeJson(doc, configStr);
            if (error) {
                Serial.print("Failed to parse config from SD: ");
                Serial.println(error.c_str());
            } else if (validateConfiguration(doc)) {
                goto parse_config;
            }
        }
    }
    
    // Try NVS fallback
    {
        String configStr = readFromNVS("config", "");
        if (configStr.length() > 0) {
            DeserializationError error = deserializeJson(doc, configStr);
            if (error) {
                Serial.print("Failed to parse config from NVS: ");
                Serial.println(error.c_str());
                return false;
            } else if (!validateConfiguration(doc)) {
                return false;
            }
            goto parse_config;
        }
    }
    
    return false;

parse_config:
    // Parse WiFi networks
    if (doc.containsKey("networks")) {
        JsonArray networks = doc["networks"];
        g_config.network_count = 0;
        for (JsonObject network : networks) {
            if (g_config.network_count >= MAX_WIFI_NETWORKS) break;
            
            const char* ssid = network["ssid"];
            const char* password = network["password"];
            int priority = network["priority"] | 0;
            
            if (ssid && password) {
                strncpy(g_config.networks[g_config.network_count].ssid, ssid, 31);
                strncpy(g_config.networks[g_config.network_count].password, password, 63);
                g_config.networks[g_config.network_count].priority = priority;
                
                // Parse static IP settings (optional)
                g_config.networks[g_config.network_count].use_static_ip = network["use_static_ip"] | false;
                if (g_config.networks[g_config.network_count].use_static_ip && network.containsKey("static_ip")) {
                    JsonArray ip = network["static_ip"];
                    if (ip.size() == 4) {
                        for (int i = 0; i < 4; i++) {
                            g_config.networks[g_config.network_count].static_ip[i] = ip[i];
                        }
                    }
                    
                    if (network.containsKey("gateway")) {
                        JsonArray gw = network["gateway"];
                        if (gw.size() == 4) {
                            for (int i = 0; i < 4; i++) {
                                g_config.networks[g_config.network_count].gateway[i] = gw[i];
                            }
                        }
                    }
                    
                    if (network.containsKey("subnet")) {
                        JsonArray sn = network["subnet"];
                        if (sn.size() == 4) {
                            for (int i = 0; i < 4; i++) {
                                g_config.networks[g_config.network_count].subnet[i] = sn[i];
                            }
                        }
                    }
                    
                    if (network.containsKey("dns1")) {
                        JsonArray dns = network["dns1"];
                        if (dns.size() == 4) {
                            for (int i = 0; i < 4; i++) {
                                g_config.networks[g_config.network_count].dns1[i] = dns[i];
                            }
                        }
                    }
                    
                    if (network.containsKey("dns2")) {
                        JsonArray dns = network["dns2"];
                        if (dns.size() == 4) {
                            for (int i = 0; i < 4; i++) {
                                g_config.networks[g_config.network_count].dns2[i] = dns[i];
                            }
                        }
                    }
                }
                
                g_config.network_count++;
            }
        }
    }
    
    // Parse camera settings
    if (doc.containsKey("camera")) {
        JsonObjectConst camera = doc["camera"].as<JsonObjectConst>();
        g_config.camera.framesize = camera["framesize"] | DEFAULT_FRAMESIZE;
        g_config.camera.quality = camera["quality"] | DEFAULT_QUALITY;
        g_config.camera.brightness = camera["brightness"] | DEFAULT_BRIGHTNESS;
        g_config.camera.contrast = camera["contrast"] | DEFAULT_CONTRAST;
        g_config.camera.saturation = camera["saturation"] | DEFAULT_SATURATION;
        g_config.camera.gainceiling = camera["gainceiling"] | 0;
        g_config.camera.colorbar = camera["colorbar"] | 0;
        g_config.camera.awb = camera["awb"] | 1;
        g_config.camera.agc = camera["agc"] | 1;
        g_config.camera.aec = camera["aec"] | 1;
        g_config.camera.hmirror = camera["hmirror"] | 0;
        g_config.camera.vflip = camera["vflip"] | 0;
        g_config.camera.awb_gain = camera["awb_gain"] | 1;
        g_config.camera.agc_gain = camera["agc_gain"] | 0;
        g_config.camera.aec_value = camera["aec_value"] | 0;
        g_config.camera.special_effect = camera["special_effect"] | 0;
        g_config.camera.wb_mode = camera["wb_mode"] | 0;
        g_config.camera.ae_level = camera["ae_level"] | 0;
        g_config.camera.dcw = camera["dcw"] | 1;
        g_config.camera.bpc = camera["bpc"] | 0;
        g_config.camera.wpc = camera["wpc"] | 1;
        g_config.camera.raw_gma = camera["raw_gma"] | 1;
        g_config.camera.lenc = camera["lenc"] | 1;
        g_config.camera.led_intensity = camera["led_intensity"] | 0;
    }
    
    // Parse system settings
    if (doc.containsKey("admin_password_hash")) {
        strncpy(g_config.admin_password_hash, doc["admin_password_hash"], 64);
    }
    g_config.ota_enabled = doc["ota_enabled"] | false;
    if (doc.containsKey("ota_password")) {
        strncpy(g_config.ota_password, doc["ota_password"], 31);
    }
    g_config.log_level = doc["log_level"] | 2;
    g_config.use_https = doc["use_https"] | false;
    g_config.server_port = doc["server_port"] | 80;
    
    return true;
}

bool saveConfiguration() {
    StaticJsonDocument<CONFIG_JSON_SIZE> doc;
    
    // Build networks array
    JsonArray networks = doc.createNestedArray("networks");
    for (int i = 0; i < g_config.network_count; i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = g_config.networks[i].ssid;
        network["password"] = g_config.networks[i].password;
        network["priority"] = g_config.networks[i].priority;
        
        // Save static IP settings if configured
        if (g_config.networks[i].use_static_ip) {
            network["use_static_ip"] = true;
            JsonArray ip = network.createNestedArray("static_ip");
            for (int j = 0; j < 4; j++) ip.add(g_config.networks[i].static_ip[j]);
            
            JsonArray gw = network.createNestedArray("gateway");
            for (int j = 0; j < 4; j++) gw.add(g_config.networks[i].gateway[j]);
            
            JsonArray sn = network.createNestedArray("subnet");
            for (int j = 0; j < 4; j++) sn.add(g_config.networks[i].subnet[j]);
            
            JsonArray dns1 = network.createNestedArray("dns1");
            for (int j = 0; j < 4; j++) dns1.add(g_config.networks[i].dns1[j]);
            
            JsonArray dns2 = network.createNestedArray("dns2");
            for (int j = 0; j < 4; j++) dns2.add(g_config.networks[i].dns2[j]);
        }
    }
    
    // Build camera settings
    JsonObject camera = doc.createNestedObject("camera");
    camera["framesize"] = g_config.camera.framesize;
    camera["quality"] = g_config.camera.quality;
    camera["brightness"] = g_config.camera.brightness;
    camera["contrast"] = g_config.camera.contrast;
    camera["saturation"] = g_config.camera.saturation;
    camera["gainceiling"] = g_config.camera.gainceiling;
    camera["colorbar"] = g_config.camera.colorbar;
    camera["awb"] = g_config.camera.awb;
    camera["agc"] = g_config.camera.agc;
    camera["aec"] = g_config.camera.aec;
    camera["hmirror"] = g_config.camera.hmirror;
    camera["vflip"] = g_config.camera.vflip;
    camera["awb_gain"] = g_config.camera.awb_gain;
    camera["agc_gain"] = g_config.camera.agc_gain;
    camera["aec_value"] = g_config.camera.aec_value;
    camera["special_effect"] = g_config.camera.special_effect;
    camera["wb_mode"] = g_config.camera.wb_mode;
    camera["ae_level"] = g_config.camera.ae_level;
    camera["dcw"] = g_config.camera.dcw;
    camera["bpc"] = g_config.camera.bpc;
    camera["wpc"] = g_config.camera.wpc;
    camera["raw_gma"] = g_config.camera.raw_gma;
    camera["lenc"] = g_config.camera.lenc;
    camera["led_intensity"] = g_config.camera.led_intensity;
    
    // System settings
    doc["admin_password_hash"] = g_config.admin_password_hash;
    doc["ota_enabled"] = g_config.ota_enabled;
    doc["ota_password"] = g_config.ota_password;
    doc["log_level"] = g_config.log_level;
    doc["use_https"] = g_config.use_https;
    doc["server_port"] = g_config.server_port;
    
    // Serialize to string
    String output;
    serializeJsonPretty(doc, output);
    
    // Try to save to SD card first
    bool savedToSD = false;
    if (isSDCardMounted()) {
        createDirectory("/config");
        savedToSD = writeFile(CONFIG_FILE_PATH, output);
        if (savedToSD) {
            Serial.println("Configuration saved to SD card");
        }
    }
    
    // Always save to NVS as backup
    bool savedToNVS = saveToNVS("config", output);
    if (savedToNVS) {
        Serial.println("Configuration saved to NVS");
    }
    
    return savedToSD || savedToNVS;
}

bool resetConfiguration() {
    setDefaultConfiguration();
    
    // Delete from SD
    if (isSDCardMounted()) {
        deleteFile(CONFIG_FILE_PATH);
        deleteFile(CONFIG_BACKUP_PATH);
    }
    
    // Clear NVS
    clearNVS();
    
    return true;
}
