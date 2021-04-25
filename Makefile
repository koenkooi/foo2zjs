LANG=C
UNAME := $(shell uname)
MACH := $(shell uname -m | sed 's/i.86/x86_32/')

ifeq ($(UNAME),SunOS)
    EUID := $(shell /usr/xpg4/bin/id -u)
    SHELL=bash
    CC=gcc
    OLDGROFF=OLDGROFF
else
    EUID := $(shell id -u)
endif

SYSNAME := $(shell uname -n)

# No version number yet...
VERSION=0.0

# Installation prefix...
PREFIX=/usr/local
PREFIX=/usr
PREFIX=$(DESTDIR)/usr

# Pathnames for this package...
BIN=$(PREFIX)/bin
SHAREZJS=$(PREFIX)/share/foo2zjs
SHAREOAK=$(PREFIX)/share/foo2oak
SHAREHP=$(PREFIX)/share/foo2hp
SHAREXQX=$(PREFIX)/share/foo2xqx
SHARELAVA=$(PREFIX)/share/foo2lava
SHAREQPDL=$(PREFIX)/share/foo2qpdl
SHARESLX=$(PREFIX)/share/foo2slx
SHAREHC=$(PREFIX)/share/foo2hiperc
SHAREHBPL=$(PREFIX)/share/foo2hbpl
SHAREDDST=$(PREFIX)/share/foo2ddst
MANDIR=$(PREFIX)/share/man
DOCDIR=$(PREFIX)/share/doc/foo2zjs/
INSTALL=install
ROOT=root

# Pathnames for referenced packages...
FOODB=$(DESTDIR)/usr/share/foomatic/db/source

# User ID's
LPuid=-oroot
LPgid=-glp
ifeq ($(UNAME),Darwin)
    LPuid=-oroot
    LPgid=-gwheel
    #ROOT=sudo
endif
ifeq ($(UNAME),FreeBSD)
    LPuid=-oroot
    LPgid=-gwheel
endif
ifeq ($(UNAME),OpenBSD)
    LPuid=-oroot
    LPgid=-gwheel
endif
ifeq ($(UNAME),SunOS)
    LPuid=-oroot
    LPgid=-glp
    INSTALL=/usr/ucb/install
endif
# If we aren't root, don't try to set ownership
ifneq ($(EUID),0)
    LPuid=
    LPgid=
endif

# Definition of modtime()
MODTIME= date -d "1/1/1970 utc + `stat -t $$1 | cut -f14 -d' '` seconds" "+%a %b %d %T %Y"
ifeq ($(UNAME),FreeBSD)
    MODTIME= stat -f "%Sm" -t "%a %b %d %T %Y" $$1
endif
ifeq ($(UNAME),OpenBSD)
    MODTIME= stat -f "%Sm" -t "%a %b %d %T %Y" $$1
endif
ifeq ($(UNAME),Darwin)
    MODTIME= stat -f "%Sm" -t "%a %b %d %T %Y" $$1
endif
ifeq ($(UNAME),SunOS)
    MODTIME= `ls -e $$1 | cut -c42-61`
endif

#
# Files for tarball
#
NULL=
WEBFILES=	\
		foo2zjs.html.in \
		style.css \
		archzjs.fig \
		2300.png \
		2430.png \
		1020.png \
		foo2oak.html.in \
		archoak.fig \
		1500.gif \
		foo2hp.html.in \
		archhp.fig \
		2600.gif \
		foo2xqx.html.in \
		archxqx.fig \
		m1005.gif \
		foo2lava.html.in \
		archlava.fig \
		2530.gif \
		foo2qpdl.html.in \
		archqplp.fig \
		foo2slx.html.in \
		archslx.fig \
		c500n.png \
		foo2hiperc.html.in \
		archhiperc.fig \
		c3400n.png \
		$(NULL)
	
FILES	=	\
		README \
		README.in \
		INSTALL \
		INSTALL.in \
		INSTALL.osx \
		INSTALL.usb \
		COPYING \
		ChangeLog \
		Makefile \
		foo2zjs.c \
		foo2zjs.1in \
		jbig.c \
		jbig.h \
		jbig_ar.c \
		jbig_ar.h \
		zjsdecode.c \
		zjsdecode.1in \
		zjs.h \
		foo2hp.c \
		foo2hp.1in \
		foo2xqx.c \
		foo2xqx.1in \
		foo2lava.c \
		foo2lava.1in \
		foo2qpdl.c \
		foo2qpdl.1in \
		foo2slx.c \
		foo2slx.1in \
		foo2hiperc.c \
		foo2hiperc.1in \
		hbpl.h \
		foo2hbpl2.c \
		foo2hbpl2.1in \
		foo2ddst.c \
		foo2ddst.1in \
		cups.h \
		xqx.h \
		xqxdecode.c \
		xqxdecode.1in \
		lavadecode.c \
		lavadecode.1in \
		qpdl.h \
		qpdldecode.c \
		qpdldecode.1in \
		opldecode.c \
		opldecode.1in \
		slx.h \
		slxdecode.c \
		slxdecode.1in \
		gipddecode.c \
		gipddecode.1in \
		hbpldecode.c \
		hbpldecode.1in \
		ddst.h \
		ddstdecode.c \
		ddstdecode.1in \
		foo2zjs-wrapper.in \
		foo2zjs-wrapper.1in \
		foo2hp2600-wrapper.in \
		foo2hp2600-wrapper.1in \
		foo2xqx-wrapper.in \
		foo2xqx-wrapper.1in \
		foo2lava-wrapper.in \
		foo2lava-wrapper.1in \
		foo2qpdl-wrapper.in \
		foo2qpdl-wrapper.1in \
		foo2slx-wrapper.in \
		foo2slx-wrapper.1in \
		foo2hiperc-wrapper.in \
		foo2hiperc-wrapper.1in \
		foo2hbpl2-wrapper.in \
		foo2hbpl2-wrapper.1in \
		foo2ddst-wrapper.in \
		foo2ddst-wrapper.1in \
		gamma.ps \
		gamma-lookup.ps \
		align.ps \
		testpage.ps \
		foomatic-db/*/*.xml \
		foomatic-test \
		getweb.in \
		icc2ps/*.[ch] \
		icc2ps/*.1in \
		icc2ps/Makefile \
		icc2ps/AUTHORS \
		icc2ps/COPYING \
		icc2ps/README \
		icc2ps/README.foo2zjs \
		osx-hotplug/Makefile \
		osx-hotplug/*.m \
		osx-hotplug/*.1in \
		osx-hotplug/*.plist \
		ppd-adjust \
		PPD/*.ppd \
		crd/zjs/*.crd \
		crd/zjs/*.ps \
		crd/qpdl/*cms* \
		crd/qpdl/*.ps \
		arm2hpdl.c \
		arm2hpdl.1in \
		usb_printerid.c \
		usb_printerid.1in \
		hplj1000 \
		hplj10xx.rules* \
		msexpand \
		oak.h \
		foo2oak.c \
		foo2oak.1in \
		oakdecode.c \
		oakdecode.1in \
		foo2oak-wrapper.in \
		foo2oak-wrapper.1in \
		hiperc.h \
		hipercdecode.c \
		hipercdecode.1in \
		c5200mono.prn \
		foo2zjs-pstops.sh \
		foo2zjs-pstops.1in \
		hplj1020.desktop \
		hplj1020_icon.png \
		hplj1020_icon.gif \
		hplj10xx_gui.tcl \
		includer \
		macros.man \
		regress.txt \
		printer-profile.sh \
		printer-profile.1in \
		freebsd-install \
		hplj10xx.conf \
		modify-ppd \
		command2foo2lava-pjl.c \
		myftpput \
		SmartInstallDisable-Tool.run \
		$(NULL)

# CUPS vars
CUPS_SERVERBIN := $(DESTDIR)$(shell cups-config --serverbin 2>/dev/null)
CUPS_DEVEL := $(shell grep cupsSideChannelDoRequest /usr/include/cups/sidechannel.h 2>/dev/null)
CUPS_GOODAPI := $(shell cups-config --api-version 2>/dev/null | sed "s/1\.[0123].*//")

# hpclj2600n-0.icm km2430_0.icm km2430_1.icm km2430_2.icm samclp300-0.icm
# sihp1000.img sihp1005.img sihp1020.img sihp1018.img
# sihpP1005.img sihpP1006.img sihpP1505.img

# Programs and libraries
PROGS=		foo2zjs zjsdecode arm2hpdl foo2hp foo2xqx xqxdecode
PROGS+=		foo2lava lavadecode foo2qpdl qpdldecode opldecode
PROGS+=		foo2oak oakdecode
PROGS+=		foo2slx slxdecode
PROGS+=		foo2hiperc hipercdecode
PROGS+=		foo2hbpl2 hbpldecode
PROGS+=		gipddecode
PROGS+=		foo2ddst ddstdecode
ifneq ($(CUPS_SERVERBIN),)
    ifneq ($(CUPS_DEVEL),)
	ifneq ($(CUPS_GOODAPI),)
	    PROGS+=	command2foo2lava-pjl
	endif
    endif
endif
SHELLS=		foo2zjs-wrapper foo2oak-wrapper foo2hp2600-wrapper \
		foo2xqx-wrapper foo2lava-wrapper foo2qpdl-wrapper \
		foo2slx-wrapper foo2hiperc-wrapper foo2hbpl2-wrapper \
		foo2ddst-wrapper
