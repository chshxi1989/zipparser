#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef OPEN_DEBUG_MSG
#define DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

unsigned int get2LE(unsigned char* p);
unsigned int get4LE(unsigned char* p);
void memdump(unsigned char* p, int len);

#endif