#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>

// DNS Server for captive portal
extern DNSServer dnsServer;

// Captive portal functions
bool startCaptivePortal();
void stopCaptivePortal();
bool isCaptivePortalActive();
void handleCaptivePortal();

// AP Mode management
bool startAPMode(const char* ssid = nullptr, const char* password = nullptr);
void stopAPMode();

// WiFi connection
bool connectToWiFi(const char* ssid, const char* password, int timeout_ms = 20000);
bool connectToWiFiWithStaticIP(const char* ssid, const char* password, IPAddress ip,
                               IPAddress gateway, int timeout_ms = 20000);
bool tryConnectSavedNetworks();

#endif  // CAPTIVE_PORTAL_H