SHELLS+=	foo2zjs-pstops
SHELLS+=	printer-profile
MANPAGES=	foo2zjs-wrapper.1 foo2zjs.1 zjsdecode.1
MANPAGES+=	foo2oak-wrapper.1 foo2oak.1 oakdecode.1
MANPAGES+=	foo2hp2600-wrapper.1 foo2hp.1
MANPAGES+=	foo2xqx-wrapper.1 foo2xqx.1 xqxdecode.1
MANPAGES+=	foo2lava-wrapper.1 foo2lava.1 lavadecode.1 opldecode.1
MANPAGES+=	foo2qpdl-wrapper.1 foo2qpdl.1 qpdldecode.1
MANPAGES+=	foo2slx-wrapper.1 foo2slx.1 slxdecode.1
MANPAGES+=	foo2hiperc-wrapper.1 foo2hiperc.1 hipercdecode.1
MANPAGES+=	foo2hbpl2-wrapper.1 foo2hbpl2.1 hbpldecode.1
MANPAGES+=	foo2ddst-wrapper.1 foo2ddst.1 ddstdecode.1
MANPAGES+=	gipddecode.1
MANPAGES+=	foo2zjs-pstops.1 arm2hpdl.1 usb_printerid.1
MANPAGES+=	printer-profile.1
LIBJBG	=	jbig.o jbig_ar.o
BINPROGS=

ifeq ($(UNAME),Linux)
	BINPROGS += usb_printerid
endif

# Compiler flags
#CFLAGS +=	-O2 -Wall -Wno-unused-but-set-variable
CFLAGS +=	-O2 -Wall 
#CFLAGS +=	-g

#
# Rules to create test documents
#
GX=10200
GY=6600
GXR=1200
GYR=600
GSOPTS=	-q -dBATCH -dSAFER -dQUIET -dNOPAUSE -sPAPERSIZE=letter -r$(GXR)x$(GYR)
JBGOPTS=-m 16 -d 0 -p 92	# Equivalent options for pbmtojbg

.SUFFIXES: .ps .pbm .pgm .pgm2 .ppm .ppm2 .zjs .cmyk .pksm .zc .zm .jbg \
	   .cups .cupm .1 .1in .fig .gif .xqx .lava .qpdl .slx .hc .hbpl .ddst

.fig.gif:
	fig2dev -L gif $*.fig | giftrans -t "#ffffff" -o $*.gif

# old .fig.gif
# fig2dev -L ppm  $*.fig | pnmquant -fs 256 \
# | ppmtogif -transparent rgb:ff/ff/ff >$*.gif

.ps.cups:
	gs $(GSOPTS) -r600x600 \
	    -dcupsColorSpace=6 -dcupsBitsPerColor=2 -dcupsColorOrder=2 \
	    -sDEVICE=cups -sOutputFile=$*.cups $*.ps

.ps.cupm:
	gs $(GSOPTS) -r600x600 \
	    -dcupsColorSpace=3 -dcupsBitsPerColor=2 -dcupsColorOrder=2 \
	    -sDEVICE=cups -sOutputFile=$*.cupm $*.ps

.ps.pbm:
	gs $(GSOPTS) -sDEVICE=pbmraw -sOutputFile=$*.pbm $*.ps

.ps.ppm:
	gs $(GSOPTS) -sDEVICE=ppmraw -sOutputFile=$*.ppm $*.ps

.ps.pgm:
	gs $(GSOPTS) -sDEVICE=pgmraw -sOutputFile=- $*.ps \
	| pnmdepth 3 > $*.pgm

.ps.pgm2:
	gs.rick $(GSOPTS) -sDEVICE=pgmraw2 -sOutputFile=$*.pgm2 $*.ps

.ps.cmyk:
	gs $(GSOPTS) -sDEVICE=bitcmyk -sOutputFile=$*.cmyk $*.ps

.ps.pksm:
	gs $(GSOPTS) -sDEVICE=pksmraw -sOutputFile=$*.pksm $*.ps

.ps.zc:
	gs $(GSOPTS) -sDEVICE=bitcmyk -sOutputFile=- - < $*.ps \
	| ./foo2zjs -r1200x600 -g10200x6600 -p1 >$*.zc

.ps.zm:
	gs $(GSOPTS) -sDEVICE=pbmraw -sOutputFile=- - < $*.ps \
	| ./foo2zjs -r1200x600 -g10200x6600 -p1 >$*.zm

.pbm.zjs:
	./foo2zjs < $*.pbm > $*.zjs

.cmyk.zjs:
	./foo2zjs < $*.cmyk > $*.zjs

.pksm.zjs:
	./foo2zjs < $*.pksm > $*.zjs

.pbm.xqx:
	./foo2xqx < $*.pbm > $*.xqx

.pbm.qpdl:
	./foo2qpdl < $*.pbm > $*.qpdl

.pbm.slx:
	./foo2slx < $*.pbm > $*.slx

.pbm.hc:
	./foo2hiperc < $*.pbm > $*.hc

.pbm.hbpl:
	./foo2hbpl2 < $*.pbm > $*.hbpl

.pbm.ddst:
	./foo2ddst < $*.pbm > $*.ddst

#
# The usual build rules
#
all:	all-test $(PROGS) $(BINPROGS) $(SHELLS) getweb \
	all-icc2ps all-osx-hotplug man doc \
	all-done

MACOSX_stdio=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/stdio.h

all-test:
	#
	# Dependencies...
	#
	@if ! type $(CC) >/dev/null 2>&1; then \
	    echo "      ***"; \
	    echo "      *** Error: $(CC) is not installed!"; \
	    echo "      ***"; \
	    echo "      *** Install Software Development (gcc) package"; \
	    echo "      ***"; \
	    echo "      *** sudo dnf install gcc OR sudo apt-get install build-essential"; \
	    echo "      ***"; \
	    exit 1; \
	fi
	@if [ "`ls $(MACOSX_stdio) 2> /dev/null`" != "" ]; then \
	    : ; \
	elif ! test -f /usr/include/stdio.h; then \
	    echo "      ***"; \
	    echo "      *** Error: /usr/include/stdio.h is not installed!"; \
	    echo "      ***"; \
	    echo "      *** Install Software Development (gcc) package"; \
	    echo "      ***"; \
	    echo "      *** for Ubuntu: sudo apt-get install build-essential"; \
	    echo "      ***"; \
	    exit 1; \
	fi
	@if ! type gs >/dev/null 2>&1; then \
	    echo "      ***"; \
	    echo "      *** Error: gs is not installed!"; \
	    echo "      ***"; \
	    echo "      *** Install ghostscript (gs) package"; \
	    echo "      ***"; \
	    echo "      *** sudo dnf install ghostscript OR sudo apt-get install ghostscript"; \
	    echo "      ***"; \
	    exit 1; \
	fi
	@if ! type dc >/dev/null 2>&1; then \
	    echo "      ***"; \
	    echo "      *** Error: dc is not installed!"; \
	    echo "      ***"; \
	    echo "      *** Install dc/bc package"; \
	    echo "      ***"; \
	    echo "      *** sudo dnf install bc OR sudo apt-get install dc"; \
	    echo "      ***"; \
	    exit 1; \
	fi
	@if ! dc -V >/dev/null 2>&1; then \
	    echo "      ***"; \
	    echo "      *** Error: must install GNU dc with the -e option!"; \
	    echo "      ***"; \
	    echo "      *** Install dc/bc package"; \
	    echo "      ***"; \
	    echo "      *** sudo dnf install bc OR sudo apt-get install dc"; \
	    echo "      ***"; \
	    exit 1; \
	fi
ifeq ($(UNAME),Darwin)
	@if ! type gsed >/dev/null 2>&1; then \
	    echo "      ***"; \
	    echo "      *** Error: gsed is not installed!"; \
	    echo "      ***"; \
	    echo "      *** For OSX: sudo port install gsed"; \
	    echo "      ***"; \
	    exit 1; \
	fi
endif
	# ... OK!
	#

all-done:
	@echo
	@echo "It is possible that certain products which can be built using this"
	@echo "software module might form inventions protected by patent rights in"
	@echo "some countries (e.g., by patents about arithmetic coding algorithms"
	@echo "owned by IBM and AT&T in the USA). Provision of this software by the"
	@echo "author does NOT include any licences for any patents. In those"
	@echo "countries where a patent licence is required for certain applications"
	@echo "of this software module, you will have to obtain such a licence"
	@echo "yourself."


foo2ddst: foo2ddst.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2ddst.o $(LIBJBG)

foo2hbpl2: foo2hbpl2.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2hbpl2.o $(LIBJBG)

foo2hp: foo2hp.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2hp.o $(LIBJBG)

foo2hiperc: foo2hiperc.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2hiperc.o $(LIBJBG)

foo2lava: foo2lava.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2lava.o $(LIBJBG)

foo2oak: foo2oak.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2oak.o $(LIBJBG)

foo2qpdl: foo2qpdl.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2qpdl.o $(LIBJBG)

foo2slx: foo2slx.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2slx.o $(LIBJBG)

foo2xqx: foo2xqx.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2xqx.o $(LIBJBG)

foo2zjs: foo2zjs.o $(LIBJBG)
	$(CC) $(CFLAGS) -o $@ foo2zjs.o $(LIBJBG)


foo2ddst-wrapper: foo2ddst-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2hbpl2-wrapper: foo2hbpl2-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2hp2600-wrapper: foo2hp2600-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2hiperc-wrapper: foo2hiperc-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2lava-wrapper: foo2lava-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2qpdl-wrapper: foo2qpdl-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2oak-wrapper: foo2oak-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2slx-wrapper: foo2slx-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2xqx-wrapper: foo2xqx-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2zjs-wrapper: foo2zjs-wrapper.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

foo2zjs-wrapper9: foo2zjs-wrapper9.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@


getweb: getweb.in Makefile
	[ ! -f $@ ] || chmod +w $@
	sed < $@.in > $@ \
	    -e "s@\$${URLZJS}@$(URLZJS)@" \
	    -e 's@^PREFIX=.*@PREFIX=$(PREFIX)@' || (rm -f $@ && exit 1)
	chmod 555 $@

all-icc2ps:
	cd icc2ps; $(MAKE) all

