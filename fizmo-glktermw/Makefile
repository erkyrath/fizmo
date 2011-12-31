
.PHONY : all install clean distclean

include config.mk


all: src/fizmo-glktermw/fizmo-glktermw

src/fizmo-glktermw/fizmo-glktermw::
	cd src/fizmo-glktermw ; make
	cp src/fizmo-glktermw/fizmo-glktermw .

install: src/fizmo-glktermw/fizmo-glktermw
	mkdir -p $(bindir)
	cp src/fizmo-glktermw/fizmo-glktermw $(bindir)

clean:
	cd src/fizmo-glktermw ; make clean

distclean: clean
	rm -f fizmo-glktermw
	cd src/fizmo-glktermw ; make distclean

