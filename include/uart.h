#ifndef _UART_H_
#define _UART_H_

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
uart1_t uart1;









#endif /* _UART_H_ */