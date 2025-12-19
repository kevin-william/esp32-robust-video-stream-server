#include "captive_portal.h"
#include "config.h"
#include "app.h"

DNSServer dnsServer;
static bool captive_portal_active = false;
static unsigned long ap_start_time = 0;

bool startAPMode(const char* ssid, const char* password) {
    if (!ssid) ssid = DEFAULT_AP_SSID;
    if (!password) password = DEFAULT_AP_PASSWORD;
    
    WiFi.mode(WIFI_AP_STA);
    
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("AP Config Failed");
        return false;
    }
    
    if (!WiFi.softAP(ssid, password)) {
        Serial.println("AP Start Failed");
        return false;
    }
    
    Serial.println("AP Started");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("AP SSID: ");
    Serial.println(ssid);
    
    ap_start_time = millis();
    return true;
}

void stopAPMode() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    captive_portal_active = false;
}

bool startCaptivePortal() {
    if (!startAPMode()) {
        return false;
    }
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
    captive_portal_active = true;
    
    return true;
}

void stopCaptivePortal() {
    dnsServer.stop();
    stopAPMode();
}

bool isCaptivePortalActive() {
    return captive_portal_active;
}

void handleCaptivePortal() {
    if (captive_portal_active) {
        dnsServer.processNextRequest();
    }
}

String scanWiFiNetworks() {
    String json = "[";
    int n = WiFi.scanNetworks();
    
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i));
        json += "}";
    }
    
    json += "]";
    WiFi.scanDelete();
    return json;
}

bool connectToWiFi(const char* ssid, const char* password, int timeout_ms) {
    Serial.printf("Connecting to WiFi: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        
        // Send event
        Event event;
        event.type = EVENT_WIFI_CONNECTED;
        event.data = 0;
        event.ptr = nullptr;
        xQueueSend(eventQueue, &event, 0);
        
        return true;
    }
    
    Serial.println("WiFi Connection Failed");
    return false;
}

bool tryConnectSavedNetworks() {
    if (g_config.network_count == 0) {
        Serial.println("No saved networks to try");
        return false;
    }
    
    // Sort networks by priority (simple bubble sort for small array)
    for (int i = 0; i < g_config.network_count - 1; i++) {
        for (int j = 0; j < g_config.network_count - i - 1; j++) {
            if (g_config.networks[j].priority < g_config.networks[j + 1].priority) {
                WiFiNetwork temp = g_config.networks[j];
                g_config.networks[j] = g_config.networks[j + 1];
                g_config.networks[j + 1] = temp;
            }
        }
    }
    
    // Try each network in priority order
    for (int i = 0; i < g_config.network_count; i++) {
        Serial.printf("Trying network %d/%d: %s (priority %d)\n", 
                     i + 1, g_config.network_count, 
                     g_config.networks[i].ssid,
                     g_config.networks[i].priority);
        
        if (connectToWiFi(g_config.networks[i].ssid, 
                         g_config.networks[i].password, 
                         15000)) {
            return true;
        }
    }
    
    return false;
}
