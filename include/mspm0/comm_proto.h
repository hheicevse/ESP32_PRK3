#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
typedef struct
{
    const char *report;
    const char *ver;
} fmt_codec_t;
typedef struct
{
    const char *init;
    const char *report;
    const char *cset;
} fmt_host_t;
fmt_host_t host = {
    .init = "init",
    .report = "report=%d",
    .cset = "cset=%d",
};
fmt_codec_t codec = {
    .report = "report=%d,%d,%d,%d,%d\n",
    .ver = "version=%d.%d.%d\n",
};
typedef struct
{
    bool ok;
    uint8_t max;
    uint8_t cp;
    uint8_t temp;
    uint32_t mv;
    uint32_t ma;
} mcu_report_t;
char *encode_mcu_report(mcu_report_t r)
{
    static char str[32] = {0};
    sprintf(str, codec.report, r.max, r.cp, r.temp, r.mv, r.ma);
    return str;
}
mcu_report_t decode_mcu_report(const char *line)
{
    static mcu_report_t r = {0};
    r.ok = sscanf(line, codec.report, &r.max, &r.cp, &r.temp, &r.mv, &r.ma);
    return r;
}
char *decode_mcu_version(const char *line)
{
    static char str[16] = {0};
    sscanf(line, "version=%s\n", str);
    return str;
}
