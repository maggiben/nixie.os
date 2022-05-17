
/**
 * @file         : Nixie.cpp
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

#include "Nixie.h"

void Nixie::setup(SemaphoreHandle_t i2cMutex) {
  // i2c mutex
  this->i2cMutex = i2cMutex;
  // init rtc
  if (!this->rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  // Get battery backup rtc
  DateTime now = this->rtc.now();
  // Adjust internal rtc
  this->esp32Time.setTime(now.unixtime());

  // Print time
  Serial.print("Time: ");
  Serial.println(this->esp32Time.getDateTime(true));

  // handle rtc battery backup power lost
  if (this->rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    this->rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  this->mcp.begin();
  // 74141 BCD decoder on PORTA
  this->mcp.pinMode(0, OUTPUT);
  this->mcp.pinMode(1, OUTPUT);
  this->mcp.pinMode(2, OUTPUT);
  this->mcp.pinMode(3, OUTPUT);
  // anodes drivers on PORTB
  this->mcp.pinMode(8, OUTPUT);
  this->mcp.pinMode(9, OUTPUT);
  this->mcp.pinMode(10, OUTPUT);
  this->mcp.pinMode(11, OUTPUT);
  this->mcp.pinMode(12, OUTPUT);
  this->mcp.pinMode(13, OUTPUT);
}

void Nixie::screenSaver(uint8_t number) {
  uint8_t anodes[] = {1, 2, 4, 8, 16, 32};
  for(uint8_t i = 0; i < sizeof(anodes); i++) {
    uint8_t anode = anodes[i];
    if (xSemaphoreTake(this->i2cMutex, portMAX_DELAY) == pdTRUE) {
      uint16_t output = (anode << 8) | (number << 0);
      mcp.writeGPIOAB(output);
      xSemaphoreGive(this->i2cMutex);
    }
    vTaskDelay(3 / portTICK_PERIOD_MS);
  }
}

void Nixie::screenSaverRandom() {
  int epoch = esp32Time.getEpoch();
  bool expired = false;
  while(!expired) {
    uint8_t number = random(9);
    this->screenSaver(number);
    expired = esp32Time.getEpoch() > epoch + 5 ? true : false;
  }
}

void Nixie::screenSaverSlot() {
  for(uint8_t number = 0; number <= 9; number++) {
    bool expired = false;
    int epoch = esp32Time.getEpoch();
    while(!expired) {
      this->screenSaver(number);
      expired = esp32Time.getEpoch() > epoch + 2 ? true : false;
    }
  }
}

void Nixie::nixieTime() {
  uint8_t anodes[] = {1, 2, 4, 8, 16, 32};
  char time[6] = { 0, 0, 0, 0, 0, 0 };
  sprintf(&time[0], "%02d", this->esp32Time.getHour());
  sprintf(&time[2], "%02d", this->esp32Time.getMinute());
  sprintf(&time[4], "%02d", this->esp32Time.getSecond());
  // reverse string
  for(uint8_t i = 0; i < sizeof(time) / 2; i++) {
    char temp = time[i];
    time[i] = time[sizeof(time) - i - 1];
    time[sizeof(time) - i - 1] = temp;
  }
  // loop though anodes
  for(uint8_t i = 0; i < sizeof(anodes); i++) {
    uint8_t anode = anodes[i];
    if (xSemaphoreTake(this->i2cMutex, portMAX_DELAY) == pdTRUE) {
      // get current digit
      char digit[2] = { time[i], '\0' };
      uint8_t number = atoi(digit);
      // join port A & port B in a single 16 bit number
      uint16_t output = (anode << 8) | (number << 0);
      // blank all
      mcp.writeGPIOAB(0);
      // light up the nixie [anode]-[number]
      mcp.writeGPIOAB(output);
      xSemaphoreGive(this->i2cMutex);
    }
    vTaskDelay(3 / portTICK_PERIOD_MS);
  }
}

void Nixie::nixieTask() {
  bool expired = false;
  long epoch = this->esp32Time.getEpoch();
  while(true) {
    while(!expired) {
      this->nixieTime();
      expired = this->esp32Time.getEpoch() > (epoch + this->screenSaverInterval) ? true : false;
    }
    this->screenSaverRandom();
    this->screenSaverSlot();
    expired = false;
    epoch = this->esp32Time.getEpoch();
  }
}