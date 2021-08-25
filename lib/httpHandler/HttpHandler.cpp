#include "httpHandler.h"

HttpHandler::HttpHandler(WebServer *server, const char *hostname, IPAddress *accessPointIp, const char *softApSsid) {
  this->server = server;
  this->hostname = hostname;
  this->accessPointIp = accessPointIp;
  this->softApSsid = softApSsid;

/* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  this->server->on("/", [this]() {
    return this->handleRoot();
  });
  this->server->on("/ip", HTTP_GET, [this]() {
    return this->getIp();
  });
  this->server->on("/rtc", HTTP_GET, [this]() {
    return this->getRtcTime();
  });
  this->server->on("/wifi", [this]() {
    return this->handleWifi();
  });
  this->server->on("/wifinetworkconfig", [this]() {
    return this->getWifiNetworkConfig();
  });
  this->server->on("/inline", [this]() {
    this->server->send(200, "text/plain", "this works as well");
  });
  this->server->onNotFound([this](){
    return this->handleNotFound();
  });
  this->server->serveStatic("/", SD, "/");
}


void HttpHandler::begin() {
  return this->server->begin(); // Web server start
}

void HttpHandler::stop() {
  return this->server->stop();
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean HttpHandler::captivePortal() {
  Serial.print("hostHeader: ");
  Serial.println(this->server->hostHeader());
  if (!isIp(this->server->hostHeader()) && this->server->hostHeader() != (String(this->hostname)+".local")) {
    Serial.print("Request redirected to captive portal");
    this->server->sendHeader("Location", String("http://") + toStringIp(server->client().localIP()), true);
    this->server->send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    this->server->client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Handle root or redirect to captive portal */
void HttpHandler::handleRoot() {
  if (this->captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  fs::FS &fs = SD;
  String path = server->uri(); //saves the to a string server uri ex.(192.168.100.110/edit server uri is "/edit")
  Serial.print("path ");
  Serial.println(path);

  //To send the index.html when the serves uri is "/"
  if (path.endsWith("/")) {
    path += "index.html";
  }
  String contentType = getContentType(server, path);
  Serial.print("contentType ");
  Serial.println(contentType);
  File file = fs.open(path, "r"); //Open the File with file name = to path with intention to read it. For other modes see <a href="https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html" style="font-size: 13.5px;"> https://arduino-esp8266.readthedocs.io/en/latest/...</a>
  size_t sent = this->server->streamFile(file, contentType); //sends the file to the server references from <a href="https://github.com/espressif/arduino-esp32/blob/master/libraries/WebServer/src/WebServer.h" style="font-size: 13.5px;"> https://arduino-esp8266.readthedocs.io/en/latest/...</a>
  if (sent != file.size()) {
    Serial.println("Sent less data than expected!");
  }
  Serial.print("sent: ");
  Serial.println(sent);
  file.close(); //Close the file
}

void HttpHandler::getIp() {
  // To get an approximate value for the document site go to: https://arduinojson.org/v6/assistant/
  StaticJsonDocument<128> document;
  String response;
  document["apIP"] = this->server->client().localIP();
  document["localIP"] = this->accessPointIp->toString();
  serializeJson(document, response);
  this->server->send(200, "application/json", response);
}

void HttpHandler::getRtcTime() {
  StaticJsonDocument<32> document;
  String response;
  document["rtc"] = esp32Time.getEpoch();
  serializeJson(document, response);
  this->server->send(200, "application/json", response);
}

/** Wifi config page handler */
void HttpHandler::handleWifi() {
  // IPAddress *accessPointIp = (IPAddress*)pvPortMalloc(sizeof(IPAddress));
  // // Reset 
  // memset(accessPointIp, 0, sizeof(IPAddress));
  // // Copy IPAddress
  // memcpy(accessPointIp, this->accessPointIp, sizeof(IPAddress));
  this->server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  this->server->sendHeader("Pragma", "no-cache");
  this->server->sendHeader("Expires", "-1");
  this->server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  this->server->send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  this->server->sendContent(
    "<html><head></head><body>"
    "<h1>Wifi config</h1>"
  );
  if (this->server->client().localIP() == *this->accessPointIp) {
    // this->server->sendContent(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
    this->server->sendContent(String("<p>You are connected through the soft AP </p>"));
  } else {
    // this->server->sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
    this->server->sendContent(String("<p>You are connected through the wifi network </p>"));
  }
  this->server->sendContent(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>"
  );
  // this->server->sendContent(String() + "<tr><td>SSID " + String(softAP_ssid) + "</td></tr>");
  this->server->sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.softAPIP()) + "</td></tr>");
  this->server->sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
  );
  // this->server->sendContent(String() + "<tr><td>SSID " + String(ssid) + "</td></tr>");
  this->server->sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.localIP()) + "</td></tr>");
  this->server->sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>"
  );
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      this->server->sendContent(String() + "\r\n<tr><td>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":" *") + " (" + WiFi.RSSI(i) + ")</td></tr>");
    }
  } else {
    this->server->sendContent(String() + "<tr><td>No WLAN found</td></tr>");
  }
  this->server->sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network' name='n'/>"
    "<br /><input type='password' placeholder='password' name='p'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p>You may want to <a href='/'>return to the home page</a>.</p>"
    "</body></html>"
  );
  this->server->client().stop(); // Stop is needed because we sent no content length
}

void HttpHandler::getWifiNetworkConfig() {
  WIFI_CREDENTIAL *wifiCredential = loadCredentials();
  this->server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  this->server->sendHeader("Pragma", "no-cache");
  this->server->sendHeader("Expires", "-1");
  this->server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  this->server->send(200, "application/xml", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  this->server->sendContent("<wifi-config>");
  if (this->server->client().localIP() == *this->accessPointIp) {
    this->server->sendContent(String("<connection-ssid>") + this->softApSsid + "</connection-type>");
  } else {
    this->server->sendContent(String("<connection-type>") + wifiCredential->ssid + "</connection-type>");
  }
  this->server->sendContent(String() + "<ssid>" + String(this->softApSsid) + "</ssid>");
  this->server->sendContent(String() + "<ip>" + toStringIp(WiFi.localIP()) + "</ip>");
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  this->server->sendContent("<networks>");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      this->server->sendContent(String() + "<ssid>" + WiFi.SSID(i) + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":" *") + " (" + WiFi.RSSI(i) + ")</ssid>");
    }
  } else {
    this->server->sendContent(String() + "<error>No WLAN found</error>");
  }
  this->server->sendContent("</networks>");
  this->server->sendContent("</wifi-config>");
  this->server->client().stop(); // Stop is needed because we sent no content length
}

void HttpHandler::handleNotFound() {
  if (this->captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += this->server->uri();
  message += "\nMethod: ";
  message += ( this->server->method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += this->server->args();
  message += "\n";

  for ( uint8_t i = 0; i < this->server->args(); i++ ) {
    message += " " + this->server->argName ( i ) + ": " + this->server->arg ( i ) + "\n";
  }
  this->server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  this->server->sendHeader("Pragma", "no-cache");
  this->server->sendHeader("Expires", "-1");
  this->server->send (404, "text/plain", message);
}