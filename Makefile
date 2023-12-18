# slstatus-bar 
.POSIX:

include config.mk

REQ = util drw
COM =\
	components/battery\
	components/cat\
	components/cpu\
	components/datetime\
	components/disk\
	components/entropy\
	components/hostname\
	components/ip\
	components/kernel_release\
	components/keyboard_indicators\
	components/keymap\
	components/load_avg\
	components/netspeeds\
	components/num_files\
	components/ram\
	components/run_command\
	components/swap\
	components/temperature\
	components/uptime\
	components/user\
	components/volume\
	components/wifi

all: slstatus-bar

$(COM:=.o): config.mk $(REQ:=.h) slstatus-bar.h
slstatus-bar.o: slstatus-bar.c slstatus-bar.h arg.h config.h config.mk $(REQ:=.h)

.c.o:
	$(CC) -o $@ -c $(CPPFLAGS) $(CFLAGS) $<

config.h:
	cp config.def.h $@

slstatus-bar: slstatus-bar.o $(COM:=.o) $(REQ:=.o)
	$(CC) -o $@ $(LDFLAGS) $(COM:=.o) $(REQ:=.o) slstatus-bar.o $(LDLIBS)

clean:
	rm -f slstatus-bar slstatus-bar.o $(COM:=.o) $(REQ:=.o)

dist:
	rm -rf "slstatus-bar-$(VERSION)"
	mkdir -p "slstatus-bar-$(VERSION)/components"
	cp -R LICENSE Makefile README config.mk config.def.h \
	      arg.h slstatus-bar.c $(COM:=.c) $(REQ:=.c) $(REQ:=.h) \
	      slstatus-bar.1 "slstatus-bar-$(VERSION)"
	tar -cf - "slstatus-bar-$(VERSION)" | gzip -c > "slstatus-bar-$(VERSION).tar.gz"
	rm -rf "slstatus-bar-$(VERSION)"

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f slstatus-bar "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/slstatus-bar"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/slstatus-bar"
