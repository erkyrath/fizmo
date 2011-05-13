
include ../../config.mk

ifneq ($(ENABLE_GDB_SYMBOLS),)
override CFLAGS += -g
endif

ifneq ($(ENABLE_OPTIMIZATION),)
override CFLAGS += -O2
endif

ifneq ($(ENABLE_TRACING),)
override CFLAGS += -DENABLE_TRACING=
endif

ifneq ($(LOCALE_SEARCH_PATH),)
I18N_DEFAULT_SEARCH_PATH = \"$(LOCALE_SEARCH_PATH)\"
endif


.PHONY : clean

all: libtools.a

i18n.o: i18n.h tracelog.h z_ucs.h stringmap.h list.h ../locales/libfizmo_locales.h i18n.c
	$(CC) $(CFLAGS) -DI18N_DEFAULT_SEARCH_PATH=$(I18N_DEFAULT_SEARCH_PATH) -c i18n.c

list.o: list.h list.c
	$(CC) $(CFLAGS) -c list.c

stringmap.o: stringmap.h types.h z_ucs.h tracelog.h stringmap.c
	$(CC) $(CFLAGS) -c stringmap.c

tracelog.o: tracelog.h types.h i18n.h z_ucs.h ../locales/libfizmo_locales.h tracelog.c
	$(CC) $(CFLAGS) -c tracelog.c

types.o: types.h types.c
	$(CC) $(CFLAGS) -c types.c

z_ucs.o: z_ucs.h types.h z_ucs.c
	$(CC) $(CFLAGS) -c z_ucs.c

libtools.a: ../locales/libfizmo_locales.o types.o z_ucs.o stringmap.o i18n.o tracelog.o list.o
	$(AR) rcs libtools.a types.o z_ucs.o stringmap.o i18n.o tracelog.o list.o ../locales/libfizmo_locales.o

test: tools_test
	./tools_test

tools_test.o: tools_test.c
	$(CC) $(CFLAGS) -c tools_test.c

../locales/libfizmo_locales.o::
	cd ../locales ; make libfizmo_locales.o

tools_test: libtools.a tools_test.o
	$(CC) $(CFLAGS) tools_test.o -L. -ltools -o tools_test

clean:
	rm -f *.o tools_test tracelog.txt

distclean: clean
	rm -f libtools.a

