
#include <Arduino.h>
#include "driver/uart.h"
#include <main.h>

mspm0Comm_t mspm0Comm;

void mspm0_read(const char * rx)
{

    Serial.printf("[RX1] %s", (char*)uart1.rx);
}

void mspm0_polling(void *param) {
  while (true) {
    if (mspm0Comm.bsl_triggered) {
      uart1_deinit();
      bsl_func(mspm0Comm.bsl_url.c_str());  // 假設這是阻塞函式
      mspm0Comm.bsl_triggered = false;  // 清除旗標      
      uart1_deinit();
      uart1_init();
    } 
    else {
      // 一般輪詢邏輯（例如發送資料）
      // Serial1.print("WWW?");
      // vTaskDelay(pdMS_TO_TICKS(200)); 

      // Serial1.print("QQQ?");
      // vTaskDelay(pdMS_TO_TICKS(200)); 


    }
    
   vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void mspm0_esp32_comm_init() {
    uart1_init();    
    mspm0Comm.bsl_triggered = false;
    mspm0Comm.bsl_url = "";
    xTaskCreatePinnedToCore(mspm0_polling,"mspm0_polling",4096, NULL,2,NULL,APP_CPU_NUM);
}