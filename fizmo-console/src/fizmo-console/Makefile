
include ../../config.mk

ifneq ($(ENABLE_TRACING),)
override CFLAGS += -DENABLE_TRACING=
endif

ifneq ($(ENABLE_READCHAR_VIA_TERMIOS),)
override CFLAGS += -DENABLE_READCHAR_VIA_TERMIOS=
endif

override CFLAGS += $(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config \
 --cflags libfizmo)
LIBS = -lm
LIBS += $(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config --libs libfizmo)

fizmo-console: fizmo-console.o
	"$(CC)" $(LDFLAGS) fizmo-console.o -o fizmo-console $(LIBS)
	#strip fizmo

fizmo-console.o: fizmo-console.c
	"$(CC)" $(CFLAGS) -c fizmo-console.c

clean:
	rm -f *.o

distclean: clean
	rm -f fizmo-console

