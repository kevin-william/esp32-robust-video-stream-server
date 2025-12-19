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
void handleWiFiScan(AsyncWebServerRequest *request);
void handleWiFiConnect(AsyncWebServerRequest *request);
void handleConfig(AsyncWebServerRequest *request);
void handleOTA(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);

// Stream helper functions
void streamJPEG(AsyncWebServerRequest *request);

// CORS middleware
void addCORSHeaders(AsyncWebServerResponse *response);

// Authentication
bool checkAuthentication(AsyncWebServerRequest *request);
String generateCSRFToken();
bool validateCSRFToken(const String& token);

#endif // WEB_SERVER_H
