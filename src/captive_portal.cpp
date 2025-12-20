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

bool connectToWiFi(const char* ssid, const char* password, int timeout_ms) {
    Serial.printf("Attempting to connect to WiFi: %s\n", ssid);
    
    // Keep AP mode active during connection attempt to maintain captive portal
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✓ WiFi Connected successfully!");
        Serial.print("  IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("  RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        
        // Send event
        Event event;
        event.type = EVENT_WIFI_CONNECTED;
        event.data = 0;
        event.ptr = nullptr;
        xQueueSend(eventQueue, &event, 0);
        
        return true;
    }
    
    Serial.println("✗ WiFi Connection Failed!");
    Serial.println("  Staying in captive portal mode");
    return false;
}

bool connectToWiFiWithStaticIP(const char* ssid, const char* password, 
                                 IPAddress ip, IPAddress gateway, 
                                 int timeout_ms) {
    Serial.printf("Attempting to connect to WiFi with static IP: %s\n", ssid);
    Serial.printf("  Static IP: %s\n", ip.toString().c_str());
    Serial.printf("  Gateway: %s\n", gateway.toString().c_str());
    
    // Use default subnet mask (255.255.255.0) and Google DNS
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns1(8, 8, 8, 8);
    IPAddress dns2(8, 8, 4, 4);
    
    // Configure static IP
    if (!WiFi.config(ip, gateway, subnet, dns1, dns2)) {
        Serial.println("Failed to configure static IP");
        return false;
    }
    
    // Keep AP mode active during connection attempt
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✓ WiFi Connected successfully with static IP!");
        Serial.print("  IP Address: ");
        Serial.println(WiFi.localIP());
        
        // Send event
        Event event;
        event.type = EVENT_WIFI_CONNECTED;
        event.data = 0;
        event.ptr = nullptr;
        xQueueSend(eventQueue, &event, 0);
        
        return true;
    }
    
    Serial.println("✗ WiFi Connection Failed!");
    Serial.println("  Staying in captive portal mode");
    return false;
}

bool tryConnectSavedNetworks() {
    if (g_config.network_count == 0) {
        Serial.println("No saved WiFi networks found in configuration");
        return false;
    }
    
    Serial.println("========================================");
    Serial.printf("Found %d saved network(s)\n", g_config.network_count);
    Serial.println("Attempting to connect...");
    Serial.println("========================================");
    
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
        Serial.printf("Attempt %d/%d - Network: '%s' (priority: %d)\n", 
                     i + 1, g_config.network_count, 
                     g_config.networks[i].ssid,
                     g_config.networks[i].priority);
        
        bool connected = false;
        
        if (g_config.networks[i].use_static_ip) {
            Serial.println("  Using static IP configuration");
            IPAddress ip(g_config.networks[i].static_ip[0], g_config.networks[i].static_ip[1], 
                        g_config.networks[i].static_ip[2], g_config.networks[i].static_ip[3]);
            IPAddress gateway(g_config.networks[i].gateway[0], g_config.networks[i].gateway[1], 
                            g_config.networks[i].gateway[2], g_config.networks[i].gateway[3]);
            
            connected = connectToWiFiWithStaticIP(g_config.networks[i].ssid, 
                                                  g_config.networks[i].password,
                                                  ip, gateway, 15000);
        } else {
            Serial.println("  Using DHCP");
            connected = connectToWiFi(g_config.networks[i].ssid, 
                                     g_config.networks[i].password, 
                                     15000);
        }
        
        if (connected) {
            Serial.println("========================================");
            return true;
        } else {
            Serial.printf("  ✗ Failed to connect to '%s'\n", g_config.networks[i].ssid);
        }
    }
    
    Serial.println("========================================");
    Serial.println("All connection attempts failed");
    Serial.println("========================================");
    return false;
}
