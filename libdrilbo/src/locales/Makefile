
.PHONY : clean

include ../../config.mk

ifneq ($(ENABLE_GDB_SYMBOLS),)
override CFLAGS += -g
endif

ifneq ($(ENABLE_OPTIMIZATION),)
override CFLAGS += -O2
endif

FIZMO_CFLAGS = $(shell pkg-config --cflags libfizmo)


all: libdrilbo_locales.o


libdrilbo_locales.o: libdrilbo_locales.c
	$(CC) $(CFLAGS) $(FIZMO_CFLAGS) -c libdrilbo_locales.c

clean:
	rm -f *.o

