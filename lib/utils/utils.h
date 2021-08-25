#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

struct WIFI_CREDENTIAL {
  /* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
  char ssid[32];
  char password[32];
};

/** Is this an IP? */
boolean isIp(String str);
/** IP to String? */
String toStringIp(IPAddress ip);
/** very crude extension type to mime type resolver **/
String getContentType(WebServer *server, String filename);
/** load wifi credentails from eeprom **/
WIFI_CREDENTIAL* loadCredentials();
/** save wifi cretentials to eeprom **/
WIFI_CREDENTIAL* saveCredentials();