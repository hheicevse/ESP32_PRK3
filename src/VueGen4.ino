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
  uart1_rx_func();
  ble_notify();//斷線會回到廣播模式


  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // 讀到換行為止
    input.trim();  // 去除前後空白或換行

    if (input == "ota ca") {
      Serial.println("uart ota ca start");
      // PC端要執行ota_server_ca.py
      ota_http_CA_func("https://192.168.3.153:443/firmware.bin");
    } 
    else if (input == "ota"){
      Serial.println("uart ota start");
      // 然後PC端要打python -m http.server 8000
      ota_http_func("http://192.168.3.153:8000/firmware.bin");
    }
    else if (input == "SB_V2"){
      Serial.println("uart simulation_Secure_Boot V2 start");
      // 然後PC端要打python -m http.server 8000
      simulation_Secure_Boot_func("http://192.168.3.152:8000/firmware.bin","http://192.168.3.152:8000/firmware.sig");
    }
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}