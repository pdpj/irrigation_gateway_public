#pragma once
#include <ESPAsyncWebServer.h>

// REST API route handlers. Registered in main.ino via server.on(...).
void handleLedPost(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
void handleStatusGet(AsyncWebServerRequest* request);
void handleSensorsRefreshPost(AsyncWebServerRequest* request);