all-osx-hotplug:
ifeq ($(UNAME),Darwin)
	cd osx-hotplug; $(MAKE) all
endif

ok: ok.o $(LIBJBG)
	$(CC) $(CFLAGS) ok.o $(LIBJBG) -o $@

ddstdecode: ddstdecode.o $(LIBJBG)
	$(CC) $(CFLAGS) ddstdecode.o $(LIBJBG) -o $@

gipddecode: gipddecode.o $(LIBJBG)
	$(CC) $(CFLAGS) gipddecode.o $(LIBJBG) -o $@

hbpldecode: hbpldecode.o $(LIBJBG)
	$(CC) $(CFLAGS) hbpldecode.o $(LIBJBG) -o $@

hipercdecode: hipercdecode.o $(LIBJBG)
	$(CC) $(CFLAGS) hipercdecode.o $(LIBJBG) -o $@

lavadecode: lavadecode.o $(LIBJBG)
	$(CC) $(CFLAGS) lavadecode.o $(LIBJBG) -o $@

oakdecode: oakdecode.o $(LIBJBG)
	$(CC) $(CFLAGS) -g oakdecode.o $(LIBJBG) -o $@

opldecode: opldecode.o $(LIBJBG)
	$(CC) $(CFLAGS) -g opldecode.o $(LIBJBG) -o $@

qpdldecode: qpdldecode.o $(LIBJBG)
	$(CC) $(CFLAGS) qpdldecode.o $(LIBJBG) -o $@

splcdecode: splcdecode.o $(LIBJBG)
	$(CC) $(CFLAGS) splcdecode.o $(LIBJBG) -lz -o $@

slxdecode: slxdecode.o $(LIBJBG)
	$(CC) $(CFLAGS) slxdecode.o $(LIBJBG) -o $@

xqxdecode: xqxdecode.o $(LIBJBG)
	$(CC) $(CFLAGS) xqxdecode.o $(LIBJBG) -o $@

zjsdecode: zjsdecode.o $(LIBJBG)
	$(CC) $(CFLAGS) zjsdecode.o $(LIBJBG) -o $@

command2foo2lava-pjl: command2foo2lava-pjl.o
	$(CC) $(CFLAGS) -L/usr/local/lib command2foo2lava-pjl.o -lcups -o $@

command2foo2lava-pjl.o: command2foo2lava-pjl.c
	$(CC) $(CFLAGS) -I/usr/local/include -c command2foo2lava-pjl.c

#
# Installation rules
#
install: all install-test install-prog install-icc2ps install-osx-hotplug \
	    install-extra install-crd install-foo install-ppd \
	    install-gui install-desktop install-filter \
	    install-man install-doc install-aa
	#
	# If you use CUPS, then restart the spooler:
	#	make cups
	#
	# Now use your printer configuration GUI to create a new printer.
	#
	# On Redhat 7.2/7.3/8.0/9.0 and Fedora Core 1-5, run "printconf-gui".
	# On Fedora 6/7/.../28, run "system-config-printer".
	# On Mandrake, run "printerdrake"
	# On Suse 9.x/10.x/11.x, run "yast"
	# On Ubuntu 5.10/6.06/6.10/7.04, run "gnome-cups-manager"
	# On Ubuntu 7.10/8.x/.../18.x, run "system-config-printer".

install-test:
	#
	# Installation Dependencies...
	#
	@if [ -f /usr/local/libexec/cups/filter/foomatic-rip ]; then \
	    : ; \
	elif [ -f /usr/libexec/cups/filter/foomatic-rip ]; then \
	    : ; \
	elif [ -f /usr/lib/cups/filter/foomatic-rip ]; then \
	    : ; \
	elif [ -f /usr/lib/lp/bin/foomatic-rip ]; then \
	    : ; \
	elif ! type foomatic-rip >/dev/null 2>&1; then \
	    echo "      ***"; \
	    echo "      *** Error: foomatic-rip is not installed!"; \
	    echo "      ***"; \
	    echo "      *** Install foomatic package(s) for your OS"; \
	    echo "      ***"; \
	    echo "      *** sudo dnf install cups-filters OR sudo apt-get install cups-filters"; \
	    echo "      ***"; \
	    exit 1; \
	fi
	# ... OK!
	#
    

UDEVBIN=$(DESTDIR)/bin/

install-prog:
	#
	# Install driver, wrapper, and development tools
	#
	$(INSTALL) -d $(BIN)
	$(INSTALL) -c $(PROGS) $(SHELLS) $(BIN)/
	if [ "$(BINPROGS)" != "" ]; then \
	    $(INSTALL) -d $(UDEVBIN); \
	    $(INSTALL) -c $(BINPROGS) $(UDEVBIN); \
	fi
	#
	# Install gamma correction files.  These are just templates,
	# and don't actually do anything right now.  If anybody wants
	# to tune them or point me at a process for doing that, please...
	#
	$(INSTALL) -d $(SHAREZJS)/
	$(INSTALL) -c -m 644 gamma.ps $(SHAREZJS)/
	$(INSTALL) -c -m 644 gamma-lookup.ps $(SHAREZJS)/
	$(INSTALL) -d $(SHAREOAK)/
	$(INSTALL) -d $(SHAREHP)/
	$(INSTALL) -d $(SHAREXQX)/
	$(INSTALL) -d $(SHARELAVA)/
	$(INSTALL) -d $(SHAREHC)/

install-foo:
	#
	# Remove obsolete foomatic database files from previous versions
	#
	rm -f $(FOODB)/opt/foo2zjs-Media.xml
	rm -f $(FOODB)/opt/foo2zjs-PaperSize.xml
	rm -f $(FOODB)/opt/foo2zjs-Source.xml
	rm -f $(FOODB)/opt/foo2zjs-DitherPPI.xml
	rm -f $(FOODB)/opt/foo2zjs-Copies.xml
	rm -f $(FOODB)/opt/foo2zjs-Nup.xml
	rm -f $(FOODB)/opt/foo2zjs-NupOrient.xml
	rm -f $(FOODB)/opt/foo2*-Quality.xml
	rm -f $(FOODB)/opt/foo2hp-AlignCMYK.xml
	rm -f $(FOODB)/printer/KonicaMinolta*.xml
	#
	# Install current database files
	#
	@if [ -d $(FOODB) ]; then \
	    for dir in driver printer opt; do \
		echo install -d $(FOODB)/$$dir/; \
		$(INSTALL) -d $(FOODB)/$$dir/; \
		echo install -m 644 foomatic-db/$$dir/*.xml $(FOODB)/$$dir/; \
		$(INSTALL) -c -m 644 foomatic-db/$$dir/*.xml $(FOODB)/$$dir/; \
	    done \
	else \
	    echo "***"; \
	    echo "*** WARNING! You don't have directory $(FOODB)/"; \
	    echo "*** If you want support for foomatic printer configuration,";\
	    echo "*** then you will have to manually install these files..."; \
	    echo "***"; \
	    ls foomatic-db/*/*.xml | sed 's/^/	/'; \
	    echo "***"; \
	    echo "*** ... wherever foomatic is stashed on your machine."; \
	    echo "***"; \
	fi
	#
	# Clear foomatic cache and rebuild database if needed
	#
	rm -rf /var/cache/foomatic/*/*
	rm -f /var/cache/foomatic/printconf.pickle
	if [ -d /var/cache/foomatic/compiled ]; then \
	    cd /var/cache/foomatic/compiled; \
	    foomatic-combo-xml -O >overview.xml; \
	fi

install-icc2ps:
	#
	# Install ICM to Postscript file conversion utility
	#
	cd icc2ps; $(MAKE) PREFIX=$(PREFIX) install

install-osx-hotplug:
ifeq ($(UNAME),Darwin)
	#
	# Install Mac OSX hotplug utility
	#
	cd osx-hotplug; $(MAKE) PREFIX=$(PREFIX) install
endif

install-crd:
	#
	# Install prebuilt CRD files (from m2300w project)
	#
	$(INSTALL) -d $(SHAREZJS)/
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREZJS)/crd/
	for i in crd/zjs/*.*; do \
	    $(INSTALL) -c -m 644 $$i $(SHAREZJS)/crd/; \
	done
	#
	# Install prebuilt CRD files for CLP-300/CLP-600
	#
	$(INSTALL) -d $(SHAREQPDL)/
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREQPDL)/crd/
	for i in crd/qpdl/*cms* crd/qpdl/*.ps; do \
	    $(INSTALL) -c -m 644 $$i $(SHAREQPDL)/crd/; \
	done

install-psfiles:
	#
	# Install prebuilt psfiles files (from m2300w project)
	#
	$(INSTALL) -d $(SHAREHP)/
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREHP)/psfiles/
	for i in psfiles/*.*; do \
	    $(INSTALL) -c -m 644 $$i $(SHAREHP)/psfiles/; \
	done

install-extra:
	#
	# Install extra files (ICM and firmware), if any exist here.
	#
	# Get files from the printer manufacturer, i.e. www.minolta-qms.com,
	# or use the "./getweb" convenience script.
	#
	$(INSTALL) -d $(SHAREZJS)/
	# foo2zjs ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREZJS)/icm/
	for i in DL*.icm CP*.icm km2430*.icm hp-cp1025*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHAREZJS)/icm/; \
	    fi; \
	done
	# foo2zjs Firmware files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREZJS)/firmware/
	for i in sihp1*.img; do \
	    if [ -f $$i ]; then \
		base=`basename $$i .img`; \
		./arm2hpdl $$i >$$base.dl; \
		$(INSTALL) -c -m 644 $$base.dl $(SHAREZJS)/firmware/; \
	    fi; \
	done
	# foo2xqx Firmware files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREXQX)/firmware/
	for i in sihpP*.img; do \
	    if [ -f $$i ]; then \
		base=`basename $$i .img`; \
		./arm2hpdl $$i >$$base.dl; \
		$(INSTALL) -c -m 644 $$base.dl $(SHAREXQX)/firmware/; \
	    fi; \
	done
	# foo2oak ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREOAK)/icm/
	for i in hpclj2[56]*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHAREOAK)/icm/; \
	    fi; \
	done
	# foo2hp ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREHP)/icm/
	for i in hpclj26*.icm km2430*.icm hp1215*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHAREHP)/icm/; \
	    fi; \
	done
	# foo2lava ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHARELAVA)/icm/
	for i in km-1600*.icm km2530*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHARELAVA)/icm/; \
	    fi; \
	done
	# foo2qpdl ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREQPDL)/icm/
	for i in samclp*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHAREQPDL)/icm/; \
	    fi; \
	done
	# foo2slx ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHARESLX)/icm/
	for i in lex*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHARESLX)/icm/; \
	    fi; \
	done
	# foo2hiperc ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREHC)/icm/
	for i in OK*.icm C3400*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHAREHC)/icm/; \
	    fi; \
	done
	# foo2hbpl ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREHBPL)/icm/
	for i in hbpl*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHAREHBPL)/icm/; \
	    fi; \
	done
	# foo2ddst ICM files (if any)
	$(INSTALL) $(LPuid) $(LPgid) -m 775 -d $(SHAREDDST)/icm/
	for i in ddst*.icm; do \
	    if [ -f $$i ]; then \
		$(INSTALL) -c -m 644 $$i $(SHAREDDST)/icm/; \
	    fi; \
	done

MODEL=$(PREFIX)/share/cups/model
LOCALMODEL=$(DESTDIR)/usr/local/share/cups/model
MACMODEL=/Library/Printers/PPDs/Contents/Resources
PPD=$(PREFIX)/share/ppd
VARPPD=/var/lp/ppd
install-ppd:
	#
	# Install PPD files for CUPS
	#
	export PATH=$$PATH:`pwd`:; \
	if [ -x /usr/sbin/ppdmgr -a -s $(VARPPD)/ppdcache ]; then \
	    $(INSTALL) $(LPgid) -d $(VARPPD)/user; \
	    cd PPD; \
	    for ppd in *.ppd; do \
		manuf=`echo "$$ppd" | sed 's/-.*//'`; \
		$(INSTALL) $(LPgid) -d $(VARPPD)/user/$$manuf; \
		modify-ppd <$$ppd | gzip > $(VARPPD)/user/$$manuf/$$ppd.gz; \
		chmod 664 $(VARPPD)/user/$$manuf/$$ppd.gz; \
	    done; \
	    ppdmgr -u; \
	elif [ -d $(PPD) ]; then \
	    find $(PPD) -name '*foo2zjs*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2hp*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2xqx*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2lava*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2qpdl*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2slx*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2hiperc*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2hbpl*' | xargs rm -rf; \
	    find $(PPD) -name '*foo2ddst*' | xargs rm -rf; \
	    [ -d $(PPD)/foo2zjs ] || mkdir $(PPD)/foo2zjs; \
	    cd PPD; \
	    for ppd in *.ppd; do \
		modify-ppd <$$ppd | gzip > $(PPD)/foo2zjs/$$ppd.gz; \
		chmod 664 $(PPD)/foo2zjs/$$ppd.gz; \
	    done; \
	fi
	#
	export PATH=$$PATH:`pwd`:; \
	if [ -d $(MODEL) ]; then \
	    rm -f $(MODEL)/KonicaMinolta*; \
	    cd PPD; \
	    for ppd in *.ppd; do \
		modify-ppd <$$ppd | gzip > $(MODEL)/$$ppd.gz; \
		chmod 664 $(MODEL)/$$ppd.gz; \
	    done; \
	elif [ -d $(LOCALMODEL) ]; then \
	    rm -f $(LOCALMODEL)/KonicaMinolta*; \
	    cd PPD; \
	    for ppd in *.ppd; do \
		modify-ppd <$$ppd | gzip > $(LOCALMODEL)/$$ppd.gz; \
		chmod 664 $(LOCALMODEL)/$$ppd.gz; \
	    done; \
	elif [ -d $(MACMODEL) ]; then \
	    rm -f $(MACMODEL)/KonicaMinolta*; \
	    cd PPD; \
	    for ppd in *.ppd; do \
		modify-ppd <$$ppd | gzip > $(MACMODEL)/$$ppd.gz; \
		chmod 664 $(MACMODEL)/$$ppd.gz; \
	    done; \
	fi

