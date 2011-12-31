
include ../../config.mk

ifneq ($(ENABLE_TRACING),)
CFLAGS = -DENABLE_TRACING=
endif

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
 --cflags libfizmo)
CFLAGS += $(NCURSESW_PKG_CFLAGS)
CFLAGS += $(LIBGLKTERMW_NONPKG_CFLAGS)


LIBS = -lm
LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs libfizmo)
LIBS += $(LIBGLKTERMW_NONPKG_LIBS)
LIBS += $(NCURSESW_NONPKG_LIBS) $(NCURSESW_PKG_LIBS)
LIBS += -lglkif -lglktermw


fizmo-glktermw: fizmo-glktermw.o
	$(CC) $(CFLAGS) fizmo-glktermw.o -o fizmo-glktermw $(LIBS)
	#strip fizmo

fizmo-glktermw.o: fizmo-glktermw.c
	$(CC) $(CFLAGS) -c fizmo-glktermw.c

clean:
	rm -f *.o

distclean: clean
	rm -f fizmo-glktermw


