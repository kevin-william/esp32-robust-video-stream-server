#include "diagnostics.h"
#include "app.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <soc/rtc.h>

DiagnosticCounters g_diag = {0, 0, 0, 0, 0, 0.0, 0, 0, 0};

void initDiagnostics() {
    g_diag.frame_count = 0;
    g_diag.frame_errors = 0;
    g_diag.total_frames_sent = 0;
    g_diag.last_frame_time = millis();
    g_diag.fps_calculation_start = millis();
    g_diag.current_fps = 0.0;
    g_diag.total_bytes_sent = 0;
    g_diag.wifi_reconnects = 0;
    g_diag.task_overruns = 0;
}

void updateFrameStats() {
    g_diag.frame_count++;
    unsigned long now = millis();
    
    // Calculate FPS every second
    if (now - g_diag.fps_calculation_start >= 1000) {
        unsigned long elapsed = now - g_diag.fps_calculation_start;
        g_diag.current_fps = (g_diag.frame_count * 1000.0) / elapsed;
        g_diag.frame_count = 0;
        g_diag.fps_calculation_start = now;
    }
    
    g_diag.last_frame_time = now;
}

float getCPUFrequency() {
    rtc_cpu_freq_config_t conf;
    rtc_clk_cpu_freq_get_config(&conf);
    return conf.freq_mhz;
}

uint32_t getCore0IdleTime() {
    return uxTaskGetSystemState(NULL, 0, NULL);
}

uint32_t getCore1IdleTime() {
    return uxTaskGetSystemState(NULL, 0, NULL);
}

size_t getTaskStackHighWaterMark(TaskHandle_t task) {
    if (task == NULL) return 0;
    return uxTaskGetStackHighWaterMark(task);
}

