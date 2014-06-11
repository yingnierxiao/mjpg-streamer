###############################################################
#
# Purpose: Makefile for "M-JPEG Streamer"
# Author.: Tom Stoeveken (TST)
# Version: 0.3
# License: GPL
#
###############################################################

CC = gcc

OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

CFLAGS += -O2 -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC -std=c99
CFLAGS += -DDEBUG -g
LFLAGS += -lpthread -ldl



LFLAGS += -ljpeg -lrt
all: output_http_push.so

clean:
	rm -f *.a *.o core *~ *.so *.lo

output_http_push.so: $(OTHER_HEADERS) output_http_push.c serverutils.lo jpegutils.lo
	$(CC) $(CFLAGS) -o $@ output_http_push.c serverutils.lo jpegutils.lo $(LFLAGS)

serverutils.lo: $(OTHER_HEADERS) serverUtils.c
	$(CC) -c $(CFLAGS) -o $@ serverUtils.c 

jpegutils.lo: $(OTHER_HEADERS) jpegUtils.c
	$(CC) -c $(CFLAGS) -o $@ jpegUtils.c
