#pragma once
#include <WebServer.h>
#include <FS.h>
#include <SD.h>
#include "utils.h"

class HttpHandler {
  private:
    WebServer* server;
    const char* hostname;
    boolean captivePortal();
  public:
    HttpHandler(WebServer *server, const char *hostname);
    void handleRoot();
    void handleNotFound();
    void begin();
    void stop();
};
