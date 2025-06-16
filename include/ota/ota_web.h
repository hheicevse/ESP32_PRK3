#ifndef _OTA_WEB_H_
#define _OTA_WEB_H_

#pragma once

#include <WebServer.h>

void ota_web_init(void);
void ota_web_Task(void *pv);
void handleRoot();
void handleUpdateUpload();
void handleUpdatePage();
void handleUpdateDone();

extern WebServer ota_web;

#endif /* _OTA_WEB_H_ */
