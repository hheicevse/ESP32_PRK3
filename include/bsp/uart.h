#ifndef _UART_H_
#define _UART_H_
#pragma once
enum{
    TIMER_NOTHING,
    TIMER_RUNNING,
    TIMER_FINISH,
    TIMER_TIMEOUT,
};

typedef struct {
    unsigned char rx_count;
    unsigned char rx_flag;
    unsigned int rx_timeout;
    unsigned char rx[100];
    unsigned char tx_data[100];//開4組陣列
    unsigned char tx_len;

} uart1_t;
extern uart1_t uart1;

void uart1_init();
void uart1_deinit();
void uart1_rx_func(void);
void uart0_rx_func(void);









#endif /* _UART_H_ */