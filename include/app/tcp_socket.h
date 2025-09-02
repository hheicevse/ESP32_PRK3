#ifndef _TCP_SOCKET_H_
#define _TCP_SOCKET_H_
#pragma once

#define SERVER_PORT_STA 1502
#define SERVER_PORT_AP 502
int tcp_sta_deinit();
int tcp_sta_init();
void tcp_sta_func();

int tcp_ap_init(uint32_t ip, uint16_t port);
void tcp_ap_func();


#endif /* _TCP_SOCKET_H_ */