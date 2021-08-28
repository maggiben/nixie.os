#pragma once
#define ARDUINOJSON_USE_LONG_LONG 1
#include <WebServer.h>
#include <FS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>
#include "NixieSettings.h"
#include "utils.h"

class HttpHandler {
  private:
    WebServer* server;
    const char *hostname;
    IPAddress *accessPointIp;
    const char *softApSsid;
    QueueHandle_t connectQueue;
    boolean captivePortal();
    ESP32Time esp32Time;
  public:
    HttpHandler(WebServer *server, const char *hostname, IPAddress *accessPointIp, const char *softApSsid, QueueHandle_t connectQueue);
    void handleRoot();
    void getIp();
    void getRtcTime();
    void handleWifi();
    void handleWifiSave();
    void getWifiNetworkConfig();
    void handleNotFound();
    void begin();
    void stop();
};
