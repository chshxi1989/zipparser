#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <errno.h>
#include <stdint.h>
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

unsigned int get2LE(char* p) {
    unsigned int result = 0;
    result = *p++;
    result = result|((*p++)<<8);
    return result;
}

unsigned int get4LE(char* p) {
    unsigned int result = 0;
    result = *p++;
    result = result|((*p++)<<8);
    result = result|((*p++)<<16);
    result = result|((*p++)<<24);
    return result;
}

int main(int argc, char** argv) {
    // get file size
    if( argc != 2) {
        printf("usage: zip [zipfilename]\n");
        return -1;
    }
    struct stat zipstat;
    int ret = stat(argv[1], &zipstat);
    if (ret != 0) {
        printf("stat file(%s) failed, %s\n", argv[1], strerror(errno));
        return -1;
    }
    long zipFileLength = zipstat.st_size;
    
    // get EOCD
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        printf("can not open %s\n", argv[1]);
        return -1;
    }
    unsigned char eocdBuffer[ENDHDR];
    lseek(fd, zipFileLength - ENDHDR, SEEK_SET);
    read(fd, eocdBuffer, ENDHDR);
    unsigned int entrynum = get2LE(eocdBuffer + ENDSUB);
    unsigned int entryoffset = get4LE(eocdBuffer + ENDOFF);
    
    // search central directory area
    lseek(fd, entryoffset, SEEK_SET);
    int i = 0;
    unsigned char cdBuffer[CENHDR];
    uint32_t fileNameLen = 0;
    uint32_t extraLen = 0;
    uint32_t fileCommentLen = 0;
    uint32_t u32offset = entryoffset;
    char nameBuffer[PATH_MAX];
    for(i = 0; i < entrynum; i++) {
        printf("CD info offset: 0x%x\n", u32offset);
        read(fd, cdBuffer, CENHDR);
        uint32_t u32sig = get4LE(cdBuffer);
        if (u32sig != 0x02014b50) {
            printf("central directory sig error: 0x%x, expert 0x02014b50\n", u32sig);
            close(fd);
            return -1;
        }
        fileNameLen = get2LE(cdBuffer + CENNAM);
        extraLen = get2LE(cdBuffer + CENEXT);
        fileCommentLen = get2LE(cdBuffer + CENCOM);
        printf("fileNameLen: %d, extraLen: %d, fileCommentLen: %d\n", fileNameLen, extraLen, fileCommentLen);
        if (fileNameLen > PATH_MAX) {
            printf("file name length(0x%x) should not larger than PATH_MAX\n", fileNameLen);
            close(fd);
            return -1;
        }
        read(fd, nameBuffer, fileNameLen);
        nameBuffer[fileNameLen] = '\0';
        printf("num %d file name: %s\n", i, nameBuffer);
        // seek next central directory
        u32offset += CENHDR + fileNameLen + extraLen + fileCommentLen;
        lseek(fd, u32offset, SEEK_SET);
    }
    close(fd);
    return 0;
}