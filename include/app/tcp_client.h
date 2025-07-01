#ifndef _TCP_TEST_H_
#define _TCP_TEST_H_
#pragma once

int tcp_available();
bool tcp_connect(const char* url, int port=502);
bool tcp_send_data(const uint8_t *data, size_t len);
int tcp_receive_data(uint8_t *buf, size_t bufsize);
void tcp_disconnect();

#endif /* _TCP_TEST_H_ */