#OPEN_DEBUG_MSG := true
OPEN_DEBUG_MSG := false
ifeq ($(OPEN_DEBUG_MSG), true)
LOCAL_COMPILE_FLAG := -DOPEN_DEBUG_MSG
else
LOCAL_COMPILE_FLAG := 
endif

all:
	gcc -Wall $(LOCAL_COMPILE_FLAG) -c zip.c -o zip.o
	gcc -Wall $(LOCAL_COMPILE_FLAG) -c hashtable.c -o hashtable.o
	gcc -Wall $(LOCAL_COMPILE_FLAG) -c crc32.c -o crc32.o
	gcc -Wall $(LOCAL_COMPILE_FLAG) -c util.c -o util.o
	gcc -Wall hashtable.o crc32.o util.o zip.o -lz -o zip
