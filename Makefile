include config.mk

SRC = tr.c
OBJ = ${SRC:.c=.o}

all: tr

options:
	@echo tr build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

tr: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	rm -f tr ${OBJ} 

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f tr ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/tr

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	rm -f ${DESTDIR}${PREFIX}/bin/tr

.PHONY: all clean install uninstall