APPL=$(DESTDIR)/usr/share/applications
OLDAPPL=$(DESTDIR)/usr/share/gnome/apps/System
PIXMAPS=$(DESTDIR)/usr/share/pixmaps

install-desktop:
	#
	# Install GNOME desktop
	#
	if [ -d $(APPL) ]; then \
	    $(INSTALL) -c -m 644 hplj1020.desktop $(APPL); \
	fi
	if [ -d $(OLDAPPL) ]; then \
	    $(INSTALL) -c -m 644 hplj1020.desktop $(OLDAPPL); \
	fi
	if [ -d $(PIXMAPS) ]; then \
	    $(INSTALL) -c -m 644 hplj1020_icon.png $(PIXMAPS); \
	fi

install-gui:
	#
	# Install GUI
	#
	$(INSTALL) -c -m 644 hplj1020_icon.gif $(SHAREZJS)
	$(INSTALL) -c -m 755 hplj10xx_gui.tcl $(SHAREZJS)
	

USBDIR=/etc/hotplug/usb
UDEVDIR=/etc/udev/rules.d
LIBUDEVDIR=/lib/udev/rules.d
RULES=hplj10xx.rules
#UDEVD=/sbin/udevd
# For FreeBSD 8.0
DEVDDIR=/etc/devd

ifeq ($(UNAME),Darwin)
install-hotplug: install-hotplug-test install-hotplug-osx
else
install-hotplug: install-hotplug-test install-hotplug-prog
endif

install-hotplug-test:
	#
	# Hotplug Installation Dependencies...
	#
	@if ! type ex >/dev/null 2>&1; then \
	    echo "      ***"; \
	    echo "      *** Error: "ex" is not installed!"; \
	    echo "      ***"; \
	    echo "      *** Install "vim" package(s) for your OS"; \
	    echo "      ***"; \
	    exit 1; \
	fi
	@if test -r $(LIBUDEVDIR)/*-printers.rules; then \
	    echo "      ***"; \
	    echo "      *** Error: system-config-printer-udev is installed!"; \
	    echo "      ***"; \
	    echo "      *** Remove it with: (Fedora)"; \
	    echo "      *** 	# dnf remove system-config-printer-udev"; \
	    echo "      *** OR"; \
	    echo "      *** 	# rpm -e --nodeps system-config-printer-udev"; \
	    echo "      *** OR (Ubuntu, Debian)"; \
	    echo "      *** 	$$ sudo apt-get remove system-config-printer-udev"; \
	    echo "      *** OR (SUSE)"; \
	    echo "      *** 	# zypper rm udev-configure-printer"; \
	    echo "      *** OR (generic linux)"; \
	    echo "      ***	# rm -f $(LIBUDEVDIR)/*-printers.rules"; \
	    echo "      ***"; \
	    exit 1; \
	fi
	# ... OK!
	#

install-hotplug-prog:
	#
	#	remove HPLIP (proprietary) files and install our version
	#
	if [ -d $(UDEVDIR) ]; then \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_1000*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_1005*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_1018*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_1020*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_p1005*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_p1006*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_p1007*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_p1008*; \
	    rm -f $(UDEVDIR)/*hpmud*laserjet_p1505*; \
	    rm -f $(UDEVDIR)/*hpmud_support.rules; \
	    rm -f $(UDEVDIR)/*hpmud_plugin.rules; \
	    rm -f $(LIBUDEVDIR)/*hpmud_support.rules; \
	    rm -f $(LIBUDEVDIR)/*hpmud_plugin.rules; \
	    rm -f $(LIBUDEVDIR)/*-hplj10xx.rules; \
	    if [ -x /sbin/udevd ]; then \
		version=`/sbin/udevd --version 2>/dev/null`; \
	    elif [ -x /usr/lib/udev/udevd ]; then \
		version=`/usr/lib/udev/udevd --version 2>/dev/null`; \
	    elif [ -x /lib/systemd/systemd-udevd ]; then \
		version=`/lib/systemd/systemd-udevd --version 2>/dev/null`; \
	    elif [ -x /usr/lib/systemd/systemd-udevd ]; then \
		version=`/usr/lib/systemd/systemd-udevd --version 2>/dev/null`; \
	    fi; \
	    version=`echo $$version | sed -e 's/^v//' -e 's/-.*//' -e 's/\..*//'`; \
	    if [ "$$version" = "" ]; then version=0; fi; \
	    echo "***"; \
	    echo "*** udev version $$version"; \
	    echo "***"; \
	    if [ "$$version" -lt 148 ]; then \
		$(INSTALL) -c -m 644 $(RULES).old $(UDEVDIR)/11-$(RULES); \
	    else \
		$(INSTALL) -c -m 644 $(RULES) $(UDEVDIR)/11-$(RULES); \
	    fi \
	fi
	if [ -d $(DEVDDIR) ]; then \
	    $(INSTALL) -c -m 644 hplj10xx.conf $(DEVDDIR)/; \
	fi
	[ -d $(USBDIR) ] || $(INSTALL) -d -m 755 $(USBDIR)/
	$(INSTALL) -c -m 755 hplj1000 $(USBDIR)/
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hplj1005
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hplj1018
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hplj1020
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hpljP1005
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hpljP1006
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hpljP1007
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hpljP1008
	ln -sf $(USBDIR)/hplj1000 $(USBDIR)/hpljP1505
	$(USBDIR)/hplj1000 install-usermap
	$(USBDIR)/hplj1005 install-usermap
	$(USBDIR)/hplj1018 install-usermap
	$(USBDIR)/hplj1020 install-usermap
	$(USBDIR)/hpljP1005 install-usermap
	$(USBDIR)/hpljP1006 install-usermap
	$(USBDIR)/hpljP1007 install-usermap
	$(USBDIR)/hpljP1008 install-usermap
	$(USBDIR)/hpljP1505 install-usermap
	# modprobe usblp
	$(USBDIR)/hplj1000 install-usblp

