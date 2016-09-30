#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zip.h"

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_
int hashtable_init();
int hashtable_insert(struct ZipEntry* pZipEntry);
int hashtable_getsize();
struct ZipEntry* hashtable_get();
#endif