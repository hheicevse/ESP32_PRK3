#include <Arduino.h>
#include <main.h>

// #include "nvs_flash.h"
// #include <NimBLEDevice.h>
void setup() {
  // esp_log_level_set("*", ESP_LOG_INFO);  // 所有 tag 都印到 INFO
  // put your setup code here, to run once:
  Serial.begin(115200);     //啟動序列通訊鮑率115200

  // RX 腳上拉
  pinMode(16, INPUT_PULLUP);
  pinMode(2, OUTPUT);//選告GPIO 2作為輸出（黃色LED）

  // modbus_init();
  tmr_init();
  uart1_init();
  wifi_init();
  ble_init();
  html_test_init();
  ota_web_init();//<-task
  ota_http_init();
  ota_http_ca_init();
  watchdog_init();//<-task

  bsd_socket_init();

  // NimBLEDevice::deleteAllBonds(); 
  // nvs_flash_erase();  // 清除整個 NVS 區塊
  // nvs_flash_init();
  // NimBLEDevice::init("MyBLE");

}

void loop() {
  uart0_rx_func();
  uart1_rx_func();
  ble_notify();//斷線會回到廣播模式
  watchdog_func();
  bsd_socket_func();
  vTaskDelay(pdMS_TO_TICKS(10));

  
}