install-hotplug-osx:
	cd osx-hotplug; $(MAKE) PREFIX=$(PREFIX) install-hotplug

install-filter:
	if [ "$(CUPS_SERVERBIN)" != "" ]; then \
	    $(INSTALL) -d $(CUPS_SERVERBIN)/filter; \
	    ln -sf $(BIN)/command2foo2lava-pjl $(CUPS_SERVERBIN)/filter/; \
	fi

install-aa:
	#
	# openSUSE tumbleweed distro breaks ghostscript with pipes!
	# 
	if [ -f /etc/apparmor.d/ghostscript ]; then \
	    aa-disable --no-reload ghostscript; \
	fi

uninstall-aa:
	if [ -f /etc/apparmor.d/ghostscript ]; then \
	    aa-enforce ghostscript; \
	fi

CUPSDCONF=/etc/cups/cupsd.conf
CUPSFILESCONF=/etc/cups/cups-files.conf
CUPSPRINTERS=/etc/cups/printers.conf
MACLOAD=/System/Library/LaunchDaemons/org.cups.cupsd.plist
# cups-config doesn't exist on Ubuntu unless apt-get install libcups2-dev ...
CUPSMAJVER=cups-config --version | sed "s/[.].*//"
# ... so we use another way
CUPSMAJVER=head -1 $(CUPSPRINTERS) | sed -e 's/.*CUPS v//' -e 's/\..*//'

cups:	FRC
	#
	# CUPS
	#
	if [ -r $(CUPSFILESCONF) ]; then \
	    (	echo "g/^FileDev/d"; \
		echo "g/ foo2zjs.../d"; \
	    	echo "g/^Sandboxing/d"; \
		echo '$$a'; \
		echo "# 'FileDevice Yes' line installed by foo2zjs..."; \
		echo "FileDevice Yes"; \
		if [ -r $(CUPSPRINTERS) ]; then \
		    CUPS_MAJVER=`$(CUPSMAJVER)`; \
		    if [ "$$CUPS_MAJVER" = 2 ]; then \
			echo "# 'Sandboxing Relaxed' installed by foo2zjs..."; \
			echo "Sandboxing Relaxed"; \
		    fi; \
		fi; \
		echo "."; \
		echo "w"; \
	    ) | ex $(CUPSFILESCONF); \
	elif [ -r $(CUPSDCONF) ]; then \
	    (	echo "g/^FileDev/d"; \
		echo "g/ foo2zjs.../d"; \
		echo '$$a'; \
		echo "# 'FileDevice Yes' line installed by foo2zjs..."; \
		echo "FileDevice Yes"; \
		echo "."; \
		echo "w"; \
	    ) | ex $(CUPSDCONF); \
	fi
	#
	# CUPS restart
	#
	if [ -x /etc/init.d/cups ]; then \
	    /etc/init.d/cups restart; \
	    if [ $$? != 0 ]; then \
		service cups restart; \
	    fi \
	elif [ -x /etc/rc.d/rc.cups ]; then \
	    /etc/rc.d/rc.cups restart; \
	elif [ -x /etc/init.d/cupsys ]; then \
	    /etc/init.d/cupsys restart; \
	elif [ -x /etc/init.d/cupsd ]; then \
	    /etc/init.d/cupsd restart; \
	elif [ -x /usr/local/etc/rc.d/cups.sh ]; then \
	    /usr/local/etc/rc.d/cups.sh restart; \
	elif [ -x /usr/local/etc/rc.d/cups.sh.sample ]; then \
	    cp /usr/local/etc/rc.d/cups.sh.sample /usr/local/etc/rc.d/cups.sh; \
	    /usr/local/etc/rc.d/cups.sh restart; \
	elif [ -x /bin/systemctl -o -x /usr/bin/systemctl ]; then \
	    systemctl restart cups.service \
		|| systemctl restart org.cups.cupsd.service; \
	elif [ -x /bin/launchctl ]; then \
	    /bin/launchctl unload $(MACLOAD); \
	    /bin/launchctl load $(MACLOAD); \
	else \
	    echo "***"; \
	    echo "*** Warning: I don't know how CUPS gets restarted!"; \
	    echo "***"; \
	fi

#
# Uninstall
#
uninstall: uninstall-aa
	cd osx-hotplug; $(MAKE) PREFIX=$(PREFIX) uninstall
	-rm -f /etc/hotplug/usb/hplj1000
	-rm -f /etc/hotplug/usb/hplj1005
	-rm -f /etc/hotplug/usb/hplj1018
	-rm -f /etc/hotplug/usb/hplj1020
	-rm -f /etc/hotplug/usb/foo2zjs.usermap
	-(echo "g/^hplj10[02][05]/d"; echo "w") | ex /etc/hotplug/usb.usermap
	-rm -f /etc/udev/rules.d/11-hplj10xx.rules
	-rm -f /usr/bin/usb_printerid /bin/usb_printerid /sbin/usb_printerid
	-rm -f /etc/hotplug/usb/hplj.usermap #
	-rm -f /etc/udev/rules.d/58-foo2zjs.rules #
	-rm -f /sbin/foo2zjs-loadfw #
	-rm -rf /usr/share/doc/foo2zjs/
	-rm -f $(MANDIR)/man1/foo2zjs*.1 $(MANDIR)/man1/zjsdecode.1
	-rm -f $(MANDIR)/man1/foo2hp*.1
	-rm -f $(MANDIR)/man1/foo2oak*.1 $(MANDIR)/man1/oakdecode.1
	-rm -f $(MANDIR)/man1/foo2lava*.1 $(MANDIR)/man1/lavadecode.1
	-rm -f $(MANDIR)/man1/foo2qpdl*.1 $(MANDIR)/man1/qpdldecode.1
	-rm -f $(MANDIR)/man1/foo2slx*.1 $(MANDIR)/man1/slxdecode.1
	-rm -f $(MANDIR)/man1/foo2xqx*.1 $(MANDIR)/man1/xqxdecode.1
	-rm -f $(MANDIR)/man1/opldecode.1 $(MANDIR)/man1/rodecode.1
	-rm -f $(MANDIR)/man1/foo2hiperc*.1 $(MANDIR)/man1/hipercdecode.1
	-rm -f $(MANDIR)/man1/foo2hbpl*.1 $(MANDIR)/man1/hbpldecode.1
	-rm -f $(MANDIR)/man1/foo2ddst*.1 $(MANDIR)/man1/ddstdecode.1
	-rm -f $(MANDIR)/man1/gipddecode.1
	-rm -f $(MANDIR)/man1/arm2hpdl.1 $(MANDIR)/man1/usb_printerid.1
	-rm -f $(MANDIR)/man1/foo2zjs-icc2ps.1
	-rm -rf /usr/share/foo2zjs/
	-rm -rf /usr/share/foo2hp/
	-rm -rf /usr/share/foo2oak/
	-rm -rf /usr/share/foo2xqx/
	-rm -rf /usr/share/foo2lava/
	-rm -rf /usr/share/foo2qpdl/
	-rm -rf /usr/share/foo2slx/
	-rm -rf /usr/share/foo2hiperc/
	-rm -rf /usr/share/foo2hbpl/
	-rm -rf /usr/share/foo2ddst/
	-rm -f /usr/bin/arm2hpdl
	-rm -f /usr/bin/foo2zjs-wrapper /usr/bin/foo2zjs /usr/bin/zjsdecode
	-rm -f /usr/bin/foo2oak-wrapper /usr/bin/foo2oak /usr/bin/oakdecode
	-rm -f /usr/bin/foo2hp2600-wrapper /usr/bin/foo2hp
	-rm -f /usr/bin/foo2xqx-wrapper /usr/bin/foo2xqx /usr/bin/xqxdecode
	-rm -f /usr/bin/foo2lava-wrapper /usr/bin/foo2lava /usr/bin/lavadecode
	-rm -f /usr/bin/foo2qpdl-wrapper /usr/bin/foo2qpdl /usr/bin/qpdldecode
	-rm -f /usr/bin/foo2slx-wrapper /usr/bin/foo2slx /usr/bin/slxdecode
	-rm -f /usr/bin/foo2hiperc-wrapper /usr/bin/foo2hiperc
	-rm -f /usr/bin/hipercdecode
	-rm -f /usr/bin/foo2hbpl2-wrapper /usr/bin/foo2hbpl2
	-rm -f /usr/bin/hbpldecode
	-rm -f /usr/bin/foo2ddst-wrapper /usr/bin/foo2ddst /usr/bin/ddstdecode
	-rm -f /usr/bin/gipddecode
	-rm -f /usr/bin/opldecode
	-rm -f /usr/bin/rodecode
	-rm -f /usr/bin/foo2zjs-icc2ps
	-rm -f /usr/bin/foo2zjs-pstops
	-rm -f /usr/bin/command2foo2lava-pjl
	-rm -f /usr/lib/cups/filter/command2foo2lava-pjl
	-rm -f /usr/share/applications/hplj1020.desktop
	-rm -f /usr/share/pixmaps/hplj1020_icon.png
	-cd foomatic-db; for i in `find driver opt printer -name "*.xml"`; do \
		rm -f $(FOODB)/$$i; \
	done
	cd PPD; for ppd in *.ppd; do \
	    rm -f $(MODEL)/$$ppd.gz; \
	done;
	-rm -f /var/cache/foomatic/printconf.pickle

