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
 
#include "main.h"

void setup() {
  BaseType_t result = pdFALSE;
  Serial.begin(115200);
  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  i2c_mutex = xSemaphoreCreateMutex();
  if (i2c_mutex == NULL) {
    Serial.println("Error insufficient heap memory to create i2c_mutex mutex");
  }

  nixie.setup(i2c_mutex);

  ntp_datetime_queue = xQueueCreate(ntp_datetime_queue_len, sizeof(DATETIME));

  // Create a one-shot timer
  // ntp_sync_timer = xTimerCreate(
  //   "Sync NTP Date Time",       // Name of timer
  //   ntp_sync_delay,             // Period of timer (in ticks)
  //   pdTRUE,                     // Auto-reload
  //   (void *)0,                  // Timer ID
  //   syncNtpDateTimeCallback);   // Callback function
  
  // // Check to make sure timers were created
  // if (ntp_sync_timer == NULL) {
  //   Serial.println("Could not create ntp_sync_timer");
  // } else {
  //   Serial.println("Starting timer ntp_sync_timer...");
  //   // Start timers (max block time if command queue is full)
  //   xTimerStart(ntp_sync_timer, portMAX_DELAY);
  // }

  // Start printMessages task
  // result = xTaskCreatePinnedToCore(printMessages,
  //   "Serial Print Service",
  //   1024,
  //   NULL,
  //   1,
  //   NULL,
  //   app_cpu);
  
  // if (result != pdPASS) {
  //   Serial.println("Serial Print Service Task creation failed.");
  // }


  // Start RTC Synctonization with NTP task
  // result = xTaskCreatePinnedToCore(syncRtckWithNtp,
  //   "RTC Synctonization with NTP",
  //   1664,
  //   NULL,
  //   tskIDLE_PRIORITY,
  //   NULL,
  //   app_cpu);

  // if (result != pdPASS) {
  //   Serial.println("RTC Synctonization with NTP Task creation failed.");
  // }

  // Start RTC Synctonization with NTP task
  result = xTaskCreatePinnedToCore(nixieTask,
    "Nixie Task",
    1664,
    NULL,
    1,
    NULL,
    app_cpu);

  if (result != pdPASS) {
    Serial.println("Nixie task creation failed.");
  }

  // Setup AP captive portal and wifi setup
  setupHanlder();

  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GTM -3 = -10800
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-10800);
  timeClient.begin();

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void syncNtpDateTimeCallback(TimerHandle_t xTimer) {
  struct DATETIME dateTime;
  if(WiFi.status() != WL_CONNECTED) {
    Serial.print( "Wifi not connected, status: ");
    Serial.println(WiFi.status());
    return;
  }
  if(!timeClient.update()) {
    Serial.println("failed to update npm");
    bool forced = timeClient.forceUpdate();
    if(!forced) {
      Serial.println("failed to force npm update");
      return;
    }
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

void syncRtckWithNtp(void *parameters) {
  struct DATETIME dateTime;
  while (true) {
    // See if there's a message in the queue (do not block)
    if (xQueueReceive(ntp_datetime_queue, (void *)&dateTime, 0) == pdTRUE) {
      if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
        DateTime now = rtc.now();
        if (dateTime.epochTime != now.unixtime()) {
          // // Adjust internal rtc
          esp32Time.setTime(dateTime.epochTime);
          if (dateTime.epochTime != esp32Time.getEpoch()) {
            Serial.println("Failed to sync internal RTC clock");
          }
          // Adjust battery backup rtc
          rtc.adjust(DateTime(dateTime.epochTime));
          now = rtc.now();
          if (dateTime.epochTime != now.unixtime()) {
            Serial.println("Failed to sync external RTC clock");
          }
        } else {
          Serial.println("RTC clocks are in sync with NTP");
        }
        xSemaphoreGive(i2c_mutex);
      }
    }
  }
}

void loop() {
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void nixieTask(void *parameters) {
  while(true) {
    nixie.nixieTask();
  }
}
