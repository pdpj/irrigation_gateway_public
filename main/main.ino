#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#pragma message ("ArduinoJson version: " ARDUINOJSON_VERSION)

#include "config.h"
#include "web_page.h"
#include "api_handlers.h"
#include "sensors.h"
#include "irrigation.h"

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);

  sensorsTaskStart();
  irrigationTaskStart();   // already configures the pin (IRRIGATION_VALVE_PIN == LED_BUILTIN, for now)

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  irrigationSyncTime();   // NTP - needs WiFi already connected; anchors the INTERVAL trigger

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", PAGE_HTML);
  });

  server.on("/led", HTTP_POST, [](AsyncWebServerRequest* request) {},
            NULL, handleLedPost);

  server.on("/api/status", HTTP_GET, handleStatusGet);
  server.on("/api/sensors/refresh", HTTP_POST, handleSensorsRefreshPost);

  server.begin();
  Serial.println("Server running.");
}

void loop() {
}
