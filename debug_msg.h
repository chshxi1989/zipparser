#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef _DEBUG_MSG_H_
#define _DEBUG_MSG_H_

#ifdef OPEN_DEBUG_MSG
#define DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

#endif