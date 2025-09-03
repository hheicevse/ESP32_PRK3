#include <Arduino.h>
#include <main.h>

void setup() {
  uart0_init();
    // RX 腳上拉
  // pinMode(5, INPUT_PULLUP);
  pinMode(2, OUTPUT);//選告GPIO 2作為輸出（黃色LED）
  digitalWrite(2, HIGH);
  // modbus_init();
  tmr_init();
  wifi_init();
  ble_init();
  html_test_init();//<-task
  ota_http_ca_init();
  watchdog_init();//<-task
  mspm0_esp32_comm_init();//<-task
}

void wifi_check_and_manage_services() {
  static bool wifiWasConnected = false;
  bool wifiNowConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiNowConnected && !wifiWasConnected) {
    Serial.println("[WiFi] services started");
    // ✅ 僅在剛連上網路時執行
    tcp_sta_init();     // TCP Server
    ota_web_init(); //<-task       // OTA Web server
    ota_http_init();
  } 
  else if (!wifiNowConnected && wifiWasConnected) {
    Serial.println("[WiFi] services stopped");
    // ✅ 僅在剛斷線時執行
    tcp_sta_deinit();
    ota_web_deinit();
    ota_http_deinit();
  }
  wifiWasConnected = wifiNowConnected;
}

void loop() {
  uart0_rx_func();//for debug
  uart1_rx_func();//for mspm0
  ble_notify();//斷線會回到廣播模式
  watchdog_func();
  tcp_sta_func();
  tcp_ap_func();
  wifi_check_and_manage_services();
  vTaskDelay(pdMS_TO_TICKS(10));

}