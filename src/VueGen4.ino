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

  if (input.startsWith("ota_ca,")) {  
    // uart傳送  ota_ca,https://192.168.3.152:443/firmware.bin
    // PC端要執行ota_server_ca.py
    String url = input.substring(7);  // 取得逗號後的字串 (從第7個字元開始)
    Serial.println("ota_ca start with URL: " + url);
    ota_http_CA_func(url.c_str());  // 把 URL 當參數傳給 ota_http_CA_func
  }
  else if (input.startsWith("ota,")) {  
    //uart傳送  ota,http://192.168.3.153:8000/firmware.bin  
    // 然後PC端要打python -m http.server 8000
    String url = input.substring(4);  // 取得逗號後的字串 (從第7個字元開始)
    Serial.println("ota start with URL: " + url);
    ota_http_func(url.c_str());  // 把 URL 當參數傳給 ota_http_CA_func
  }

  else if (input.startsWith("SB_V2,")) {
    Serial.println("uart simulation_Secure_Boot V2 start");
    // uart傳送 SB_V2,http://192.168.3.152:8000/firmware.bin,http://192.168.3.152:8000/firmware.sig
    // 然後PC端要打python -m http.server 8000
    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);
    if (firstComma != -1 && secondComma != -1) {
      String bin_url = input.substring(firstComma + 1, secondComma);
      String sig_url = input.substring(secondComma + 1);
      Serial.println("firmware URL: " + bin_url);
      Serial.println("signature URL: " + sig_url);
      simulation_Secure_Boot_func(bin_url.c_str(), sig_url.c_str());  // 傳成 const char* 如果你的函式定義是這樣
    } else {
      Serial.println("format failed, please use SB_V2,firmware_url,signature_url");
    }
  }




    // else if (input == "ota"){
    //   Serial.println("uart ota start");
    //   // 然後PC端要打python -m http.server 8000
    //   ota_http_func("http://192.168.3.153:8000/firmware.bin");
    // }
    // else if (input == "SB_V2"){
    //   Serial.println("uart simulation_Secure_Boot V2 start");
    //   // 然後PC端要打python -m http.server 8000
    //   simulation_Secure_Boot_func("http://192.168.3.152:8000/firmware.bin","http://192.168.3.152:8000/firmware.sig");
    // }
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}