String getDiagnosticsJSON() {
    StaticJsonDocument<1536> doc;
    
    // System Info
    JsonObject system = doc.createNestedObject("system");
    system["uptime_sec"] = millis() / 1000;
    system["cpu_freq_mhz"] = getCPUFrequency();
    system["chip_model"] = ESP.getChipModel();
    system["chip_revision"] = ESP.getChipRevision();
    system["cpu_cores"] = ESP.getChipCores();
    system["sdk_version"] = ESP.getSdkVersion();
    
    // Memory Info
    JsonObject memory = doc.createNestedObject("memory");
    memory["heap_size"] = ESP.getHeapSize();
    memory["free_heap"] = ESP.getFreeHeap();
    memory["min_free_heap"] = ESP.getMinFreeHeap();
    memory["heap_usage_pct"] = 100.0 * (1.0 - (float)ESP.getFreeHeap() / ESP.getHeapSize());
    
    if (psramFound()) {
        memory["psram_size"] = ESP.getPsramSize();
        memory["free_psram"] = ESP.getFreePsram();
        memory["min_free_psram"] = ESP.getMinFreePsram();
        memory["psram_usage_pct"] = 100.0 * (1.0 - (float)ESP.getFreePsram() / ESP.getPsramSize());
    } else {
        memory["psram_available"] = false;
    }
    
    // Camera/Streaming Stats
    JsonObject streaming = doc.createNestedObject("streaming");
    streaming["fps"] = g_diag.current_fps;
    streaming["total_frames"] = g_diag.total_frames_sent;
    streaming["frame_errors"] = g_diag.frame_errors;
    streaming["error_rate_pct"] = (g_diag.total_frames_sent + g_diag.frame_errors) > 0 ? 
        100.0 * g_diag.frame_errors / (g_diag.total_frames_sent + g_diag.frame_errors) : 0.0;
    streaming["last_frame_ms_ago"] = millis() - g_diag.last_frame_time;
    streaming["total_bytes_sent"] = g_diag.total_bytes_sent;
    streaming["camera_initialized"] = camera_initialized;
    streaming["camera_sleeping"] = camera_sleeping;
    
    // WiFi Stats
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["connected"] = WiFi.status() == WL_CONNECTED;
    if (WiFi.status() == WL_CONNECTED) {
        wifi["ssid"] = WiFi.SSID();
        wifi["rssi"] = WiFi.RSSI();
        wifi["ip"] = WiFi.localIP().toString();
        wifi["gateway"] = WiFi.gatewayIP().toString();
        wifi["dns"] = WiFi.dnsIP().toString();
        wifi["channel"] = WiFi.channel();
        wifi["tx_power"] = WiFi.getTxPower();
    }
    wifi["reconnects"] = g_diag.wifi_reconnects;
    wifi["ap_mode_active"] = ap_mode_active;
    
    // Task Stats
    JsonObject tasks = doc.createNestedObject("tasks");
    
    if (cameraTaskHandle != NULL) {
        JsonObject cam_task = tasks.createNestedObject("camera_task");
        cam_task["state"] = eTaskGetState(cameraTaskHandle);
        cam_task["priority"] = uxTaskPriorityGet(cameraTaskHandle);
        cam_task["stack_hwm"] = getTaskStackHighWaterMark(cameraTaskHandle);
        cam_task["core"] = xPortGetCoreID();
    }
    
    if (webServerTaskHandle != NULL) {
        JsonObject web_task = tasks.createNestedObject("web_task");
        web_task["state"] = eTaskGetState(webServerTaskHandle);
        web_task["priority"] = uxTaskPriorityGet(webServerTaskHandle);
        web_task["stack_hwm"] = getTaskStackHighWaterMark(webServerTaskHandle);
    }
    
    if (watchdogTaskHandle != NULL) {
        JsonObject wd_task = tasks.createNestedObject("watchdog_task");
        wd_task["state"] = eTaskGetState(watchdogTaskHandle);
        wd_task["priority"] = uxTaskPriorityGet(watchdogTaskHandle);
        wd_task["stack_hwm"] = getTaskStackHighWaterMark(watchdogTaskHandle);
    }
    
    tasks["overruns"] = g_diag.task_overruns;
    
    // Performance Metrics
    JsonObject perf = doc.createNestedObject("performance");
    perf["frame_time_target_ms"] = 66; // ~15 FPS
    perf["actual_frame_time_ms"] = g_diag.current_fps > 0 ? 1000.0 / g_diag.current_fps : 0;
    
    float heap_frag = 100.0 * (ESP.getHeapSize() - ESP.getFreeHeap() - ESP.getMaxAllocHeap()) / ESP.getHeapSize();
    perf["heap_fragmentation_pct"] = heap_frag;
    
    // Health Status
    JsonObject health = doc.createNestedObject("health");
    health["overall"] = "ok";
    
    JsonArray warnings = health.createNestedArray("warnings");
    JsonArray errors = health.createNestedArray("errors");
    
    if (ESP.getFreeHeap() < 20000) {
        warnings.add("Low free heap (<20KB)");
    }
    
    if (psramFound() && ESP.getFreePsram() < 100000) {
        warnings.add("Low free PSRAM (<100KB)");
    }
    
    if (g_diag.current_fps < 5 && camera_initialized && !camera_sleeping) {
        warnings.add("Low FPS (<5)");
    }
    
    if (WiFi.status() == WL_CONNECTED && WiFi.RSSI() < -80) {
        warnings.add("Weak WiFi signal (<-80 dBm)");
    }
    
    // Camera task removed - AsyncWebServer handles streaming directly
    // Stack check removed as task no longer exists
    
    unsigned long total_attempts = g_diag.total_frames_sent + g_diag.frame_errors;
    if (total_attempts > 10 && g_diag.frame_errors > total_attempts * 0.1) {
        errors.add("High frame error rate (>10%)");
    }
    
    if (g_diag.frame_errors > g_diag.frame_count * 0.1) {
        errors.add("High frame error rate (>10%)");
    }
    
    if (errors.size() > 0) {
        health["overall"] = "error";
    } else if (warnings.size() > 0) {
        health["overall"] = "warning";
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}
