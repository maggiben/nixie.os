
/**
 * @file         : Nixie.h
 * @summary      : Nixie handler based on the 74141 driver and the MCP23017 i/o expander
 * @version      : 1.0.1
 * @project      : Nixie Clock
 * @description  : Nixie handler based on the 74141 driver and the MCP23017 i/o expander
 * @author       : Benjamin Maggi
 * @email        : benjaminmaggi@gmail.com
 * @date         : 18 Oct 2021
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
#include <Adafruit_MCP23017.h>
#include <ESP32Time.h>
#include <RTClib.h>
#include "Arduino.h"

class Nixie {
  private:
    long screenSaverInterval = 600; // time in seconds between anti anode poisoning routine
    Adafruit_MCP23017 mcp;
    RTC_DS3231 rtc;
    ESP32Time esp32Time;
    SemaphoreHandle_t i2cMutex;
    void screenSaver(uint8_t number);
    void screenSaverRandom();
    void screenSaverSlot();
    void nixieTime();
  public:
    Nixie() {};
    void setup(SemaphoreHandle_t i2cMutex);
    void nixieTask();
};
