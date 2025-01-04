#ifndef __UTF8_GB2312_SWITCH_H
#define __UTF8_GB2312_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>

size_t utf8_to_gb2312(const uint8_t *utf8Data, size_t len, uint8_t *gb2312Data, size_t size);
size_t gb2312_to_utf8(const uint8_t *gb2312Data, size_t len, uint8_t *utf8Data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // !