#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zip.h"

#define TABLE_INIT_SIZE (10)
static struct ZipEntry* g_pzip_entry = NULL;
static int g_zip_entry_num = 0;
static int g_cur_entry_pos = 0;

int hashtable_init()
{
    if (g_pzip_entry != NULL)
    {
        free(g_pzip_entry);
    }
    g_zip_entry_num = TABLE_INIT_SIZE;
    g_cur_entry_pos = 0;
    g_pzip_entry = malloc(g_zip_entry_num*sizeof(struct ZipEntry));
    return 0;
}

int hashtable_destory()
{
    if (g_pzip_entry != NULL)
    {
        free(g_pzip_entry);
    }
    return 0;
}

int hashtable_insert(struct ZipEntry* pZipEntry)
{
    if (g_pzip_entry == NULL)
    {
        printf("please init hash table first\n");
        return -1;
    }
    memcpy(g_pzip_entry + g_cur_entry_pos, pZipEntry, sizeof(struct ZipEntry));
    g_cur_entry_pos++;
    if (g_cur_entry_pos == g_zip_entry_num)
    {
        // realloc zipentry
        int new_zip_entry_num = g_zip_entry_num*2;
        struct ZipEntry* temp = malloc(new_zip_entry_num*sizeof(struct ZipEntry));
        if (temp == NULL)
        {
            // realloc memory failed
            free(g_pzip_entry);
            printf("failed to realloc zip entry\n");
            return -1;
        }
        memcpy(temp, g_pzip_entry, sizeof(struct ZipEntry)*g_zip_entry_num);
        free(g_pzip_entry);
        g_pzip_entry = temp;
        g_zip_entry_num = new_zip_entry_num;
    }
    return 0;
}

int hashtable_getsize()
{
    return g_cur_entry_pos;
}

struct ZipEntry* hashtable_get()
{
    return g_pzip_entry;
}

