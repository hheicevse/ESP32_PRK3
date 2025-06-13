#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <NimBLEDevice.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WiFiMulti.h>

WebServer ota_web(80);
HTTPClient ota_http;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);     //啟動序列通訊鮑率115200

  // RX 腳上拉
  pinMode(16, INPUT_PULLUP);

  pinMode(2, OUTPUT);//選告GPIO 2作為輸出（黃色LED）

  tmr_init();
  uart1_init();
  wifi_init();
  ble_init();
  ota_web_init();
  ota_http_init();
  ota_http_ca_init();
  
  
  // rtos
  xTaskCreatePinnedToCore(ota_web_Task, "ota_web_Task", 4096, NULL, 1, NULL, 1);

}

void loop() {
  uart0_rx_func();
  uart1_rx_func();
  ble_notify();//斷線會回到廣播模式



  vTaskDelay(pdMS_TO_TICKS(10));
}