#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

/* Is this an IP? */
boolean isIp(String str);
/* IP to String? */
String toStringIp(IPAddress ip);
/* very crude extension type to mime type resolver */
String getContentType(WebServer *server, String filename);
