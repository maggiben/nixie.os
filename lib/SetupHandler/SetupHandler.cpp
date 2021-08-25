#include "SetupHandler.h"

// Use only core
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

DNSServer dnsServer;
WebServer server(80);

const char *softAP_ssid = "nixie";
const char *softAP_password = "12345678";

/* hostname for mDNS. Should work at least on windows. Try http://nixie.local */
const char *myHostname = "nixie";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";

/* Soft AP network parameters */
IPAddress apIP(192, 168, 0, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;

HttpHandler httpHandler(&server, myHostname, &apIP, softAP_ssid);



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
  uint64_t freeSize = cardSize - (SD.usedBytes() / (1024 * 1024));
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("SD Free Size: %lluMB\n", freeSize);
}

void setupHanlder() {
  BaseType_t result = pdFALSE;
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

  httpHandler.begin();

  Serial.println("HTTP server started");
  WIFI_CREDENTIAL *wifiCredential = loadCredentials();
  Serial.print("preferences ssid: ");
  Serial.println(wifiCredential->ssid);
  Serial.print("preferences password: ");
  Serial.println(wifiCredential->password);
  
  connect = strlen(wifiCredential->ssid) > 0; // Request WLAN connect if there is a SSID
  vPortFree(wifiCredential);

  Serial.print("Connect: ");
  Serial.println(connect);

  // Start AP Captive Portal and Wifi Setup task
  result = xTaskCreatePinnedToCore(handleApRequestTask,
    "AP Captive Portal and Wifi Setup",
    2560,
    NULL,
    2,
    NULL,
    app_cpu);

  if (result != pdPASS) {
    Serial.println("AP Captive Portal and Wifi Setup Task creation failed.");
  }
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
