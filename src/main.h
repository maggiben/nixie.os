/**
 * @file         : main.h
 * @summary      : Nixie Operating System
 * @version      : 1.0.0
 * @project      : Nixie Clock
 * @description  : A nixie clock operating system 
 * @author       : Benjamin Maggi
 * @email        : benjaminmaggi@gmail.com
 * @date         : 08 Aug 2021
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

#include <FreeRTOS.h>
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <RTClib.h>
RTC_DS3231 rtc;

#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 25       // modify to the pin we connected
#define DHTTYPE DHT21   // AM2301 
DHT_Unified dht(DHTPIN, DHTTYPE);

#include <Adafruit_MCP23017.h>
Adafruit_MCP23017 mcp;

#include <ESP32Time.h>

ESP32Time esp32Time;
// Use only core
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

struct DHTSENSORDATA {
  float temperature;        // temperature is in degrees centigrade (Celsius)
  float relative_humidity;  // relative humidity in percent
  long timestamp;           // measurment timestamp
};

// Functions
void printDhtSensorData();
void syncNtpDateTimeCallback(TimerHandle_t xTimer);
void syncDhtSensorCallback(TimerHandle_t xTimer);
void syncRtckWithNtp(void *parameters);
void printMessages(void *parameters);
void displayTimeStamp(DHTSENSORDATA *dhtSensorData, int16_t x, int16_t y, uint16_t color);
void displayMessages(void *parameters);
void setEsp32Time();
void testOutput(void *parameters);
void nixieTime();

// Settings
static const TickType_t ntp_sync_delay = 5000 / portTICK_PERIOD_MS;
static const uint8_t ntp_datetime_queue_len = 5;
// Globals
static QueueHandle_t ntp_datetime_queue = NULL;
static TimerHandle_t ntp_sync_timer = NULL;
struct DATETIME {
  String formattedDate;
  String dateStamp;
  String timeStamp;
  unsigned long epochTime;
};

// Settings
static const uint8_t dht_queue_len = 5;
// Globals
static QueueHandle_t dht_queue = NULL;
static TimerHandle_t dht_event_timer = NULL;
static SemaphoreHandle_t i2c_mutex;
