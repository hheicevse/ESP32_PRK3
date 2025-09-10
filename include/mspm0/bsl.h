#ifndef _BSL_COPILOT_H_
#define _BSL_COPILOT_H_
#pragma once

#define MSPM0_UART_PORT UART_NUM_1
void bsl_init_uart();
void bsl_func(const char* url,int fd);

#endif /* _BSL_COPILOT_H_ */