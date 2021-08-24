#include "httpHandler.h"

HttpHandler::HttpHandler(WebServer *server, const char *hostname) {
  this->server = server;
  this->hostname = hostname;

/* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  this->server->on("/", [this]() {
    return this->handleRoot();
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