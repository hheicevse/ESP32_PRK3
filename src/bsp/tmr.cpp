#include <Arduino.h>
#include <bsp/uart.h>

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void ARDUINO_ISR_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);

  if(uart1.rx_flag==TIMER_RUNNING && uart1.rx_timeout++>=10)
  uart1.rx_flag=TIMER_FINISH;
  portEXIT_CRITICAL_ISR(&timerMux);
}

#ifdef PLATFORMIO

void tmr_init() {
  // 初始化 timer，參數如下：
  // timerBegin(timer number 0~3, 頻率分頻器, 計數方向)
  // 80 分頻 = 1 tick = 1μs（ESP32 80MHz APB clock）
  timer = timerBegin(0, 80, true);

  // 設定中斷觸發的 callback
  timerAttachInterrupt(timer, &onTimer, true);
 
  // 設定 100μs 觸發一次中斷（100 ticks）
  timerAlarmWrite(timer, 100, true);
  // 啟用中斷
  timerAlarmEnable(timer);
}
#else

void tmr_init() {
  // Set timer frequency to 1Mhz
  timer = timerBegin(1000000);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer);
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(timer, 100, true, 0);
}

#endif
