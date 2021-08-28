#include "NixieSettings.h"

NixieSettings::NixieSettings() {
  Serial.println("constructor NixieSettings::NixieSettings");
  this->settings = (SETTINGS*)pvPortMalloc(sizeof(SETTINGS));
  memset(this->settings, 0, sizeof(SETTINGS));
  this->settings->wifiCredential = (WIFI_CREDENTIAL*)pvPortMalloc(sizeof(WIFI_CREDENTIAL));
  memset(this->settings->wifiCredential, 0, sizeof(WIFI_CREDENTIAL));
};

NixieSettings::~NixieSettings() {
  Serial.println("destructor NixieSettings::~NixieSettings");
  Serial.print("sizeof(ssid): ");
  Serial.println(sizeof(this->settings->wifiCredential->ssid));
  Serial.print("sizeof(password): ");
  Serial.println(sizeof(this->settings->wifiCredential->password));
  if(this->settings->wifiCredential != NULL) {
    vPortFree(this->settings->wifiCredential);
  }
  if(this->settings != NULL) {
    vPortFree(this->settings);
  }
  Serial.println("end destructor NixieSettings::~NixieSettings (END)");
};

SETTINGS* NixieSettings::getSettings() {
  this->preferences.begin("NixieSettings", false);
  this->settings->syncNtpDateTime = this->preferences.getBool("syncNtpDateTime", true);
  this->preferences.getString("poolServerName", this->settings->poolServerName, sizeof(char[64]));
  this->preferences.getString("ssid", this->settings->wifiCredential->ssid, sizeof(this->settings->wifiCredential->ssid));
  this->preferences.getString("password", this->settings->wifiCredential->password, sizeof(this->settings->wifiCredential->password));
  this->preferences.end();
  return this->settings;
}


/** Store WLAN credentials to Preference */
bool NixieSettings::setSettings(SETTINGS *settings) {
  /* Storage for SSID and password */
  bool hasError = false;
  Preferences preferences;
  size_t size = 0;
  preferences.begin("NixieSettings", false);
  size = this->preferences.putBool("syncNtpDateTime", this->settings->syncNtpDateTime);
  if (size != sizeof(this->settings->syncNtpDateTime)) {
    hasError = true;
    Serial.println("Sent less data than expected!");
  }
  size = preferences.putString("poolServerName", this->settings->poolServerName);
  if (size != sizeof(this->settings->poolServerName)) {
    hasError = true;
    Serial.println("Sent less data than expected!");
  }
  size = preferences.putString("ssid", this->settings->wifiCredential->ssid);
  if (size != sizeof(this->settings->wifiCredential->ssid)) {
    hasError = true;
    Serial.println("Sent less data than expected!");
  }
  size = preferences.putString("password", this->settings->wifiCredential->password);
  if (size != sizeof(this->settings->wifiCredential->password)) {
    hasError = true;
    Serial.println("Sent less data than expected!");
  }
  preferences.end();
  return hasError;
}