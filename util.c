#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "util.h"

unsigned int get2LE(unsigned char* p) {
    unsigned int result = 0;
    result = *p++;
    result = result|((*p++)<<8);
    return result;
}

unsigned int get4LE(unsigned char* p) {
    unsigned int result = 0;
    result = *p++;
    result = result|((*p++)<<8);
    result = result|((*p++)<<16);
    result = result|((*p++)<<24);
    return result;
}

void memdump(unsigned char* p, int len) {
    int i = 0;
    for (i = 0; i< len; i++) {
        printf("0x%x ", *(p++));
    }
    printf("\n");
}