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

CFLAGS += -O2 -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC
CFLAGS += -DDEBUG -g
LFLAGS += -lpthread -ldl

all: output_http_push.so

clean:
	rm -f *.a *.o core *~ *.so *.lo

output_http_push.so: $(OTHER_HEADERS) output_http_push.c serverutils.lo
	$(CC) $(CFLAGS) -o $@ output_http_push.c serverutils.lo

serverutils.lo: $(OTHER_HEADERS) serverUtils.c
	$(CC) -c $(CFLAGS) -o $@ serverUtils.c
