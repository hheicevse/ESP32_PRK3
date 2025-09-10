
#include <Arduino.h>
#include "driver/uart.h"
#include "mspm0/comm_proto.h"
#include <main.h>
#include <ArduinoJson.h>
mspm0Comm_t mspm0Comm = {};
StaticJsonDocument<512> msg;

String get_mcu_report()
{
    String s;
    serializeJson(msg, s);
    return s;
}

void mspm0_read(const char *rx)
{
    mcu_report_t r = decode_mcu_report(rx);
    if (r.ok)
    {
        msg["current"] = (double)r.ma / 1000;
        return;
    }
    char *v = decode_mcu_version(rx);
    if (strlen(v))
    {
        Serial.printf("[MCU] version=%s\n", v);
        return;
    }
    Serial.printf("[MCU] ");
    for (size_t i = 0; i < strlen(rx); i++)
        Serial.printf("%d,", rx[i]);
    Serial.println();
}
void mspm0_polling(void *param)
{
    Serial1.println();
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial1.println("init");
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial1.println("report=1");
    while (true)
    {
        if (mspm0Comm.bsl_triggered)
        {
            uart1_deinit();
            bsl_func(mspm0Comm.bsl_url.c_str(),mspm0Comm.bsl_fd); // 假設這是阻塞函式
            mspm0Comm.bsl_triggered = false;     // 清除旗標
            mspm0Comm.bsl_url = "";
            mspm0Comm.bsl_fd = -1;
            uart1_deinit();
            uart1_init();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void mspm0_esp32_comm_init()
{
    uart1_init();
    xTaskCreatePinnedToCore(mspm0_polling, "mspm0_polling", 4096, NULL, 2, NULL, APP_CPU_NUM);
}