# slstatus version
VERSION = 0

# customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

INCS = -I/usr/include/X11 -I/usr/include/freetype2
LIBS = -lX11 -lfontconfig -lfreetype -lXft -lXrender

# flags
CPPFLAGS = $(INCS) -D_DEFAULT_SOURCE -DALSA
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Os
LDFLAGS  = $(LIBS) -s -lX11 -lfontconfig -lfreetype -lXft -lXrender
# OpenBSD: add -lsndio
# FreeBSD: add -lkvm -lsndio
LDLIBS   = -lX11

# compiler and linker
CC = cc
