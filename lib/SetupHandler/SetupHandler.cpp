#include "SetupHandler.h"

DNSServer dnsServer;
WebServer server(80);

const char *softAP_ssid = "nixie";
const char *softAP_password = "12345678";

/* hostname for mDNS. Should work at least on windows. Try http://nixie.local */
const char *myHostname = "nixie";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";

/* Storage for SSID and password */
Preferences preferences;

/* Soft AP network parameters */
IPAddress apIP(192, 168, 0, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;

HttpHandler httpHandler(&server, myHostname);

/** Load WLAN credentials from Preferences */
void loadCredentials() {
  preferences.getString("ssid", ssid, sizeof(ssid));
  preferences.getString("password", password, sizeof(password));
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password)>0?"********":"<no password>");
}

/** Store WLAN credentials to Preference */
void saveCredentials() {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  Serial.println("Saved credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password)>0?"********":"<no password>");
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  Serial.print("hostHeader: ");
  Serial.println(server.hostHeader());
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  fs::FS &fs = SD;
  String path = server.uri(); //saves the to a string server uri ex.(192.168.100.110/edit server uri is "/edit")
  Serial.print("path ");  Serial.println(path);

  //To send the index.html when the serves uri is "/"
  if (path.endsWith("/")) {
    path += "index.html";
  }
  String contentType = getContentType(&server, path);
  Serial.print("contentType ");
  Serial.println(contentType);
  File file = fs.open(path, "r"); //Open the File with file name = to path with intention to read it. For other modes see <a href="https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html" style="font-size: 13.5px;"> https://arduino-esp8266.readthedocs.io/en/latest/...</a>
  size_t sent = server.streamFile(file, contentType); //sends the file to the server references from <a href="https://github.com/espressif/arduino-esp32/blob/master/libraries/WebServer/src/WebServer.h" style="font-size: 13.5px;"> https://arduino-esp8266.readthedocs.io/en/latest/...</a>
  if (sent != file.size()) {
    Serial.println("Sent less data than expected!");
  }
  Serial.print("sent: ");
  Serial.println(sent);
  file.close(); //Close the file

  // server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  // server.sendHeader("Pragma", "no-cache");
  // server.sendHeader("Expires", "-1");
  // server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  // server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  // server.sendContent(
  //   "<html><head></head><body>"
  //   "<h1>HELLO WORLD!!</h1>"
  // );
  // if (server.client().localIP() == apIP) {
  //   server.sendContent(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  // } else {
  //   server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  // }
  // server.sendContent(
  //   "<p>You may want to <a href='/wifi'>config the wifi connection</a>.</p>"
  //   "</body></html>"
  // );
  // server.client().stop(); // Stop is needed because we sent no content length
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}

void initSDCard() {
  if(!SD.begin(SS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if(cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void setupHanlder() {
  preferences.begin("CapPortAdv", false);
  Serial.print("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  vTaskDelay(500 / portTICK_PERIOD_MS);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  if (!MDNS.begin(myHostname))  {
    Serial.println("Error setting up MDNS responder!");
  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(myHostname);
    Serial.println(".local");
  }
  
  initSDCard();

  // HttpHandler httpHandler(&server, myHostname);

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  // server.on("/", handleRoot);
  // server.on("/", []() {
  //   return httpHandler.handleRoot();
  // });
  // server.serveStatic("/", SD, "/");
  // server.on("/inline", []() {
  //   server.send(200, "text/plain", "this works as well");
  // });
  // server.onNotFound([]() {
  //   return httpHandler.handleNotFound();
  // });
  // server.begin(); // Web server start
  
  httpHandler.begin();

  Serial.println("HTTP server started");
  loadCredentials(); // Load WLAN credentials from network
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
  Serial.print("Connect: ");
  Serial.println(connect);
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin ( ssid, password );
  int connRes = WiFi.waitForConnectResult();
  Serial.print("connRes: ");
  Serial.println ( connRes );
}

void handleApRequestTask(void *parameters) {
  /** Current WLAN status */
  wl_status_t oldWifiStatus = WL_IDLE_STATUS;
  while(true) {
    // if (connect) {
    //   Serial.println("Connect requested");
    //   connect = false;
    //   connectWifi();
    //   lastConnectTry = millis();
    // }
    wl_status_t newWifiStatus = WiFi.status();

    // if (s == 0 && millis() > (lastConnectTry + 60000) ) {
    //   /* If WLAN disconnected and idle try to connect */
    //   /* Don't set retry time too low as retry interfere the softAP operation */
    //   connect = true;
    // }
    // if (status != s) { // WLAN status change
    //   Serial.print ("Status: ");
    //   Serial.println (s);
    //   status = s;
    //   if (s == WL_CONNECTED) {
    //     /* Just connected to WLAN */
    //     Serial.println ();
    //     Serial.print ("Connected to ");
    //     Serial.println(ssid);
    //     Serial.print ("IP address: ");
    //     Serial.println (WiFi.localIP());

    //     // Setup MDNS responder
    //     if (!MDNS.begin(myHostname)) {
    //       Serial.println("Error setting up MDNS responder!");
    //     } else {
    //       Serial.println("mDNS responder started");
    //       // Add service to MDNS-SD
    //       MDNS.addService("http", "tcp", 80);
    //     }
    //   } else if (s == WL_NO_SSID_AVAIL) {
    //     WiFi.disconnect();
    //   }
    // }
    if (oldWifiStatus != newWifiStatus) { // WLAN Status Change
      Serial.print("Status: ");
      Serial.println(newWifiStatus);
      oldWifiStatus = newWifiStatus;
    }
    // Do work:
    //DNS
    dnsServer.processNextRequest();
    //HTTP
    server.handleClient();
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}
