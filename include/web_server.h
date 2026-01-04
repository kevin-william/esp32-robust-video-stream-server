#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Server instance
extern AsyncWebServer server;

// Initialize web server with all routes
void initWebServer();

// Route handlers
void handleStatus(AsyncWebServerRequest *request);
void handleSleepStatus(AsyncWebServerRequest *request);
void handleCapture(AsyncWebServerRequest *request);
void handleStream(AsyncWebServerRequest *request);
void handleBMP(AsyncWebServerRequest *request);
void handleControl(AsyncWebServerRequest *request);
void handleSleep(AsyncWebServerRequest *request);
void handleWake(AsyncWebServerRequest *request);
void handleRestart(AsyncWebServerRequest *request);
void handleFactoryReset(AsyncWebServerRequest *request);
void handleWiFiConnect(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleConfig(AsyncWebServerRequest *request);
void handleOTA(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);

// New endpoints as per requirements
void handleReset(AsyncWebServerRequest *request);    // Hard/Brute Reset with cleanup
void handleStop(AsyncWebServerRequest *request);     // Stop camera service and power-down
void handleStart(AsyncWebServerRequest *request);    // Start/restart camera service

// Stream helper functions
void streamJPEG(AsyncWebServerRequest *request);

// CORS middleware
void addCORSHeaders(AsyncWebServerResponse *response);

// Authentication
bool checkAuthentication(AsyncWebServerRequest *request);
String generateCSRFToken();
bool validateCSRFToken(const String& token);

#endif // WEB_SERVER_H
