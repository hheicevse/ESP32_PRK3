#ifndef _MSPM0_ESP32_COMM_H_
#define _MSPM0_ESP32_COMM_H_
#pragma once


typedef struct {
    String bsl_url;
    bool bsl_triggered;
    int bsl_fd;
} mspm0Comm_t;
extern mspm0Comm_t mspm0Comm;

void mspm0_read(const char * rx);
void mspm0_esp32_comm_init();
String get_mcu_report();

#endif /* _MSPM0_ESP32_COMM_H_ */