# dt version
VERSION = 1.0

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
LIBS = -lc -lncurses

CFLAGS = -g -std=c99 -pedantic -Wall -O0
#CFLAGS += -std=c99 -pedantic -Wall
LDFLAGS = -g ${LIBS}
#LDFLAGS += -s ${LIBS}

# compiler and linker
CC = cc
