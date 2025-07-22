#include <Arduino.h>
#include <bsp/uart.h>
#include <main.h>
uart0_t uart0;
uart1_t uart1;

void IRAM_ATTR uart1_isr() {
  // digitalWrite(2, HIGH);
  while (Serial1.available()) {
    uart1.rx[uart1.rx_count++] = Serial1.read();
    uart1.rx_timeout=0;
    uart1.rx_flag=TIMER_RUNNING;
  }
}

void uart1_init() {
  Serial1.begin(115200, SERIAL_8N1, 16, 17); // UART1 用 GPIO16, 17
  Serial1.onReceive(uart1_isr);
}

void uart1_deinit() {
  Serial1.end(); // 關閉 UART1
  Serial1.onReceive(NULL); // 移除接收回呼
  memset(uart1.rx, 0, sizeof(uart1.rx));
  uart1.rx_count=0;
  uart1.rx_flag=TIMER_NOTHING;
}

void uart1_rx_func(void)
{
    if(uart1.rx_flag==TIMER_FINISH)
    {
      uart1.rx_flag=TIMER_NOTHING;
      // if(strcmp((const char*)uart1.rx, "FFF?") == 0){
      //     Serial1.println("wdqwdw");
          
      // }
      mspm0_read((const char*)uart1.rx);

      memset(uart1.rx, 0, sizeof(uart1.rx));
      uart1.rx_count=0;
    }
}





void IRAM_ATTR uart0_isr() {
  while (Serial.available()) {
    uart0.rx[uart0.rx_count++] = Serial.read();
    uart0.rx_timeout=0;
    uart0.rx_flag=TIMER_RUNNING;
  }
}

void uart0_init() {
  Serial.begin(115200);
  Serial.onReceive(uart0_isr);
}

void uart0_deinit() {
  Serial.end(); // 關閉 UART
  Serial.onReceive(NULL); // 移除接收回呼
  memset(uart0.rx, 0, sizeof(uart0.rx));
  uart0.rx_count=0;
  uart0.rx_flag=TIMER_NOTHING;
}
void uart0_rx_func() {
 
    if(uart0.rx_flag==TIMER_FINISH)
    {
      uart0.rx_flag=TIMER_NOTHING;

      // ✅ 把 rx buffer 轉成 String
      String input = String((char*)uart0.rx);
      input.trim();  // 去掉前後空白和換行符

      // ✅ 拆欄位
      int firstComma = input.indexOf(',');
      int secondComma = input.indexOf(',', firstComma + 1);
      int thirdComma = input.indexOf(',', secondComma + 1);  // optional

      String cmd = input.substring(0, firstComma);
      // uart傳送  ota_ca,https://192.168.3.152:443/firmware.bin
      // PC端要執行 ota_server_ca.py
      if (cmd == "ota_ca" && firstComma != -1) {
        String url = input.substring(firstComma + 1);  // URL 是第2欄
        Serial.println("[RX0] ota_ca start with URL: " + url);
        ota_http_CA_func(url.c_str());  // 把 URL 當參數傳給 ota_http_CA_func
      }

      // uart傳送  ota,http://192.168.3.152:8000/firmware.bin
      // 然後 PC 端要打 python -m http.server 8000
      else if (cmd == "ota" && firstComma != -1) {
        String url = input.substring(firstComma + 1);  // URL 是第2欄
        Serial.println("[RX0] ota start with URL: " + url);
        ota_http_func(url.c_str());  // 把 URL 當參數傳給 ota_http_func
      }

      // uart傳送 SB_V2,http://192.168.3.152:8000/firmware.bin,http://192.168.3.152:8000/firmware.sig
      // 然後 PC 端要打 python -m http.server 8000
      else if (cmd == "SB_V2" && secondComma != -1) {
        String bin_url = input.substring(firstComma + 1, secondComma);   // 第2欄
        String sig_url = input.substring(secondComma + 1);               // 第3欄
        Serial.println("[RX0] uart simulation_Secure_Boot V2 start");
        Serial.println("[RX0] firmware URL: " + bin_url);
        Serial.println("[RX0] signature URL: " + sig_url);
        simulation_Secure_Boot_func(bin_url.c_str(), sig_url.c_str());
      }

      // uart傳送  bsl_mspm0,http://192.168.3.153:8000/app2.txt
      // 然後 PC 端要打 python -m http.server 8000
      else if (cmd == "bsl_mspm0" && firstComma != -1) {
        String url = input.substring(firstComma + 1);  // URL 是第2欄
        Serial.println("[RX0] bsl_mspm0 start with URL: " + url);
        // bsl_func(url.c_str());  // 把 URL 當參數傳給 bsl_func
        // 寫入共享變數給 task
        mspm0Comm.bsl_url = url;
        mspm0Comm.bsl_triggered = true;
      }

      // uart傳送  tcp_connect,192.168.3.153,502
      else if (cmd == "tcp_connect" && secondComma != -1) {
        String ip = input.substring(firstComma + 1, secondComma);  // 第2欄 IP
        String port = input.substring(secondComma + 1);            // 第3欄 port
        tcp_connect(ip.c_str(), port.toInt());
      }
      // uart傳送  tcp_disconnect
      else if (cmd == "tcp_disconnect") {
        tcp_disconnect();
      }
      // uart傳送  tcp_sent
      else if (cmd == "tcp_sent") {
        const uint8_t tx[] = {0x01, 0x03, 0x35, 0x00, 0x00, 0x01};  // 你自定封包
        tcp_send_data(tx, sizeof(tx));
      }
      // uart傳送  tcp_read
      else if (cmd == "tcp_read") {
        // 改用 non-blocking tcp_available() 檢查
        int len = tcp_available();
        if (len > 0) {
          uint8_t buf[128];
          int r = tcp_receive_data(buf, len);
        }
      }

      else {
        Serial.println("[RX0] Unknown or malformed command: " + input);
      }

      memset(uart0.rx, 0, sizeof(uart0.rx));
      uart0.rx_count=0;
    }

}
