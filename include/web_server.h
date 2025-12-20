#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <esp_http_server.h>

// Server handle
extern httpd_handle_t server;

// Initialize web server with all routes
void initWebServer();

// Stop web server
void stopWebServer();

// Route handlers
esp_err_t handleRoot(httpd_req_t *req);
esp_err_t handleStatus(httpd_req_t *req);
esp_err_t handleSleepStatus(httpd_req_t *req);
esp_err_t handleCapture(httpd_req_t *req);
esp_err_t handleStream(httpd_req_t *req);
esp_err_t handleBMP(httpd_req_t *req);
esp_err_t handleControl(httpd_req_t *req);
esp_err_t handleSleep(httpd_req_t *req);
esp_err_t handleWake(httpd_req_t *req);
esp_err_t handleRestart(httpd_req_t *req);
esp_err_t handleFactoryReset(httpd_req_t *req);
esp_err_t handleWiFiConnect(httpd_req_t *req);
esp_err_t handleDiagnostics(httpd_req_t *req);

#endif
