#include <Arduino.h>
#include <main.h>


#include "esp_task_wdt.h"
// Watchdog timeout 秒數
#define WDT_TIMEOUT 5
// Task Handle（示意用）
TaskHandle_t watchdogTaskHandle = NULL;

void watchdog_Task(void *parameter) {
  Serial.println("watchdog_Task start");
  // 把自己加入 watchdog 監控
  esp_task_wdt_add(NULL);
  while (true) {
    esp_task_wdt_reset();  // 餵狗
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // 1秒餵一次
  }
}

void watchdog_init() {

  // 初始化 Task Watchdog，啟動自動重啟
  esp_task_wdt_init(WDT_TIMEOUT, true);

  // 把目前的 loop() task 加入 WDT 監控
  esp_task_wdt_add(NULL);

  // 創建一個獨立的 task 來幫忙餵 watchdog
  xTaskCreatePinnedToCore(
    watchdog_Task,        // Task函式
    "watchdog_Task",      // 任務名稱
    1024,                 // stack size
    NULL,                 // 參數
    1,                    // 優先度
    &watchdogTaskHandle,  // task handle
    1                     // 指定在 Core 1 執行
  );
  Serial.println("Setup watchdog");
}

void watchdog_func() {

  // loop task 也要餵狗，確保自己不被重啟
  esp_task_wdt_reset();
}
