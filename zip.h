#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef _ZIP_H_
#define _ZIP_H_
/*
 * Offset and length constants (java.util.zip naming convention).
 */

enum {
    CENSIG = 0x02014b50,      // PK12 , central directory header, describe file entry
    CENHDR = 46,

    CENVEM =  4,
    CENVER =  6,
    CENFLG =  8,
    CENHOW = 10,
    CENTIM = 12,
    CENDAT = 14,
    CENCRC = 16,
    CENSIZ = 20,
    CENLEN = 24,
    CENNAM = 28,
    CENEXT = 30,
    CENCOM = 32,
    CENDSK = 34,
    CENATT = 36,
    CENATX = 38,
    CENOFF = 42,

    ENDSIG = 0x06054b50,     // PK56 , EOCD, end of central directory
    ENDHDR = 22,

    ENDSUB =  8,             // central directory num
    ENDTOT = 10,
    ENDSIZ = 12,
    ENDOFF = 16,             // central directory offset
    ENDCOM = 20,

    EXTSIG = 0x08074b50,     // PK78, external data, no exist commonly
    EXTHDR = 16,

    EXTCRC =  4,
    EXTSIZ =  8,
    EXTLEN = 12,

    LOCSIG = 0x04034b50,      // PK34, local file header
    LOCHDR = 30,

    LOCVER =  4,
    LOCFLG =  6,
    LOCHOW =  8,
    LOCTIM = 10,
    LOCCRC = 14,
    LOCSIZ = 18,
    LOCLEN = 22,
    LOCNAM = 26,
    LOCEXT = 28,

    STORED = 0,
    DEFLATED = 8,

    CENVEM_UNIX = 3 << 8,   // the high byte of CENVEM
};

struct ZipEntry {
    uint32_t filenameLen;
    char* filename;
    uint32_t compLen;
    uint32_t uncompLen;
    uint32_t offset;
    uint32_t crc32;
    uint16_t compression;
    uint32_t versionMadeBy;
    uint32_t externalFileAttributes;
    uint16_t modifyTime;
    uint16_t modifyData;
};
#endif