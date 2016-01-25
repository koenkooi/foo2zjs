/*
 * $Id: qpdldecode.c,v 1.41 2014/01/24 19:25:47 rick Exp $
 */

/*b
 * Copyright (C) 2003-2006  Rick Richardson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: Rick Richardson <rick.richardson@comcast.net>
b*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "jbig.h"

/*
 * Global option flags
 */
int	Debug = 0;
char	*RawFile;
char	*DecFile;
int	PrintOffset = 0;
int	PrintHexOffset = 0;

void
debug(int level, char *fmt, ...)
{
    va_list ap;

    if (Debug < level)
	return;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

int
error(int fatal, char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, fatal ? "Error: " : "Warning: ");
	if (errno)
	    fprintf(stderr, "%s: ", strerror(errno));
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (fatal > 0)
	    exit(fatal);
	else
	{
	    errno = 0;
	    return (fatal);
	}
}

void
usage(void)
{
    fprintf(stderr,
"Usage:\n"
"	qpdldecode [options] < qpdl-file.prn\n"
"\n"
"	Decode a QPDL stream into human readable form.\n"
"\n"
"	A Quick Page Description Langauge (QPDL) is the printer language"
"	used by some Samsung printers, such as the CLP-600n.\n"
"\n"
"	Also known as SPLC."
"\n"
"Options:\n"
"       -d basename Basename of .pbm file for saving decompressed planes\n"
//"       -r basename Basename of .jbg file for saving raw planes\n"
"       -o          Print file offsets\n"
"       -h          Print hex file offsets\n"
"       -D lvl      Set Debug level [%d]\n"
    , Debug
    );

    exit(1);
}

static int
getBEdword(char buf[4])
{
    unsigned char *b = (unsigned char *) buf;
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | (b[3] << 0);
}

static int
getLEdword(char buf[4])
{
    unsigned char *b = (unsigned char *) buf;
    return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | (b[0] << 0);
}

static int
getBEword(char buf[2])
{
    unsigned char *b = (unsigned char *) buf;
    return (b[0] << 8) | (b[1] << 0);
}

void
print_bih(unsigned char bih[20])
{
    unsigned int xd, yd, l0;

    xd = (bih[4] << 24) | (bih[5] << 16) | (bih[6] << 8) | (bih[7] << 0);
    yd = (bih[8] << 24) | (bih[9] << 16) | (bih[10] << 8) | (bih[11] << 0);
    l0 = (bih[12] << 24) | (bih[13] << 16) | (bih[14] << 8) | (bih[15] << 0);

    printf("		DL = %d, D = %d, P = %d, - = %d, XY = %d x %d, "
	    "%s\n",
	    bih[0], bih[1], bih[2], bih[3], xd, yd,
	    (xd % 256) ? "*** xd%256 != 0!" : "");

    printf("		L0 = %d, MX = %d, MY = %d\n",
	l0, bih[16], bih[17]);

    printf("		Order   = %d %s%s%s%s%s\n", bih[18],
	bih[18] & JBG_HITOLO ? " HITOLO" : "",
	bih[18] & JBG_SEQ ? " SEQ" : "",
	bih[18] & JBG_ILEAVE ? " ILEAVE" : "",
	bih[18] & JBG_SMID ? " SMID" : "",
	bih[18] & 0xf0 ? " other" : "");

    printf("		Options = %d %s%s%s%s%s%s%s%s\n", bih[19],
	bih[19] & JBG_LRLTWO ? " LRLTWO" : "",
	bih[19] & JBG_VLENGTH ? " VLENGTH" : "",
	bih[19] & JBG_TPDON ? " TPDON" : "",
	bih[19] & JBG_TPBON ? " TPBON" : "",
	bih[19] & JBG_DPON ? " DPON" : "",
	bih[19] & JBG_DPPRIV ? " DPPRIV" : "",
	bih[19] & JBG_DPLAST ? " DPLAST" : "",
	bih[19] & 0x80 ? " other" : "");
    printf("		%u stripes, %d layers, %d planes\n",
	((yd >> bih[1]) +  ((((1UL << bih[1]) - 1) & xd) != 0) + l0 - 1) / l0,
	bih[1] - bih[0], bih[2]);
}

void
proff(int curOff)
{
    if (PrintOffset)
        printf("%d:     ", curOff);
    else if (PrintHexOffset)
        printf("%6x:    ", curOff);
}

