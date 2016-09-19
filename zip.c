#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <errno.h>
#include <stdint.h>
#include <zlib.h>
#include <assert.h>
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

unsigned int get4LE(unsigned char* p) {
    unsigned int result = 0;
    result = *p++;
    result = result|((*p++)<<8);
    result = result|((*p++)<<16);
    result = result|((*p++)<<24);
    return result;
}

int decompress(int srcfd, uint32_t u32srcOffset, uint32_t u32srcCompLen, uint32_t u32srcUncompLen, int dstfd) {
    long result = 0;
    unsigned char procBuf[32 * 1024];
    z_stream zstream;
    int zerr;
    // malloc buffer to store data
    uint8_t* pBuf = (uint8_t*)malloc(u32srcCompLen);
    lseek(srcfd, u32srcOffset, SEEK_SET);
    if(read(srcfd, pBuf, u32srcCompLen) != u32srcCompLen) {
        printf("failed to read data\n");
        free(pBuf);
        return -1;
    }
    
    /*
     * Initialize the zlib stream.
     */
    memset(&zstream, 0, sizeof(zstream));
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.next_in = pBuf;
    zstream.avail_in = u32srcCompLen;
    zstream.next_out = (Bytef*) procBuf;
    zstream.avail_out = sizeof(procBuf);
    zstream.data_type = Z_UNKNOWN;

    /*
     * Use the undocumented "negative window bits" feature to tell zlib
     * that there's no zlib header waiting for it.
     */
    zerr = inflateInit2(&zstream, -MAX_WBITS);
    if (zerr != Z_OK) {
        if (zerr == Z_VERSION_ERROR) {
            printf("Installed zlib is not compatible with linked version (%s)\n",
                ZLIB_VERSION);
        } else {
            printf("Call to inflateInit2 failed (zerr=%d)\n", zerr);
        }
        goto bail;
    }

    /*
     * Loop while we have data.
     */
    do {
        /* uncompress the data */
        zerr = inflate(&zstream, Z_NO_FLUSH);
        if (zerr != Z_OK && zerr != Z_STREAM_END) {
            printf("zlib inflate call failed (zerr=%d)\n", zerr);
            goto z_bail;
        }

        /* write when we're full or when we're done */
        if (zstream.avail_out == 0 || (zerr == Z_STREAM_END && zstream.avail_out != sizeof(procBuf))) {
            long procSize = zstream.next_out - procBuf;
            printf("+++ processing %d bytes\n", (int) procSize);
            int ret = write(dstfd, procBuf, procSize);
            if (ret == -1) {
                printf("Process function elected to fail (in inflate)\n");
                goto z_bail;
            }

            zstream.next_out = procBuf;
            zstream.avail_out = sizeof(procBuf);
        }
    } while (zerr == Z_OK);

    assert(zerr == Z_STREAM_END);       /* other errors should've been caught */

    // success!
    result = zstream.total_out;

z_bail:
    inflateEnd(&zstream);        /* free up any allocated structures */

bail:
    if (result != u32srcUncompLen) {
        if (result != -1)        // error already shown?
            printf("Size mismatch on inflated file (%ld vs %ld)\n", result, u32srcUncompLen);
        free(pBuf);
        return -1;
    }
    free(pBuf);
    return 0;
}

void memdump(unsigned char* p, int len) {
    int i = 0;
    for (i = 0; i< len; i++) {
        printf("0x%x ", *(p++));
    }
    printf("\n");
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
    printf("file st mode: 0x%x\n", zipstat.st_mode);
    printf("file size: 0x%x\n", zipFileLength);
    if (S_ISREG(zipstat.st_mode)) {
        printf("is a regular file\n");
    }
    // get EOCD
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        printf("can not open %s\n", argv[1]);
        return -1;
    }
    unsigned char eocdBuffer[ENDHDR];
    lseek(fd, zipFileLength - ENDHDR, SEEK_SET);
    ret = read(fd, eocdBuffer, ENDHDR);
    if (ret != ENDHDR) {
        printf("read data from %s failed", argv[1]);
        return -1;
    }
    memdump(eocdBuffer, ENDHDR);
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
    uint32_t u32fileLocalHeaderOffset = 0;
    char nameBuffer[PATH_MAX];
    unsigned char localFileHeaderBuffer[LOCHDR];
    uint32_t u32fileExteralAttr = 0;
    uint16_t u16versionMadeBy = 0;
    int newfd = -1;
    for(i = 0; i < entrynum; i++) {
        printf("CD info offset: 0x%x\n", u32offset);
        lseek(fd, u32offset, SEEK_SET);
        read(fd, cdBuffer, CENHDR);
        uint32_t u32sig = get4LE(cdBuffer);
        if (u32sig != CENSIG) {
            printf("central directory sig error: 0x%x, expert 0x%x\n", u32sig, CENSIG);
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
        u16versionMadeBy = get2LE(cdBuffer + CENVEM);
        // check if unix file
        if (u16versionMadeBy&0xFF00 != CENVEM_UNIX) {
            printf("file not unix style\n");
            return -1;
        }
        u32fileExteralAttr = get4LE(cdBuffer + CENATX);
        // seek next central directory
        u32offset += CENHDR + fileNameLen + extraLen + fileCommentLen;
        
        // get file local header
        u32fileLocalHeaderOffset = get4LE(cdBuffer + CENOFF);
        lseek(fd, u32fileLocalHeaderOffset, SEEK_SET);
        read(fd, localFileHeaderBuffer, LOCHDR);
        u32sig =  get4LE(localFileHeaderBuffer);
        if (u32sig != LOCSIG) {
            printf("local file header sig error: 0x%x, expert 0x%x\n", u32sig, LOCSIG);
            close(fd);
            return -1;
        }
        // get file data
        uint32_t u32compLen = get4LE(localFileHeaderBuffer+LOCSIZ);
        uint32_t u32uncompLen = get4LE(localFileHeaderBuffer+LOCLEN);
        uint32_t u32dataOffset = u32fileLocalHeaderOffset + LOCHDR + get2LE(localFileHeaderBuffer+LOCNAM) + get2LE(localFileHeaderBuffer+LOCEXT);
        uint8_t u8compType = get2LE(localFileHeaderBuffer+LOCHOW);
        printf("u32fileExteralAttr: 0x%x\n", u32fileExteralAttr);
        if (S_ISDIR(u32fileExteralAttr >> 16)) {
            // file is a direcotry
            printf("object is a directory\n");
            mkdir(nameBuffer, S_IRUSR|S_IWUSR|S_IXUSR);
        } else if (S_ISREG(u32fileExteralAttr >> 16)) {
            // object is a regular file
            printf("object is a regular file\n");
            newfd = open(nameBuffer, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
            // decompress data
            if (u8compType == STORED) {
                // data not compress
            } else if (u8compType == DEFLATED) {
                // deflate compress
                ret = decompress(fd, u32dataOffset, u32compLen, u32uncompLen, newfd);
                if (ret != 0) {
                    printf("data decompress failed\n");
                }
            }
            close(newfd);
        }
    }
    close(fd);
    return 0;
}
