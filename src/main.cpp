/**
 * @file         : main.cpp
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
#include "main.h"
#include "DateTime.h"

#include <WiFi.h>
#include <NTPClient.h>
// change next line to use with another board/shield
#include <WiFiUdp.h>

const char *ssid     = "WIFI-3532";
const char *password = "12345678";

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

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

  setEsp32Time();

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  
  mcp.begin();
  // configure pin for output
  mcp.pinMode(0, OUTPUT);
  mcp.pinMode(1, OUTPUT);
  mcp.pinMode(2, OUTPUT);
  mcp.pinMode(3, OUTPUT);
  
  WiFi.begin(ssid, password);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  } else {
    // Create mutex before starting tasks
    display_mutex = xSemaphoreCreateMutex();
    if (display_mutex == NULL) {
      Serial.println("Error insufficient heap memory to create display_mutex mutex");
    } else {
      display.clearDisplay();
    }
  }

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
  sensor_t sensor;
  // Dumy read to get sensor info
  dht.humidity().getSensor(&sensor);
  // Set delay between sensor readings based on sensor details.
  const TickType_t dht_sense_interval = sensor.min_delay / 1000 / portTICK_PERIOD_MS;
  printDhtSensorData();

  ntp_datetime_queue = xQueueCreate(ntp_datetime_queue_len, sizeof(DATETIME));
  dht_queue = xQueueCreate(dht_queue_len, sizeof(DHTSENSORDATA));

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

  // Create DHT sesor sync timer
  dht_event_timer = xTimerCreate(
    "Read DHT Sensor",            // Name of timer
    dht_sense_interval,           // Set delay between sensor readings based on sensor details.
    pdTRUE,                       // Auto-reload
    (void *)1,                    // Timer ID
    syncDhtSensorCallback);       // Callback function

  if (dht_event_timer == NULL) {
    Serial.println("Could not create dht_event_timer");
  } else {
    Serial.println("Starting timers dht_event_timer...");
    // Start timers (max block time if command queue is full)
    xTimerStart(dht_event_timer, portMAX_DELAY);
  }

  // Start printMessages task
  result = xTaskCreatePinnedToCore(printMessages,
    "Serial Print Service",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);
  
  if (result != pdPASS) {
    Serial.println("Serial Print Service Task creation failed.");
  }

    // Start printMessages task
  result = xTaskCreatePinnedToCore(displayMessages,
    "Display Print Service",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);
  
  if (result != pdPASS) {
    Serial.println("Display Print Service Task creation failed.");
  }

  // Start RTC Synctonization with NTP task
  result = xTaskCreatePinnedToCore(syncRtckWithNtp,
    "RTC Synctonization with NTP",
    1536,
    NULL,
    1,
    NULL,
    app_cpu);

  if (result != pdPASS) {
    Serial.println("RTC Synctonization with NTP Task creation failed.");
  }

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void printDhtSensorData() {
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
}

void syncNtpDateTimeCallback(TimerHandle_t xTimer) {
  struct DATETIME dateTime;
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print( "." );
  }
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // Variables to save date and time
  dateTime.epochTime = timeClient.getEpochTime();
  dateTime.formattedDate = getFormattedDate(&timeClient);
  int splitT = dateTime.formattedDate.indexOf("T");
  dateTime.dateStamp = dateTime.formattedDate.substring(0, splitT);
  dateTime.timeStamp = dateTime.formattedDate.substring(splitT+1, dateTime.formattedDate.length()-1);
  // Try to add item to queue for 10 ticks, fail if queue is full
  if (xQueueSend(ntp_datetime_queue, (void *)&dateTime, 10) != pdTRUE) {
    Serial.println("ntp_datetime_queue queue full");
  }
}

void syncDhtSensorCallback(TimerHandle_t xTimer) {
  struct DHTSENSORDATA dhtSensorData;
  sensors_event_t event;
  // Get temperature event
  dht.temperature().getEvent(&event);
  dhtSensorData.temperature = event.temperature;
  // Get humidity event
  dht.humidity().getEvent(&event);
  dhtSensorData.relative_humidity = event.relative_humidity;

  // Test if sensor data is valid
  if (isnan(dhtSensorData.temperature) || isnan(dhtSensorData.relative_humidity)) {
    Serial.println(F("Error reading temperature or humidity!"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(dhtSensorData.temperature);
    Serial.println(F("°C"));

    Serial.print(F("Humidity: "));
    Serial.print(dhtSensorData.relative_humidity);
    Serial.println(F("%"));

    // Try to add item to queue for 10 ticks, fail if queue is full
    if (xQueueSend(dht_queue, (void *)&dhtSensorData, 10) != pdTRUE) {
      Serial.println("dht_queue queue full");
    }
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
        // Adjust battery backup rtc
        rtc.adjust(DateTime(dateTime.epochTime));
        // Adjust internal rtc
        esp32Time.setTime(dateTime.epochTime);
      } else {
        Serial.println("RTC and NTP are in sync");
      }
    }
  }
}

void setEsp32Time() {
  // Get battery backup rtc
  DateTime now = rtc.now();
  // Adjust internal rtc
  esp32Time.setTime(now.unixtime());
}

// Task: wait for item on queue and print it
void printMessages(void *parameters) {
  while (true) {
    Serial.println(esp32Time.getDateTime(true));
    // Print out number of free heap memory bytes before malloc
    // Serial.print("Heap size (bytes): ");
    // Serial.println(xPortGetFreeHeapSize());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void displayTimeStamp(int16_t x, int16_t y, uint16_t color) {
  struct DHTSENSORDATA dhtSensorData;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(color);

  if (xQueueReceive(dht_queue, (void *)&dhtSensorData, 0) == pdTRUE) {
    display.setCursor(x, y);
    display.print(F("Temperature: "));
    display.print(dhtSensorData.temperature);
    display.println(F("*C"));
    display.display();

    display.setCursor(x, y + 10);
    display.print(F("Humidity: "));
    display.print(dhtSensorData.relative_humidity);
    display.println(F("%"));
    display.display(); 
    
    display.setCursor(x, y + 20);
    display.print(F("Date: "));
    display.print(esp32Time.getDateTime(true));
    display.println();
    display.display();
  }
}

void displayMessages(void *parameters) {
  while (true) {
    if (xSemaphoreTake(display_mutex, portMAX_DELAY)) {
      displayTimeStamp(0, 0, WHITE);
      xSemaphoreGive(display_mutex);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void loop() {
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
