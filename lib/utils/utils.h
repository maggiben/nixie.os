#pragma once
#include <WiFi.h>

/** Is this an IP? */
boolean isIp(String str);

/** IP to String? */
String toStringIp(IPAddress ip);