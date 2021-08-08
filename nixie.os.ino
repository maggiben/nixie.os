/**
 * @file         : nixie.os.ino
 * @summary      : Nixie Operating System
 * @version      : 1.0.0
 * @project      : Nixie Clock
 * @description  : A nixie clock operating system 
 * @author       : Benjamin Maggi
 * @email        : benjaminmaggi@gmail.com
 * @date         : 06 Aug 2021
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
 
// Use only core
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

#include <WiFi.h>
#include <NTPClient.h>
// change next line to use with another board/shield
#include <WiFiUdp.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <RTClib.h>
RTC_DS3231 rtc;

const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 25    // modify to the pin we connected
#define DHTTYPE DHT21   // AM2301 
 
DHT_Unified dht(DHTPIN, DHTTYPE);

#include <Adafruit_MCP23X17.h>
#define LED_PIN 0     // MCP23XXX pin LED is attached to
Adafruit_MCP23X17 mcp;

uint32_t delayMS;

const char *ssid     = "WIFI-3532";
const char *password = "12345678";

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// Settings
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
static TickType_t dht_event_delay;
static const uint8_t dht_queue_len = 5;
// Globals
static QueueHandle_t dht_queue = NULL;
static TimerHandle_t dht_event_timer = NULL;
struct DHTEVENT {
  sensors_event_t event;
  String formattedDate;
};

static SemaphoreHandle_t DISPLAYMutex;

void setup(){
  BaseType_t result = pdFALSE;
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  
  if (!mcp.begin_I2C()) {
    Serial.println("MCP initzialization error.");
    while (1);
  }
  // configure pin for output
  mcp.pinMode(0, OUTPUT);
  mcp.pinMode(1, OUTPUT);
  mcp.pinMode(2, OUTPUT);
  mcp.pinMode(3, OUTPUT);
  
  WiFi.begin(ssid, password);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();

  // while(WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print( "." );
  // }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GTM -3 = -10800
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-10800);
  timeClient.begin();
  
  // Initialize device.
  dht.begin();

  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000 / portTICK_PERIOD_MS;

  ntp_datetime_queue = xQueueCreate(ntp_datetime_queue_len, sizeof(DATETIME));

  // Create a one-shot timer
  ntp_sync_timer = xTimerCreate(
    "Sync NTP Date Time",       // Name of timer
    ntp_sync_delay,             // Period of timer (in ticks)
    pdTRUE,                     // Auto-reload
    (void *)0,                  // Timer ID
    syncNtpDateTimeCallback);   // Callback function
  
  // Check to make sure timers were created
  if (ntp_sync_timer == NULL) {
    Serial.println("Could not create ntp_sync_timer");
  } else {
    Serial.println("Starting timers...");
    // Start timers (max block time if command queue is full)
    xTimerStart(ntp_sync_timer, portMAX_DELAY);
  }

  // Start printMessages task
  result = xTaskCreatePinnedToCore(printMessages,
    "Serial Print Service",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);
  
  if(result != pdPASS) {
    Serial.println("Serial Print Service Task creation failed.");
  }

  result = pdFALSE;

  // Start RTC Synctonization with NTP task
  result = xTaskCreatePinnedToCore(syncRtckWithNtp,
    "RTC Synctonization with NTP",
    1536,
    NULL,
    1,
    NULL,
    app_cpu);

  if(result != pdPASS) {
    Serial.println("RTC Synctonization with NTP Task creation failed.");
  }

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void syncNtpDateTimeCallback(TimerHandle_t xTimer) {
  struct DATETIME dateTime;
  while(WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print( "." );
  }
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // Variables to save date and time
  dateTime.epochTime = timeClient.getEpochTime();
  dateTime.formattedDate = timeClient.getFormattedDate();
  int splitT = dateTime.formattedDate.indexOf("T");
  dateTime.dateStamp = dateTime.formattedDate.substring(0, splitT);
  dateTime.timeStamp = dateTime.formattedDate.substring(splitT+1, dateTime.formattedDate.length()-1);
  // Try to add item to queue for 10 ticks, fail if queue is full
  if (xQueueSend(ntp_datetime_queue, (void *)&dateTime, 10) != pdTRUE) {
    Serial.println("ntp_datetime_queue queue full");
  }
}

void syncRtckWithNtp(void *parameters) {
  struct DATETIME dateTime;
  while (true) {
    // See if there's a message in the queue (do not block)
    if (xQueueReceive(ntp_datetime_queue, (void *)&dateTime, 0) == pdTRUE) {
      DateTime now = rtc.now();
      if (dateTime.epochTime != now.unixtime()) {
        Serial.println(dateTime.epochTime);
        Serial.println(now.unixtime());
        rtc.adjust(DateTime(dateTime.epochTime));
      } else {
        Serial.println("RTC and NTP are in sync");
      }
    }
  }
}

// Task: wait for item on queue and print it
void printMessages(void *parameters) {
  while (true) {
    DateTime now = rtc.now();
    Serial.println(now.timestamp());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void loop() {
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
