
include ../../config.mk


ifneq ($(ENABLE_GDB_SYMBOLS),)
override CFLAGS += -g
endif

ifneq ($(ENABLE_OPTIMIZATION),)
override CFLAGS += -O2
endif


.PHONY : clean

all: libfizmo_locales.o


libfizmo_locales.o: libfizmo_locales.c
	"$(CC)" $(CFLAGS) -c libfizmo_locales.c

clean:
	rm -f *.o

