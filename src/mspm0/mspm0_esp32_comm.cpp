
#include <Arduino.h>
#include "driver/uart.h"
#include <main.h>
mspm0Comm_t mspm0Comm;

// static bool command_wait_ack(int timeout_ms = 500) {
//     uint8_t rx_buf[64] = {0};
//     int64_t start = esp_timer_get_time(); // us
//     while ((esp_timer_get_time() - start) < timeout_ms * 1000) {
     
//         int len = uart_read_bytes(MSPM0_UART_PORT, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(50));
//         if (len > 0) {
//           rx_buf[len] = '\0';  // 確保結尾為 NULL（但記得 rx_buf 必須比 len 大 1）
//           Serial.printf("[RX] %s\n", (char*)rx_buf);
//         }


//     }
//     return false; // timeout
// }

void polling_task(void *param) {
  while (true) {
    if (mspm0Comm.bsl_triggered) {

      uart1_deinit();
      bsl_func(mspm0Comm.bsl_url.c_str());  // 假設這是阻塞函式
      mspm0Comm.bsl_triggered = false;  // 清除旗標
      // uart_set_baudrate(MSPM0_UART_PORT, 115200);
      
      uart1_deinit();
      uart1_init();
    } 
    else {
      // 一般輪詢邏輯（例如發送資料）
      Serial1.print("WWW?");
      vTaskDelay(pdMS_TO_TICKS(200)); 

      Serial1.print("QQQ?");
      vTaskDelay(pdMS_TO_TICKS(200)); 


    }
    
  }
   vTaskDelay(pdMS_TO_TICKS(1));
}

void mspm0_esp32_comm_init() {
    uart1_init();    
    mspm0Comm.bsl_triggered = false;
    mspm0Comm.bsl_url = "";
    xTaskCreatePinnedToCore(    polling_task,    "polling_task",    4096,    NULL,    2,    NULL,    APP_CPU_NUM   );
}