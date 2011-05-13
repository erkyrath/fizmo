
include ../../config.mk

# "-std=c99" is required to avoid trouble with the built-in round function.
CFLAGS += -std=c99 $(SOUNDSDL_PKG_CFLAGS) $(SOUNDSDL_NONPKG_CFLAGS)

ifneq ($(ENABLE_TRACING),)
CFLAGS += -DENABLE_TRACING=
endif

ifneq ($(ENABLE_OPTIMIZATION),)
CFLAGS += -O2
endif

ifneq ($(ENABLE_AIFF_FOR_SOUND_SDL),)
CFLAGS += -DENABLE_AIFF_FOR_SOUND_SDL=
endif

ifneq ($(ENABLE_GDB_SYMBOLS),)
override CFLAGS += -g
endif

LIBFIZMO_CFLAGS = $(shell pkg-config --cflags libfizmo)


.PHONY : all clean

all: libsndifsdl.a

libsndifsdl.a: sound_sdl.o
	$(AR) rcs libsndifsdl.a sound_sdl.o

sound_sdl.o: sound_sdl.c
	$(CC) $(CFLAGS) $(LIBFIZMO_CFLAGS) $(SDL_CFLAGS) $(SNDFILE_CFLAGS) -c sound_sdl.c

clean:
	rm -f *.o

distclean: clean
	rm -f libsndifsdl.a

