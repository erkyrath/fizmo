
GROFF = groff
GS = gs -dNOPAUSE -dQUIET

all: plain-text html postscript pdf

plain-text: fizmo.txt fizmo-utf8.txt

fizmo.txt: fizmo-console.6
	"$(GROFF)" -Tascii -man fizmo-console.6 | ul -t dumb > fizmo-console.txt

fizmo-utf8.txt: fizmo-console.6
	"$(GROFF)" -Tutf8 -man fizmo-console.6 | ul -t dumb > fizmo-console-utf8.txt

html: fizmo.html

fizmo.html: fizmo-console.6
	groff -Thtml -man fizmo-console.6 > fizmo-console.html

postscript: fizmo-a4.ps fizmo-letter.ps

fizmo-a4.ps: fizmo-console.6
	groff -t -e -mandoc -Tps -dpaper=a4 -P-pa4 fizmo-console.6 > fizmo-console-a4.ps

fizmo-letter.ps: fizmo-console.6
	groff -t -e -mandoc -Tps -dpaper=letter -P-pletter fizmo-console.6 > fizmo-console-letter.ps

pdf: fizmo-a4.pdf fizmo-letter.pdf

fizmo-a4.pdf: fizmo-console.6
	groff -t -e -mandoc -Tps -dpaper=a4 -P-pa4 fizmo-console.6 | "$(GS)" -sDEVICE=pdfwrite -sOutputFile=fizmo-console-a4.pdf

fizmo-letter.pdf: fizmo-console.6
	groff -t -e -mandoc -Tps -dpaper=letter -P-pletter fizmo-console.6 | "$(GS)" -sDEVICE=pdfwrite -sOutputFile=fizmo-console-letter.pdf

clean:
	rm -f fizmo-console.txt fizmo-console-utf8.txt fizmo-console.html fizmo-console-a4.ps fizmo-console-letter.ps fizmo-console-a4.pdf fizmo-console-letter.pdf

