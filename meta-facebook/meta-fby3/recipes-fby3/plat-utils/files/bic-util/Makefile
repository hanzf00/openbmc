# Copyright 2015-present Facebook. All Rights Reserved.
all: bic-util

CFLAGS += -Wall -Werror

bic-util: bic-util.c
	$(CC) -pthread $(CFLAGS) -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o bic-util
