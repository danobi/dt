include config.mk

SRC = dt.c
OBJ = ${SRC:.c=.o}

all: dt

options:
	@echo dt build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

dt: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	rm -f dt ${OBJ}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dt ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dt

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	rm -f ${DESTDIR}${PREFIX}/bin/dt

.PHONY: all clean install uninstall