#
# Clean
#
clean:
	-rm -f $(PROGS) $(BINPROGS) $(SHELLS)
	-rm -f *.zc *.zm *.zm1
	-rm -f xxx.* xxxomatic
	-rm -f foo2zjs.o jbig.o jbig_ar.o zjsdecode.o foo2hp.o
	-rm -f foo2oak.o oakdecode.o
	-rm -f foo2xqx.o xqxdecode.o
	-rm -f foo2lava.o lavadecode.o
	-rm -f foo2qpdl.o qpdldecode.o
	-rm -f foo2slx.o slxdecode.o
	-rm -f foo2hiperc.o hipercdecode.o
	-rm -f foo2hbpl2.o hbpldecode.o
	-rm -f opldecode.o gipddecode.o
	-rm -f foo2dsst.o ddstdecode.o
	-rm -f command2foo2lava-pjl.o
	-rm -f foo2oak.html foo2zjs.html foo2hp.html foo2xqx.html foo2lava.html
	-rm -f foo2slx.html foo2qpdl.html foo2hiperc.html foo2hbpl.html
	-rm -f foo2ddst.html
	-rm -f index.html
	-rm -f arch*.gif
	-rm -f sihp*.dl
	-rm -f *.tar.gz
	-rm -f getweb
	-rm -f patch.db
	-rm -f $(MANPAGES) manual.pdf
	-rm -f *.zjs *.zm *.zc *.zc? *.zc?? *.oak *.pbm *.pksm *.cmyk
	-rm -f pksm2bitcmyk
	-rm -f *.icm.*.ps
	cd icc2ps; $(MAKE) $@
	cd osx-hotplug; $(MAKE) $@

#
# Header dependencies
#
jbig.o: jbig.h

foo2ddst.o: jbig.h ddst.h
foo2hiperc.o: jbig.h hiperc.h
foo2hp.o: jbig.h zjs.h cups.h
foo2hbpl2.o: jbig.h hbpl.h
foo2lava.o: jbig.h
foo2oak.o: jbig.h oak.h
foo2qpdl.o: jbig.h qpdl.h
foo2slx.o: jbig.h slx.h
foo2xqx.o: jbig.h xqx.h
foo2zjs.o: jbig.h zjs.h

ddstdecode.o: ddst.h jbig.h
gipddecode.o: slx.h jbig.h
hbpldecode.o: jbig.h
hipercdecode.o: hiperc.h jbig.h
lavadecode.o: jbig.h
opldecode.o: jbig.h
qpdldecode.o: jbig.h
slxdecode.o: slx.h jbig.h
xqxdecode.o: xqx.h jbig.h
zjsdecode.o: jbig.h zjs.h

#
# foo2* Regression tests
#
test:		testzjs testhp
	@ls -l *.z* #*.oak
	#
	# All regression tests passed.
	#
	# Send the appropriate test page .zm/.zc ZjStream file(s) to
	# your printer using a *RAW* printer queue.

#
# foo2zjs Regression tests
#
testzjs:	testpage.zm \
		testpage.zc10 \
		lj1000.zm lj1020.zm
# testpage.zc1 testpage.zc2 testpage.zc3 \

testpage.zm: testpage.ps foo2zjs-wrapper foo2zjs Makefile FRC
	#
	# Tests will pass only if you are using ghostscript-8.71-16.fc14
	#
	# Monochrome test page for Minolta 2200/2300 DL
	PATH=.:$$PATH time -p foo2zjs-wrapper testpage.ps > $@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

testpage.zc10: testpage.ps foo2zjs-wrapper foo2zjs Makefile FRC
	#
	# Color test page for Minolta 2200/2300 DL
	PATH=.:$$PATH time -p foo2zjs-wrapper -c -C10 testpage.ps > $@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

testpage.zc1: testpage.ps foo2zjs-wrapper foo2zjs Makefile FRC
	PATH=.:$$PATH time -p foo2zjs-wrapper -c -C1 testpage.ps > $@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

testpage.zc2: testpage.ps foo2zjs-wrapper foo2zjs Makefile FRC
	PATH=.:$$PATH time -p foo2zjs-wrapper -c -C2 testpage.ps > $@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

testpage.zc3: testpage.ps foo2zjs-wrapper foo2zjs Makefile FRC
	PATH=.:$$PATH time -p foo2zjs-wrapper -c -C3 testpage.ps > $@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

lj1000.zm: testpage.ps foo2zjs-wrapper foo2zjs Makefile FRC
	#
	# Monochrome test page for HP LJ1000
	PATH=.:$$PATH time -p foo2zjs-wrapper -r600x600 -P testpage.ps >$@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

lj1020.zm: testpage.ps foo2zjs-wrapper foo2zjs Makefile FRC
	#
	# Monochrome test page for HP LJ1020
	PATH=.:$$PATH time -p foo2zjs-wrapper -r600x600 -P -z1 \
	    testpage.ps | sed "/JOBATTR/d" >$@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

#
# foo2hp Regression tests
#
testhp: lj2600.zm1 lj2600.zc1

lj2600.zm1: testpage.ps foo2hp2600-wrapper foo2hp Makefile FRC
	#
	# Monochrome test page for HP 2600n (1-bit)
	PATH=.:$$PATH time -p foo2hp2600-wrapper testpage.ps > $@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

lj2600.zc1: testpage.ps foo2hp2600-wrapper foo2hp Makefile FRC
	#
	# Color test page for HP 2600n (1-bit)
	PATH=.:$$PATH time -p foo2hp2600-wrapper -c testpage.ps > $@
	@got=`md5sum $@`; grep -q "$$got" regress.txt || \
	    { echo "*** Test failure, got $$got"; ls -l $@; exit 1; }

#
# foo2oak Regression tests
#
testoak: pprtest-0.oak pprtest-1.oak pprtest-2.oak pprtest-3.oak

pprtest-0.oak: FRC
	#
	# 1-bit Monochrome test page for OAKT
	PATH=.:$$PATH foo2oak-wrapper -b1 -D12345678 pprtest.ps > $@
	@want="fbd4c1a560985a6ad47ff23b018c7ce8  $@"; got=`md5sum $@`; \
	[ "$$want" = "$$got" ] || \
	    { echo "*** Test failure, got $$got"; exit 1; }

pprtest-1.oak: FRC
	#
	# 2-bit Monochrome test page for OAKT
	PATH=.:$$PATH foo2oak-wrapper -b2 -D12345678 pprtest.ps > $@
	@want="bec9a24ee1ce0d388b773f83609a4d01  $@"; got=`md5sum $@`; \
	[ "$$want" = "$$got" ] || \
	    { echo "*** Test failure, got $$got"; exit 1; }

pprtest-2.oak: FRC
	#
	# 1-bit color test page for OAKT
	PATH=.:$$PATH foo2oak-wrapper -c -b1 -D12345678 pprtest.ps > $@
	@want="c714bcd69fe5f3b2b257d7435eb938d1  $@"; got=`md5sum $@`; \
	[ "$$want" = "$$got" ] || \
	    { echo "*** Test failure, got $$got"; exit 1; }

pprtest-3.oak: FRC
	#
	# 2-bit color test page for OAKT
	PATH=.:$$PATH foo2oak-wrapper -c -b2 -D12345678 pprtest.ps > $@
	@want="ed89abcd873979bc9337e02263511964  $@"; got=`md5sum $@`; \
	[ "$$want" = "$$got" ] || \
	    { echo "*** Test failure, got $$got"; exit 1; }

#
#	icc2ps regression tests
#
ICC2PS=./icc2ps/foo2zjs-icc2ps
icctest:
	for g in *.icm; do \
	    for i in 0 1 2 3; do \
		$(ICC2PS) -o $$g -t$$i \
		| sed '/Created:/d' > $$g.$$i.ps; \
	    done; \
	done


#
# Make phony print devices for testing full spooler interface without printing
#
tmpdev:
	DEV=/tmp/OAK; > $$DEV; chmod 666 $$DEV
	DEV=/tmp/OAKCM; > $$DEV; chmod 666 $$DEV
	DEV=/tmp/testfile; > $$DEV; chmod 666 $$DEV

#
# Test files for debugging
#
testpage.pbm: testpage.ps Makefile
xxx.zc: FRC
xxx.zm: FRC

