#include <Arduino.h>
#include <main.h>



void setup() {
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
  // watchdog_init();//<-task

// bsl_func("bsl_mspm0,http://192.168.3.153:8000/app2.txt");
}

void loop() {
  uart0_rx_func();
  uart1_rx_func();
  ble_notify();//斷線會回到廣播模式
  // watchdog_func();
  vTaskDelay(pdMS_TO_TICKS(10));

  
}