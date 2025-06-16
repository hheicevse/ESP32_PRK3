#include <Arduino.h>
#include <bsp/uart.h>
#include <main.h>
uart1_t uart1;
void IRAM_ATTR uart_isr() {
  digitalWrite(2, HIGH);
  while (Serial1.available()) {
    uart1.rx[uart1.rx_count++] = Serial1.read();
    uart1.rx_timeout=0;
    uart1.rx_flag=TIMER_RUNNING;
  }
}

void uart1_init() {
  Serial1.begin(115200, SERIAL_8N1, 16, 17); // UART1 用 GPIO16, 17
  Serial1.onReceive(uart_isr);
}

void uart1_rx_func(void)
{
    if(uart1.rx_flag==TIMER_FINISH)
    {
      uart1.rx_flag=TIMER_NOTHING;
      if(strcmp((const char*)uart1.rx, "FFF?") == 0){
          Serial1.println("wdqwdw");
      }
      memset(uart1.rx, 0, sizeof(uart1.rx));
      uart1.rx_count=0;
    }
}


void uart0_rx_func(void)
{
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
      //uart傳送  ota,http://192.168.3.152:8000/firmware.bin
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

  }
}