#
#	PPD files
#	
#	Don't edit the PPD files.  Instead, change the
#	foomatic/{device,printer,opt}/*.xml files or the "modify-ppd" script.
#
FOOPRINT=*.xml
ppd:
	#
	# Generate PPD files using local tools
	#
	[ -d PPD ] || mkdir PPD
	> foomatic-db/oldprinterids
	cd foomatic-db; rm -f db; ln -sf . db
	cd foomatic-db; rm -f source; ln -sf . source
	# for i in foomatic-db/printer/Samsung-ML*xml;
	for i in foomatic-db/printer/$(FOOPRINT); \
	do \
	    printer=`basename $$i .xml`; \
	    case "$$printer" in \
	    *"d-Color_P160"*)   driver=foo2hiperc;; \
	    *M1005*|*M1120*)    driver=foo2xqx;; \
	    *M1132*)		driver=foo2xqx;; \
	    *P1[05]0[5678]*)    driver=foo2xqx;; \
	    *P2014*)            driver=foo2xqx;; \
	    *M1212*)            driver=foo2xqx;; \
	    *1500*|*OAKT*)      driver=foo2oak;; \
	    *1018*|*102[02]*)	driver=foo2zjs-z1;; \
	    *P2035*)		driver=foo2zjs-z1;; \
	    *1319*)		driver=foo2zjs-z1;; \
	    *M12a|*M12w)	driver=foo2zjs-z2;; \
	    *P110*)		driver=foo2zjs-z2;; \
	    *P156*)		driver=foo2zjs-z2;; \
	    *P160*)		driver=foo2zjs-z2;; \
	    *CP102*)		driver=foo2zjs-z3;; \
	    *1635*|*2035*)      driver=foo2oak-z1;; \
	    *1600W|*16[89]0*)   driver=foo2lava;; \
	    *4690*)		driver=foo2lava;; \
	    *2530*|*24[89]0*)   driver=foo2lava;; \
	    *6115*)             driver=foo2lava;; \
	    *C110*)             driver=foo2lava;; \
	    *6121*)             driver=foo2lava;; \
	    *1600*|*2600*)      driver=foo2hp;; \
	    *1215*)		driver=foo2hp;; \
	    *C500*)             driver=foo2slx;; \
	    *C301*|*C310*)      driver=foo2hiperc;; \
	    *C511*)	        driver=foo2hiperc;; \
	    *C810*)             driver=foo2hiperc-z1;; \
	    *C3[1234]00*)       driver=foo2hiperc;; \
	    *C3530*)	        driver=foo2hiperc;; \
	    *C5[12568][05]0*)   driver=foo2hiperc;; \
	    *CLP*|*CLX*|*6110*) driver=foo2qpdl;; \
	    *ML-167*)		driver=foo2qpdl;; \
	    *6015*|*1355*)	driver=foo2hbpl2;; \
	    *C1765*)		driver=foo2hbpl2;; \
	    *CX17*)		driver=foo2hbpl2;; \
	    *CM2[01]5*)		driver=foo2hbpl2;; \
	    *P205*|*3045*)	driver=foo2hbpl2;; \
	    *3010*|*3040*)	driver=foo2hbpl2;; \
	    *M215*)		driver=foo2hbpl2;; \
	    *M1400*)		driver=foo2hbpl2;; \
	    *SP_*)		driver=foo2ddst;; \
	    *)                  driver=foo2zjs;; \
	    esac; \
	    echo $$driver - $$printer; \
	    if true; then \
		foomatic-ppdfile -d $$driver -p $$printer > PPD/$$printer.ppd; \
	    else \
		# 09/06/18: Use the older foomatic??? \
		ENGINE=../foomatic/foomatic-db-engine; \
		PERL5LIB=$$ENGINE/lib \
		    FOOMATICDB=foomatic-db \
		    $$ENGINE/foomatic-ppdfile \
		    -d $$driver -p $$printer \
		    > PPD/$$printer.ppd; \
	    fi \
	done

oldppd:
	# Did you do a "make install"????
	./getweb ppd

#
# Manpage generation.  No, I am not interested in "info" files or
# HTML documentation.
#
man: $(MANPAGES) man-icc2ps man-osx-hotplug

$(MANPAGES): macros.man includer

man-icc2ps:
	cd icc2ps; $(MAKE) man

man-osx-hotplug:
	cd osx-hotplug; $(MAKE) man

.1in.1: 
	-rm -f $*.1
	modtime() { $(MODTIME); }; \
	MODpage=`modtime $*.1in`; \
	MODver=$(VERSION); \
	./includer -t man -v DEF1=$(OLDGROFF) $*.1in | sed > $*.1 \
	    -e "s@\$${URLOAK}@$(URLOAK)@" \
	    -e "s@\$${URLZJS}@$(URLZJS)@" \
	    -e "s@\$${URLHP}@$(URLHP)@" \
	    -e "s@\$${URLXQX}@$(URLXQX)@" \
	    -e "s@\$${URLLAVA}@$(URLLAVA)@" \
	    -e "s@\$${URLQPDL}@$(URLQPDL)@" \
	    -e "s@\$${URLSLX}@$(URLSLX)@" \
	    -e "s@\$${URLHC}@$(URLHC)@" \
	    -e "s@\$${URLHBPL}@$(URLHBPL)@" \
	    -e "s@\$${URLDDST}@$(URLDDST)@" \
	    -e "s/\$${MODpage}/$$MODpage/" \
	    -e "s/\$${MODver}/$$MODver/"
	chmod a-w $*.1

install-man: man
	#
	# Install manual pages
	#
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -d -m 755 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2zjs.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2zjs-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 zjsdecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2oak.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2oak-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 oakdecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2hp.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2hp2600-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2xqx.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2xqx-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 xqxdecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2lava.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2lava-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 lavadecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 opldecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2qpdl.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2qpdl-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 qpdldecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2slx.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2slx-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 slxdecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2hiperc.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2hiperc-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 hipercdecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2hbpl2.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2hbpl2-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 hbpldecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2ddst.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2ddst-wrapper.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 ddstdecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 gipddecode.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2zjs-pstops.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 arm2hpdl.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 usb_printerid.1 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 printer-profile.1 $(MANDIR)/man1/
	cd icc2ps; $(MAKE) install-man
ifeq ($(UNAME),Darwin)
	cd osx-hotplug; $(MAKE) install-man
endif

doc: README INSTALL manual.pdf

install-doc: doc
	#
	# Install documentation
	#
	$(INSTALL) -d -m 755 $(DOCDIR)
	$(INSTALL) -c -m 644 manual.pdf $(DOCDIR)
	$(INSTALL) -c -m 644 COPYING $(DOCDIR)
	$(INSTALL) -c -m 644 INSTALL $(DOCDIR)
	$(INSTALL) -c -m 644 INSTALL.osx $(DOCDIR)
	$(INSTALL) -c -m 644 README $(DOCDIR)
	$(INSTALL) -c -m 644 ChangeLog $(DOCDIR)

GROFF=/usr/local/test/bin/groff
GROFF=groff
manual.pdf: $(MANPAGES) icc2ps/foo2zjs-icc2ps.1 osx-hotplug/osx-hplj-hotplug.1
	-$(GROFF) -t -man \
	    `ls $(MANPAGES) \
		icc2ps/foo2zjs-icc2ps.1 \
		osx-hotplug/osx-hplj-hotplug.1 \
		| sort` \
	    | ps2pdf - $@

README: README.in
	rm -f $@
	sed < $@.in > $@ \
	    -e "s@\$${URLOAK}@$(URLOAK)@" \
	    -e "s@\$${URLZJS}@$(URLZJS)@"
	chmod a-w $@

INSTALL: INSTALL.in Makefile
	rm -f $@
	echo "TOPICS" > $@.tmp
	echo "------" >> $@.tmp
	grep ^[A-Z][A-Z] $@.in | sed "s/^/    * /" >> $@.tmp
	echo >> $@.tmp
	cat $@.tmp $@.in | sed > $@ \
	    -e "s@\$${URLOAK}@$(URLOAK)@" \
	    -e "s@\$${URLZJS}@$(URLZJS)@"
	rm -f $@.tmp
	chmod a-w $@

