UNAME := $(shell uname)

PREFIX=	/usr
PREFIX=	$(DESTDIR)/usr
BIN=	$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man

SRC=	icc2ps.c xgetopt.c
LIB=	cmscam97.c cmscnvrt.c cmserr.c cmsgamma.c cmsgmt.c cmsintrp.c cmsio1.c \
	cmslut.c cmsmatsh.c cmsmtrx.c cmsnamed.c cmspack.c cmspcs.c cmsps2.c \
	cmssamp.c cmswtpnt.c cmsxform.c cmsio0.c cmsvirt.c
CFLAGS=	-O3
INSTALL=install
ifeq ($(UNAME),SunOS)
    INSTALL=/usr/ucb/install
    CC=gcc
endif

.SUFFIXES: .1 .1in

all: foo2zjs-icc2ps man

foo2zjs-icc2ps: $(SRC) $(LIB)
	$(CC) $(CFLAGS) $(SRC) $(LIB) -lm -o $@

install: all
	$(INSTALL) -c foo2zjs-icc2ps $(BIN)

man:	foo2zjs-icc2ps.1

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

install-man: man
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -d -m 755 $(MANDIR)/man1/
	$(INSTALL) -c -m 644 foo2zjs-icc2ps.1 $(MANDIR)/man1/

clean:
	rm -f *.o foo2zjs-icc2ps *.1

foo2zjs-icc2ps: icc34.h lcms.h

regress: all
	@./foo2zjs-icc2ps a b 2>&1 | grep "little cms P"
	@for i in ../icm/*.icm; do \
	    b=`basename $$i`; \
	    ./foo2zjs-icc2ps -o $$i -t0 | grep -v Created >$$b.ps; \
	    md5sum $$b.ps; \
	done
	@rm -f *icm.ps
