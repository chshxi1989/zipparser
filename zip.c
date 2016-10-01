#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <stdint.h>
#include <zlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>
#include <getopt.h>
#include "zip.h"
#include "hashtable.h"
#include "debug_msg.h"

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

int inflate_entry(struct ZipEntry* pZipEntry, uint8_t* pZipAddr, const char* dir_path) {
    if (pZipEntry == NULL || pZipAddr == NULL) {
        printf("pZipEntry or pZipAddr can not be NULL\n");
        return -1;
    }
    if ((pZipEntry->versionMadeBy&0xFF00) != CENVEM_UNIX) {
        printf("zip file not unix stype\n");
        return -1;
    }
    
    // construct file name
    char filename[PATH_MAX] = "\0";
    if (dir_path != NULL)
    {
        strcpy(filename, dir_path);
        if (dir_path[strlen(dir_path) -1] != '/') {
            strcat(filename, "/");
        }
    }
    strncat(filename, pZipEntry->filename, pZipEntry->filenameLen);
    DEBUG("filename :%s\n", filename);
    mode_t mode = pZipEntry->externalFileAttributes >> 16;
    if (S_ISDIR(mode)) {
        printf("object is a directory\n");
        mkdir(filename, S_IRWXU);
        return 0;
    }
    if (!S_ISREG(mode)) {
        printf("object is not a regular file\n");
        return -1;
    }
    // create new file
    int dstfd = open(filename, O_CREAT|O_WRONLY, S_IRWXU);
    uint32_t result = 0;
    unsigned char procBuf[32 * 1024];
    z_stream zstream;
    int zerr;

    /*
     * Initialize the zlib stream.
     */
    memset(&zstream, 0, sizeof(zstream));
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.next_in = pZipAddr + pZipEntry->offset;
    zstream.avail_in = pZipEntry->compLen;
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
    if (result != pZipEntry->uncompLen) {
        if (result != -1)        // error already shown?
            printf("Size mismatch on inflated file (0x%x vs 0x%x)\n", result, pZipEntry->uncompLen);
        return -1;
    }
    close(dstfd);
    return 0;
}

int checkdir(const char* dirpath) {
    struct stat dirstat;
    int ret = stat(dirpath, &dirstat);
    if (ret != 0) {
        // directory not exist, so create it
        mkdir(dirpath, S_IRWXU);
    } else {
        if (!S_ISDIR(dirstat.st_mode)) {
            printf("%s not a directory\n", dirpath);
            return -1;
        }
    }
    return 0;
}

void print_usage()
{
    printf("zip [-f zipfile] [-d dir] -l\n");
}

void print_entry(char* zipfile, struct ZipEntry* pZipEntry, int entry_num)
{
    printf("Archive:  %s\n", zipfile);
    printf("  Length   Name\n");
    printf("---------  ----\n");
    int loop = 0;
    int i = 0;
    for(loop = 0; loop < entry_num; loop++)
    {
        printf("%9d  ", pZipEntry->uncompLen);
        for(i = 0; i< pZipEntry->filenameLen; i++)
        {
            printf("%c", *(pZipEntry->filename + i));
        }
        printf("\n");
        pZipEntry++;
    }
}