void
decode(FILE *fp)
{
    int		c;
    int		rc;
    FILE	*dfp = NULL;
    int		pageNum = 0;
    int		i;
    int		curOff = 0;
    struct jbg_dec_state	s[5];
    unsigned char	bih[5][20];
    int			imageCnt[5] = {0,0,0,0};
    int         	pn = 0;
    int			ver, end;
    char		buf[5120*1024];

    c = getc(fp);
    if (c == EOF)
    {
	printf("EOF on file\n");
	return;
    }
    ungetc(c, fp);
    if (c == '\033')
    {
	while (fgets(buf, sizeof(buf), fp))
	{
	    proff(curOff);
	    if (buf[0] == '\033')
	    {
		printf("\\033");
		fputs(buf+1, stdout);
	    }
	    else
		fputs(buf, stdout);
	    curOff += strlen(buf);
	    if ((strcmp(buf, "@PJL ENTER LANGUAGE = QPDL\r\n") == 0)
		|| (strcmp(buf, "@PJL ENTER LANGUAGE = QPDL\n") == 0))
		break;
	}
    }

    for (;;)
    {
	int	reclen;
	int	rectype, subtype;
	int	wb, h, comp, stripe;
	int	cksum;

	#define STRARY(X, A) \
	    ((X) >= 0 && (X) < sizeof(A)/sizeof(A[0])) \
	    ? A[X] : "UNK"
	char *strsize[] = {
	    /*00*/ "letter", "legal", "a4", "exec", "ledger",
	    /*05*/ "a3", "env#10", "envMonarch", "envC5", "envDL",
	    /*10*/ "b4", "b5jis", "b5iso", "UNK", "UNK",
	    /*15*/ "UNK", "a5", "a6", "UNK", "UNK",
	    /*20*/ "UNK", "custom",  "UNK", "envC6", "folio",
	    /*25*/ "env6.75", "env#9",  "UNK", "oficio", "UNK",
	    /*30*/ "statement", "UNK",  "UNK", "UNK", "UNK",
	    };
	char *strsource[] = {
	    /*00*/ "unk", "auto", "manual", "multi", "tray1",
	    };

	rectype = fgetc(fp);
	if (rectype == EOF)
	    break;

	proff(curOff);
	curOff++;
	printf("RECTYPE 0x%x", rectype);

	switch (rectype)
	{
	case 0x01:
	    printf("	len=3\n");
	    if (fread(buf+1, 2, 1, fp) != 1)
		error(1, "Couldn't get 2 bytes\n");
	    curOff += 2;
	    printf("\t\tcopies=%d\n", getBEword(buf+1));
	    break;
	case 0x11:	// NOT JBIG!!
	    if (fread(buf+1, 4, 1, fp) != 1)
	    {
		printf("\n");
                error(1, "Couldn't get 4 bytes\n");
	    }
	    curOff += 4;
	    reclen = getBEdword(buf+1);
	    ++reclen;
	    printf("	len=%d(0x%x)\n", 5+reclen, 5+reclen);
	    if (fread(buf, reclen, 1, fp) != 1)
		error(1, "Couldn't get 0x%x(%d) bytes\n", reclen, reclen);
	    curOff += reclen;

	    break;
	case 0x09:
	    printf("	len=0\n");
	    goto done;
	    break;
	case 0x00:
	    printf("	len=17	pageNum=%d\n", ++pageNum);
	    if (fread(buf+1, 16, 1, fp) != 1)
		error(1, "Couldn't get 16 bytes\n");
	    curOff += 16;
	    printf("\t\tyres=%d, copies=%d, papersize=%s(%d), w=%d, h=%d\n",
		buf[1]*100,
		getBEword(buf+2),
		STRARY((int)buf[4], strsize), buf[4],
		getBEword(buf+5),
		getBEword(buf+7)
		);
	    printf("\t\tpapersource=%s, unk=%d, duplex=%d:%d, unk=%d,%d, "
		" unk=%d(0x%x)\n"
		, STRARY((int)buf[9], strsource)
		, buf[0xA]
		, buf[0xB]
		, buf[0xC]
		, buf[0xD]
		, buf[0xE]
		, getBEword(buf+15)
		, getBEword(buf+15)
		);
	    printf("\t\txres=%d\n"
		, (getBEword(buf+15) & 0xff) * 100
		);

	    pn = 0;
	    memset(imageCnt, 0, sizeof(imageCnt));
	    break;
	case 0x13:
	    printf("    len=15\n");
            if (fread(buf+1, 14, 1, fp) != 1)
                error(1, "Couldn't get 14 bytes\n");
            curOff += 14;
	    printf("\t\t");
	    for (i = 1; i <= 14; ++i)
		printf("%02x, ", buf[i]);
	    printf("\n");
	    break;
	case 0x14:
            if (fread(buf+1, 7, 1, fp) != 1)
                error(1, "Couldn't get 7 bytes\n");
	    curOff += 7;
	    subtype = buf[1];
	    if (subtype == 0x10)
	    {
		printf("    len=8\n");
		printf("\t\tunknown: ");
		for (i = 1; i <= 7; ++i)
		    printf("%02x, ", (unsigned char) buf[i]);
		printf("\n");
	    }
	    else
	    {
		/* BIH */
		printf("    len=25\n");
		if (fread(buf+7+1, 24-7, 1, fp) != 1)
		    error(1, "Couldn't get 24 bytes\n");
		curOff += 24-7;
		if (0)
		{
		    printf("\t\t");
		    for (i = 1; i <= 16; ++i)
			printf("%02x, ", (unsigned char) buf[i]);
		    printf("\n\t\t");
		    for (i = 17; i <= 24; ++i)
			printf("%02x, ", (unsigned char) buf[i]);
		}
		else
		{
		    printf("\t\t");
		    for (i = 21; i <= 24; ++i)
			printf("%02x, ", (unsigned char) buf[i]);
		    printf("(Margin=%d)", (unsigned char) buf[24]);
		}
		printf("\n");
		print_bih( (unsigned char *) buf+1);
		for (i = 0; i <=4; ++i)
		    memcpy(bih[i], buf+1, 20);
	    }
	    break;
	case 0x0c:
	    if (fread(buf+1, 11, 1, fp) != 1)
	    {
		printf("\n");
		error(1, "Couldn't get 11 bytes\n");
	    }
	    curOff += 11;
	    stripe = buf[1];
	    wb = getBEword(buf+2);
	    h = getBEword(buf+4);
	    pn = buf[6];
	    comp = buf[7];
	    reclen = getBEdword(buf+8);
	    if (comp == 0x12)
		reclen++;
	    if (comp == 0x11)
		reclen++;
	    printf("\tlen=%d(0x%x)\n", 12+reclen, 12+reclen);
	    printf("\t\tstripe=%d, WB=%d(0x%x), H=%d(0x%x), plane=%d, "
			"comp=0x%x,\n\t\tlen=%d(0x%x)\n",
			stripe, wb, wb, h, h, pn, comp, reclen, reclen);

	    if (fread(buf, reclen, 1, fp) != 1)
		error(1, "Couldn't get 0x%x(%d) bytes\n", reclen, reclen);
	    curOff += reclen;
	
	    cksum = 0;
	    for (i = 0; i < reclen-4; ++i)
		cksum += (unsigned char) buf[i];

	    if (comp == 0x15)
	    {
		if (0) printf("pn=%d\n", pn);
	    }
	    else if ( ((unsigned char) buf[0]) == 0xef)
	    {
		ver = getLEdword(buf+0) >> 28;
		end = (getBEdword(buf+8) >> 24) - 1;
		printf("\t\tmagic=0x%x, len=%d(0x%x), unk=%x,%x,%x,%x,%x,%x,"
		    "\n\t\tend=%d, ver=%d, checksum WANT=0x%x GOT=0x%x\n"
		    , getLEdword(buf+0)
		    , getLEdword(buf+4)
		    , getLEdword(buf+4)
		    , getLEdword(buf+8)
		    , getLEdword(buf+12)
		    , getLEdword(buf+16)
		    , getLEdword(buf+20)
		    , getLEdword(buf+24)
		    , getLEdword(buf+28)
		    , end
		    , ver
		    , cksum
		    , getLEdword(buf+reclen-4)
		    );
	    }
	    else
	    {
		ver = getBEdword(buf+0) >> 28;
		end = (getBEdword(buf+8) >> 24) - 1;
		printf("\t\tmagic=0x%x, len=%d(0x%x), unk=%x,%x,%x,%x,%x,%x,"
		    "\n\t\tend=%d, ver=%d, checksum WANT=0x%x GOT=0x%x\n"
		    , getBEdword(buf+0)
		    , getBEdword(buf+4)
		    , getBEdword(buf+4)
		    , getBEdword(buf+8)
		    , getBEdword(buf+12)
		    , getBEdword(buf+16)
		    , getBEdword(buf+20)
		    , getBEdword(buf+24)
		    , getBEdword(buf+28)
		    , end
		    , ver
		    , cksum
		    , getBEdword(buf+reclen-4)
		    );
	    }

	    if (comp == 0x13 || comp == 0x15)
	    {
if (0) printf("stripe=%d\n", stripe);
		// if ( (comp == 0x13 && stripe == 0) || comp == 0x15)
		if (comp == 0x13 && stripe == 0)
		{
		    size_t      cnt;

		    memcpy(bih[pn], buf+32, 20);
		    print_bih(bih[pn]);

		    jbg_dec_init(&s[pn]);
		    rc = jbg_dec_in(&s[pn], bih[pn], 20, &cnt);
		    if (rc == JBG_EIMPL)
			error(1, "JBIG uses unimpl feature\n");
		    break;
		}
		else if (comp == 0x15)
		{
		    size_t      cnt;

		    jbg_dec_init(&s[pn]);
		    rc = jbg_dec_in(&s[pn], bih[pn], 20, &cnt);
		    if (rc == JBG_EIMPL)
			error(1, "JBIG uses unimpl feature\n");
		}
		else if (comp == 0x13 && stripe >= 1)
		{
		    printf("\t\tData: ");
		    for (i = 0; i < 16; ++i)
			printf("%02x ", (unsigned char) buf[32+i]);
		    printf("...\n");
		}

		if (DecFile)
		{
		    if (comp == 0x15)
		    {
			reclen -= 4;
			i = 0;
		    }
		    else
			i = 32;
if (0 && comp == 0x15) printf("c=%02x ", (unsigned char) buf[i]);
		    for (; i < reclen; ++i)
		    {
			size_t              cnt;
			unsigned char       ch = c;

			ch = c = buf[i];

			rc = JBG_EAGAIN;
if (0 && comp == 0x15) printf("i=%d ", i);
			rc = jbg_dec_in(&s[pn], &ch, 1, &cnt);
if (0 && comp == 0x15) printf("rc=%d ", rc);
if (0 && comp == 0x15 && i == (reclen-1))  printf("c=%02x ", ch);
if (0 && comp == 0x15 && i == (reclen-2))  printf("c=%02x ", ch);
			if (rc == JBG_EOK)
			{
			    int     h, w, len;
			    unsigned char *image;

if (0 && comp == 0x15) printf("OK\n");
if (0) printf("OK\n");
			    // debug(0, "JBG_EOK: %d\n", pn);
			    h = jbg_dec_getheight(&s[pn]);
			    w = jbg_dec_getwidth(&s[pn]);
			    image = jbg_dec_getimage(&s[pn], 0);
			    len = jbg_dec_getsize(&s[pn]);
			    if (comp == 0x13 && image)
			    {
				char        buf[512];
				sprintf(buf, "%s-%02d-%d.pbm",
					DecFile, pageNum, pn);
				dfp = fopen(buf,
					    imageCnt[pn] ? "a" : "w");
				if (dfp)
				{
				    if (imageCnt[pn] == 0)
					fprintf(dfp, "P4\n%8d %8d\n", w, h);
				    //imageCnt[pn] += incrY;
				    rc = fwrite(image, 1, len, dfp);
				    fclose(dfp);
				    dfp = NULL;
				}
			    }
			    else if (comp == 0x15 && image)
			    {
				char        buf[512];
				if (dfp == 0)
				{
				    sprintf(buf, "%s-%02d-%d.pbm",
					DecFile, pageNum, pn);
				    dfp = fopen(buf,
					    imageCnt[pn] ? "r+" : "w");
				}
				if (dfp)
				{
				    fseek(dfp, 0, SEEK_SET);
				    // if (imageCnt[pn] == 0)
				    fprintf(dfp, "P4\n%8d %8d\n", w, h*stripe);
				    imageCnt[pn] += 1;
				    fseek(dfp, stripe * h * wb, SEEK_CUR);
				    rc = fwrite(image, 1, len, dfp);
				    fclose(dfp);
				    dfp = NULL;
				}
			    }
			    else
				debug(0, "Missing image %dx%d!\n", h, w);
			    jbg_dec_free(&s[pn]);
			    break;
			}
		    }
		}
	    }
	    break;
	default:
	    printf("\n");
	    error(1, "Unknown rectype 0x%x at 0x%x(%d)\n",
			rectype, curOff, curOff);
	    break;
	}
    }

done:
    c = fgetc(fp);
    if (c != 033)
	return;
    ungetc(c, fp);

    while (fgets(buf, sizeof(buf), fp))
    {
	proff(curOff);
	if (buf[0] == '\033')
        {
            printf("\\033");
            fputs(buf+1, stdout);
        }
        else
            fputs(buf, stdout);
	curOff += strlen(buf);
	if (strcmp(buf, "@PJL ENTER LANGUAGE=HIPERC\n") == 0)
	    break;
    }
    printf("\n");
}

int
main(int argc, char *argv[])
{
	extern int	optind;
	extern char	*optarg;
	int		c;

	while ( (c = getopt(argc, argv, "d:hor:D:?h")) != EOF)
		switch (c)
		{
		case 'd': DecFile = optarg; break;
		case 'r': RawFile = optarg; break;
		case 'o': PrintOffset = 1; break;
		case 'h': PrintHexOffset = 1; break;
		case 'D': Debug = atoi(optarg); break;
		default: usage(); exit(1);
		}

	argc -= optind;
	argv += optind;

	if (argc > 0)
	{
	    FILE	*fp;

	    fp = fopen(argv[0], "r");
	    if (!fp)
		error(1, "file '%s' doesn't exist\n", argv[0]);
	    decode(fp);
	    fclose(fp);
	}
	else
	    decode(stdin);

	exit(0);
}
