#ifndef _MSPM0_ESP32_COMM_H_
#define _MSPM0_ESP32_COMM_H_
#pragma once



typedef struct {
    String bsl_url;
    bool bsl_triggered;
} mspm0Comm_t;
extern mspm0Comm_t mspm0Comm;

void mspm0_esp32_comm_init();

#endif /* _MSPM0_ESP32_COMM_H_ */