int main(int argc, char** argv) {
    int ret = -1;
    int list_flag = -1;
    char* zipfile = NULL;
    char* zipdir = NULL;
    while ((ret = getopt(argc, argv, "ld:f:")) != -1)
    {
        DEBUG("%c\n", ret);
        switch(ret)
        {
            case 'l':
                list_flag = 1;
                break;
            case 'd':
                zipdir = optarg;
                break;
            case 'f':
                zipfile = optarg;
                break;
            default: 
                printf("unknow option %c\n", ret);
                print_usage();
                return -1;
                break;
        }
    }
    if (zipfile == NULL)
    {
        printf("zip file can not be None\n");
        print_usage();
        return -1;
    }
    if (zipdir != NULL)
    {
        ret = checkdir(zipdir);
        if (ret != 0)
        {
            return -1;
        }
    }
    struct stat zipstat;
    // get file size
    ret = stat(zipfile, &zipstat);
    if (ret != 0) {
        printf("stat file(%s) failed, %s\n", zipfile, strerror(errno));
        return -1;
    }
    uint32_t zipFileLength = zipstat.st_size;
    DEBUG("file size: 0x%x\n", zipFileLength);
    // map file to dram
    int fd = open(zipfile, O_RDONLY);
    if (fd == -1) {
        printf("can not open %s, %s\n", zipfile, strerror(errno));
        return -1;
    }
    uint8_t* pZipAddr = (uint8_t*)mmap(NULL, zipFileLength, PROT_READ, MAP_PRIVATE, fd, 0);
    if (pZipAddr == MAP_FAILED) {
        printf("can not map zip file %s to dram, %s\n", zipfile, strerror(errno));
        return -1;
    }
    // parser EOCD
    uint8_t* pu8EocdData = pZipAddr + zipFileLength - ENDHDR;
    // check EOCD SIG
    if (get4LE(pu8EocdData) != ENDSIG) {
        printf("zipfile %s not a zip format\n", zipfile);
        ret = -1;
        goto done;
    }
    uint32_t entryNum = get2LE(pu8EocdData + ENDSUB); // how many central directory in zip file
    uint32_t entryOffset = get2LE(pu8EocdData + ENDOFF); // central directory data offset in zip file
    
    // parser central directory
    uint8_t* pu8CentralDirEntry = pZipAddr + entryOffset;
    uint8_t* pu8LocalFileHeader = NULL;
    int loop;
    struct ZipEntry zipEntry;
    uint32_t u32localOffset = 0;
    // create hash table
    hashtable_init();
    for (loop = 0; loop < entryNum; loop++) {
        // check central directory SIG
        if (get4LE(pu8CentralDirEntry) != CENSIG) {
            printf("central directory sig of %d not match\n", loop);
            continue;
        }
        zipEntry.versionMadeBy = get2LE(pu8CentralDirEntry + CENVEM);
        zipEntry.externalFileAttributes = get4LE(pu8CentralDirEntry + CENATX);
        zipEntry.crc32 = get2LE(pu8CentralDirEntry + CENCRC);
        zipEntry.filenameLen = get2LE(pu8CentralDirEntry + CENNAM);
        zipEntry.filename = (char*)pu8CentralDirEntry + CENHDR;
        zipEntry.compLen = get4LE(pu8CentralDirEntry + CENSIZ);
        zipEntry.uncompLen = get4LE(pu8CentralDirEntry + CENLEN);
        u32localOffset = get4LE(pu8CentralDirEntry + CENOFF);
        pu8LocalFileHeader = pZipAddr + u32localOffset;
        zipEntry.offset = u32localOffset + LOCHDR + get2LE(pu8LocalFileHeader + LOCNAM) + get2LE(pu8LocalFileHeader + LOCEXT);
        pu8CentralDirEntry += CENHDR + zipEntry.filenameLen + get2LE(pu8CentralDirEntry + CENEXT)+ get2LE(pu8CentralDirEntry + CENCOM);
        // hash table insert
        hashtable_insert(&zipEntry);
    }

    struct ZipEntry* pZipEntry = hashtable_get();
    int hash_num = hashtable_getsize();
    if (list_flag == 1)
    {
        // print file info in zip file
        print_entry(zipfile, pZipEntry, hash_num);
    }
    else
    {
        // inflate file data
        for(loop = 0; loop < hash_num; loop++)
        {
            ret = inflate_entry(pZipEntry + loop, pZipAddr, zipdir);
            if( ret != 0) {
                printf("inflate file %s failed\n", zipEntry.filename);
                goto done;
            }
        }
        printf("inflate zip file ok\n");
    }
done:
    hashtable_destory();
    munmap(pZipAddr, zipFileLength);
    close(fd);
    if (ret != 0) {
        return -1;
    } else {
        return 0;
    }
}
