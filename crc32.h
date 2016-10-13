#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef _CRC32_H_
#define _CRC32_H_
uint32_t cal_crc32(uint32_t crc, const void *buf, uint32_t size);
#endif