#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <esp_http_server.h>

// Initialize OTA subsystem
void initOTA();

// Register OTA endpoints with HTTP server
void registerOTAEndpoints(httpd_handle_t server);

// Get OTA status
const char* getOTAStatus();
int getOTAProgress();

#endif // OTA_UPDATE_H
