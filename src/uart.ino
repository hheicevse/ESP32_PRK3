#include <Arduino.h>
#include <uart.h>

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