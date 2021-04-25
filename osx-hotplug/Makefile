LANG=C
UNAME := $(shell uname)
PREFIX= /usr
PREFIX= $(DESTDIR)/usr
BIN=    $(PREFIX)/bin
MANDIR=$(PREFIX)/share/man
INSTALL=install
RC=/etc/rc.local

.SUFFIXES: .1 .1in

ifeq ($(UNAME),Darwin)
all:	osx-hplj-hotplug
else
all:
endif

osx-hplj-hotplug: osx-hplj-hotplug.o
	gcc osx-hplj-hotplug.o -o $@ \
	    -framework IOKit \
	    -framework Foundation \
	    -framework CoreFoundation

osx-hplj-hotplug.o: osx-hplj-hotplug.m
	cc -DPREFIX=\"$(PREFIX)\" -c -o osx-hplj-hotplug.o osx-hplj-hotplug.m

man:	osx-hplj-hotplug.1

.1in.1: 
	-rm -f $*.1
	MODver=$(VERSION); \
	../includer-man -v DEF1=$(OLDGROFF) $*.1in | sed > $*.1 \
	    -e "s@\$${URLOAK}@$(URLOAK)@" \
	    -e "s@\$${URLZJS}@$(URLZJS)@" \
	    -e "s@\$${URLHP}@$(URLHP)@" \
	    -e "s@\$${URLXQX}@$(URLXQX)@" \
	    -e "s@\$${URLLAVA}@$(URLLAVA)@" \
	    -e "s@\$${URLQPDL}@$(URLQPDL)@" \
	    -e "s@\$${URLSLX}@$(URLSLX)@" \
	    -e "s@\$${URLHC}@$(URLHC)@" \
	    -e "s/\$${MODver}/$$MODver/"
	chmod a-w $*.1

install: all
	$(INSTALL) -c osx-hplj-hotplug $(BIN)/

install-hotplug:
	$(INSTALL) -c local.localhost.startup.plist /Library/LaunchDaemons/
	( \
		echo "g/osx-hplj-hotplug/d"; \
		echo "\$$a"; \
		echo "# BEGIN osx-hplj-hotplug from foo2zjs"; \
		echo "	killall osx-hplj-hotplug"; \
		echo "	osx-hplj-hotplug >/tmp/osx-hplj-hotplug 2>&1 &"; \
		echo "# END osx-hplj-hotplug from foo2zjs"; \
		echo "."; \
		echo "w"; \
	) | ex $(RC)
	chmod +x $(RC)
	-killall osx-hplj-hotplug
	nohup osx-hplj-hotplug >/tmp/osx-hplj-hotplug 2>&1 &

install-man: man
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -d -m 755 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 osx-hplj-hotplug.1 $(MANDIR)/man1/

clean:
	rm -f *.o *.1

uninstall:
	( \
		echo "g/osx-hplj-hotplug/d"; \
		echo "w"; \
	) | ex $(RC)
	-rm -f $(BIN)/osx-hplj-hotplug
