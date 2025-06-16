#ifndef _OTA_HTTP_CA_H_
#define _OTA_HTTP_CA_H_

#pragma once
#include <HTTPClient.h> 

void ota_http_ca_init();
bool ota_http_CA_func(const char* url);

extern HTTPClient ota_http;

#endif /* _OTA_HTTP_CA_H_ */
