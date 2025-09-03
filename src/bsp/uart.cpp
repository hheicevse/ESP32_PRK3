#include <Arduino.h>
#include <bsp/uart.h>
#include <main.h>
uart0_t uart0;
uart1_t uart1;
/////////////////////////////// uart1 ////////////////////////////////////
void IRAM_ATTR uart1_isr() {
  // digitalWrite(2, HIGH);
  while (Serial1.available()) {
    uart1.rx[uart1.rx_count++] = Serial1.read();
    uart1.rx_timeout=0;
    uart1.rx_flag=TIMER_RUNNING;
  }
}

void uart1_init() {
  // Serial1.begin(115200, SERIAL_8N1, 16, 17); // UART1 用 GPIO16, 17
  Serial1.begin(115200, SERIAL_8N1, 5, 18); // RX=5, TX=18
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
      mspm0_read((const char*)uart1.rx);
      memset(uart1.rx, 0, sizeof(uart1.rx));
      uart1.rx_count=0;
    }
}



/////////////////////////////// uart0 ////////////////////////////////////

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
      debug_read((const char*)uart0.rx);
      memset(uart0.rx, 0, sizeof(uart0.rx));
      uart0.rx_count=0;
    }

}