#
#	Check db files against current foomatic to see if any changes
#	need to be made or reported.
#
MYFOODB=../foomatic/foomatic-db/db/source
checkdb:
	@for dir in driver printer opt; do \
	    for file in foomatic-db/$$dir/*.xml ; do \
		ofile=$(MYFOODB)/$$dir/`basename $$file`; \
		: echo diff -N -u $$ofile $$file; \
		if [ ! -f  $$ofile ]; then \
		    ofile=/dev/null; \
		fi; \
		diff -N -u $$ofile $$file; \
	    done \
	done

#
#	Mail my latest foomatic-db entries to Till.
#
WHO=rick.richardson@comcast.net
WHO=till.kamppeter@gmx.net
maildb:
	$(MAKE) -s checkdb > patch.db
	echo "Here is a patch for the foomatic-db foo2zjs/foo2oak entries." | \
	mutt -a patch.db \
	    -s "foo2zjs/foo2oak - patch for foomatic database" $(WHO)

#
# Create tarball
#
tar:	
	HERE=`basename $$PWD`; \
	/bin/ls $(FILES) | \
	sed -e "s?^?$$HERE/?" | \
	(cd ..; tar -c -z -f $$HERE/$$HERE.tar.gz -T-)

tarver:	
	HERENO=`basename $$PWD`; \
	HERE=`basename $$PWD-$(VERSION)`; \
	ln -sf $$HERENO ../$$HERE; \
	/bin/ls $(FILES) | \
	sed -e "s?^?$$HERE/?" | \
	(cd ..; tar -c -z -f $$HERE/$$HERE.tar.gz -T-); \
	rm -f ../$$HERE

#
# Populate the web site
#	make web
#	make webworld
#	make webextra
#
URLOAK=http://foo2oak.rkkda.com
URLZJS=http://foo2zjs.rkkda.com
URLHP=http://foo2hp.rkkda.com
URLXQX=http://foo2xqx.rkkda.com
URLLAVA=http://foo2lava.rkkda.com
URLQPDL=http://foo2qpdl.rkkda.com
URLSLX=http://foo2slx.rkkda.com
URLHC=http://foo2hiperc.rkkda.com
URLHBPL=http://foo2hbpl.rkkda.com
URLDDST=http://foo2ddst.rkkda.com
FTPSITE=~/.ncftp-website
FTPOPTS=
FTPOPTS=-S

foo2zjs.html foo2oak.html foo2hp.html \
    foo2xqx.html foo2lava.html foo2qpdl.html \
    foo2slx.html foo2hiperc.html foo2hbpl.html \
    foo2ddst.html: thermometer.gif FRC
	rm -f $@
	HERE=`basename $$PWD`; \
	TZ=`date | cut -c 21-24`; \
	modtime() { $(MODTIME); }; \
	MODindex=`modtime $@.in`; \
	MODtarball=`modtime $$HERE.tar.gz`; \
	MODsha=`sha1sum $$HERE.tar.gz | awk '{print $$1}'` ; \
	PRODUCT=`basename $@ .html`; \
	./includer -t html $@.in | sed > $@ \
	    -e "s@\$${URLOAK}@$(URLOAK)@g" \
	    -e "s@\$${URLZJS}@$(URLZJS)@g" \
	    -e "s@\$${URLHP}@$(URLHP)@g" \
	    -e "s@\$${URLXQX}@$(URLXQX)@g" \
	    -e "s@\$${URLLAVA}@$(URLLAVA)@g" \
	    -e "s@\$${URLQPDL}@$(URLQPDL)@g" \
	    -e "s@\$${URLSLX}@$(URLSLX)@g" \
	    -e "s@\$${URLHC}@$(URLHC)@g" \
	    -e "s@\$${URLHBPL}@$(URLHBPL)@g" \
	    -e "s@\$${URLDDST}@$(URLDDST)@g" \
	    -e "s@\$${PRODUCT}@$$PRODUCT@g" \
	    -e "s/\$${MODindex}/$$MODindex $$TZ/" \
	    -e "s/\$${MODtarball}/$$MODtarball $$TZ/" \
	    -e "s/\$${MODsha}/$$MODsha/"
	chmod a-w $@

myftpput: ../geo/myftpput
	rm -f myftpput
	cp -a ../geo/myftpput .
	chmod 555 myftpput

web: test tar manual.pdf webindex
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2zjs \
	    ChangeLog INSTALL manual.pdf foo2zjs.tar.gz;

webt: tar manual.pdf webindex
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2zjs \
	    ChangeLog INSTALL manual.pdf foo2zjs.tar.gz;

webworld: web webpics

webindex: INSTALL zjsindex oakindex hpindex xqxindex lavaindex \
	qpdlindex oakindex slxindex hcindex hbplindex ddstindex

webpics: redhat suse ubuntu mandriva fedora

webphotos:
	cd printer-photos; $(MAKE)

zjsindex: foo2zjs.html archzjs.gif thermometer.gif webphotos
	ln -sf foo2zjs.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2zjs \
	    index.html style.css archzjs.gif thermometer.gif \
	    images/flags.png INSTALL INSTALL.osx images/zjsfavicon.png \
	    Laserjet-1005-Series-MacOSX-10.pdf \
	    tablesort.js printer-photos/printers.jpg;

oakindex: foo2oak.html archoak.gif thermometer.gif webphotos
	ln -sf foo2oak.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2oak \
	    index.html style.css archoak.gif thermometer.gif \
	    images/flags.png INSTALL \
	    printer-photos/printers.jpg;

hpindex: foo2hp.html archhp.gif thermometer.gif webphotos
	ln -sf foo2hp.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hp \
	    index.html style.css archhp.gif thermometer.gif \
	    images/flags.png INSTALL images/hpfavicon.png \
	    printer-photos/printers.jpg;

xqxindex: foo2xqx.html archxqx.gif thermometer.gif webphotos
	ln -sf foo2xqx.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2xqx \
	    index.html style.css archxqx.gif thermometer.gif \
	    images/flags.png INSTALL images/xqxfavicon.png \
	    printer-photos/printers.jpg;

lavaindex: foo2lava.html archlava.gif thermometer.gif webphotos
	ln -sf foo2lava.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2lava \
	    index.html style.css archlava.gif thermometer.gif \
	    images/flags.png INSTALL images/lavafavicon.png \
	    printer-photos/printers.jpg;

qpdlindex: foo2qpdl.html archqpdl.gif thermometer.gif webphotos
	ln -sf foo2qpdl.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2qpdl \
	    index.html style.css archqpdl.gif thermometer.gif \
	    images/flags.png INSTALL images/qpdlfavicon.png \
	    printer-photos/printers.jpg;

slxindex: foo2slx.html archslx.gif thermometer.gif webphotos
	ln -sf foo2slx.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2slx \
	    index.html style.css archslx.gif thermometer.gif \
	    images/flags.png INSTALL images/slxfavicon.png \
	    printer-photos/printers.jpg;

hcindex: foo2hiperc.html archhiperc.gif thermometer.gif webphotos
	ln -sf foo2hiperc.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc \
	    index.html style.css archhiperc.gif thermometer.gif \
	    images/flags.png INSTALL images/hipercfavicon.png \
	    printer-photos/printers.jpg;

hbplindex: foo2hbpl.html archhbpl.gif thermometer.gif webphotos
	ln -sf foo2hbpl.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hbpl \
	    index.html style.css archhbpl.gif thermometer.gif \
	    images/flags.png INSTALL images/hbplfavicon.png \
	    printer-photos/printers.jpg;

ddstindex: foo2ddst.html archddst.gif thermometer.gif webphotos
	ln -sf foo2ddst.html index.html
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2ddst \
	    index.html style.css archddst.gif thermometer.gif \
	    images/flags.png INSTALL images/ddstfavicon.png \
	    printer-photos/printers.jpg;

foo2zjs.html: warning.html contribute.html resources.html unsupported.html
foo2hp.html: warning.html contribute.html resources.html unsupported.html
foo2xqx.html: warning.html contribute.html resources.html unsupported.html
foo2lava.html: warning.html contribute.html resources.html unsupported.html
foo2qpdl.html: warning.html contribute.html resources.html unsupported.html
foo2slx.html: warning.html contribute.html resources.html unsupported.html
foo2hiperc.html: warning.html contribute.html resources.html unsupported.html
foo2oak.html: warning.html contribute.html resources.html unsupported.html
foo2hbpl.html: warning.html contribute.html resources.html unsupported.html
foo2ddst.html: warning.html contribute.html resources.html unsupported.html

# RedHat
redhat: FRC
	cd redhat; $(MAKE) web FTPSITE=$(FTPSITE)

# Fedora Core 6+
fedora: FRC
	cd fedora; $(MAKE) web FTPSITE=$(FTPSITE)

suse: FRC
	cd suse; $(MAKE) web FTPSITE=$(FTPSITE)

ubuntu: FRC
	cd ubuntu; $(MAKE) web FTPSITE=$(FTPSITE)

mandriva: FRC
	cd mandriva; $(MAKE) web FTPSITE=$(FTPSITE)

#
#	Extra files from web
#
webextra: webicm webfw

webicm: \
	icm/dl2300.tar.gz \
	icm/km2430.tar.gz icm/hpclj2600n.tar.gz \
	icm/hp-cp1025.tar.gz \
	icm/hpclj2500.tar.gz \
	icm/hp1215.tar.gz icm/km2530.tar.gz \
	icm/km-1600.tar.gz \
	icm/samclp300.tar.gz icm/samclp315.tar.gz \
	icm/lexc500.tar.gz \
	icm/okic301.tar.gz \
	icm/okic310.tar.gz \
	icm/okic511.tar.gz \
	icm/okic3200.tar.gz \
	icm/okic3400.tar.gz icm/okic5600.tar.gz \
	icm/okic810.tar.gz
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2zjs/icm icm/dl2300.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2zjs/icm icm/km2430.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2zjs/icm icm/hp-cp1025.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hp/icm icm/hpclj2500.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hp/icm icm/hpclj2600n.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hp/icm icm/hp1215.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2lava/icm icm/km2530.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2lava/icm icm/km-1600.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2qpdl/icm icm/samclp300.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2qpdl/icm icm/samclp315.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2slx/icm icm/lexc500.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc/icm icm/okic301.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc/icm icm/okic310.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc/icm icm/okic511.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc/icm icm/okic3200.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc/icm icm/okic3400.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc/icm icm/okic5600.tar.gz;
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2hiperc/icm icm/okic810.tar.gz;

icm/dl2300.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ CP*.icm DL*.icm
icm/km2430.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ km2430*.icm
icm/hp-cp1025.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ hp-cp1025*.icm
icm/hpclj2500.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ hpclj2500*.icm
icm/hpclj2600n.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ hpclj2600*.icm
icm/hp1215.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ hp1215*.icm
icm/km2530.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ km2530*.icm
icm/km-1600.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ km-1600*.icm
icm/samclp300.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ samclp300*.icm
icm/samclp315.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ samclp315*.icm
icm/lexc500.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ lexR*.icm
icm/okic301.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ OKC301*.icm
icm/okic310.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ OKC310*.icm
icm/okic511.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ OKC511*.icm
icm/okic3200.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ OK32*.icm
icm/okic3400.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ C3400*.icm
icm/okic5600.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ OK56*.icm
icm/okic810.tar.gz: FRC
	cd icm; tar -c -z -f ../$@ OKC810*.icm

webfw:	firmware/sihp1000.tar.gz \
	firmware/sihp1005.tar.gz \
	firmware/sihp1018.tar.gz \
	firmware/sihp1020.tar.gz \
	firmware/sihpP1005.tar.gz \
	firmware/sihpP1006.tar.gz \
	firmware/sihpP1505.tar.gz \
	$(NULL)
	./myftpput $(FTPOPTS) -m -f $(FTPSITE) foo2zjs/firmware firmware/*.tar.gz;

firmware/sihp1000.tar.gz: FRC
	cd firmware; tar -c -z -f ../$@ sihp1000.img
firmware/sihp1005.tar.gz: FRC
	cd firmware; tar -c -z -f ../$@ sihp1005.img
firmware/sihp1018.tar.gz: FRC
	cd firmware; tar -c -z -f ../$@ sihp1018.img
firmware/sihp1020.tar.gz: FRC
	cd firmware; tar -c -z -f ../$@ sihp1020.img
firmware/sihpP1005.tar.gz: FRC
	cd firmware; tar -c -z -f ../$@ sihpP1005.img
firmware/sihpP1006.tar.gz: FRC
	cd firmware; tar -c -z -f ../$@ sihpP1006.img
firmware/sihpP1505.tar.gz: FRC
	cd firmware; tar -c -z -f ../$@ sihpP1505.img

FRC:

#
# Misc
#
misc: pksm2bitcmyk phorum-logo.gif

pksm2bitcmyk: pksm2bitcmyk.c
	$(CC) $(CFLAGS) pksm2bitcmyk.c -lnetpbm -o $@

phorum-logo.gif: archhp.fig
	fig2dev -L gif -m.25 archhp.fig | giftrans -t "#ffffff" -o $@

w:	all
	$(ROOT) $(MAKE) install install-hotplug cups
