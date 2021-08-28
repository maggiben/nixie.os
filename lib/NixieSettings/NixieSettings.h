/**
 * @file         : Settings.h
 * @summary      : Nixie settings handler
 * @version      : 1.0.0
 * @project      : Nixie Clock
 * @description  : Nixie settings handler
 * @author       : Benjamin Maggi
 * @email        : benjaminmaggi@gmail.com
 * @date         : 27 Aug 2021
 * @license:     : MIT
 *
 * Copyright 2021 Benjamin Maggi <benjaminmaggi@gmail.com>
 *
 *
 * License:
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **/

#pragma once
#include <Preferences.h>
#include "utils.h"

struct WIFI_CREDENTIAL {
  /* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
  char ssid[32];
  char password[32];
};

struct SETTINGS {
  bool syncNtpDateTime;
  char* poolServerName;
  char* alarm;
  WIFI_CREDENTIAL* wifiCredential = NULL;
};


class NixieSettings {
  private:
    Preferences preferences;
    SETTINGS* settings = NULL;
  public:
    NixieSettings();
    virtual ~NixieSettings();
    SETTINGS* getSettings();
    boolean setSettings(SETTINGS* settings);
};
