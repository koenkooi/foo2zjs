/*
 * $Id: opldecode.c,v 1.11 2014/01/24 19:25:47 rick Exp $
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
"	opldecode [options] < zjs-file\n"
"\n"
"	Decode a Raster Object (opl) stream into human readable form.\n"
"\n"
"	A Raster Object stream is the printer language used by some Konica\n"
"	Minolta printers, such as the magicolor 2480 MF.\n"
"\n"
"\n"
"Options:\n"
"       -d basename Basename of .pbm file for saving decompressed planes\n"
"       -o          Print file offsets\n"
"       -h          Print hex file offsets\n"
"       -D lvl      Set Debug level [%d]\n"
    , Debug
    );

    exit(1);
}

void
print_bih(unsigned char bih[20])
{
    unsigned int xd, yd, l0;
    char hdr[] = "\n\t\t\t\t";

    if (!PrintOffset && !PrintHexOffset) hdr[strlen(hdr)-1] = 0;

    xd = (bih[4] << 24) | (bih[5] << 16) | (bih[6] << 8) | (bih[7] << 0);
    yd = (bih[8] << 24) | (bih[9] << 16) | (bih[10] << 8) | (bih[11] << 0);
    l0 = (bih[12] << 24) | (bih[13] << 16) | (bih[14] << 8) | (bih[15] << 0);

    printf("%sDL = %d, D = %d, P = %d, - = %d, XY = %d x %d",
	 hdr, bih[0], bih[1], bih[2], bih[3], xd, yd);

    printf("%sL0 = %d, MX = %d, MY = %d",
	 hdr, l0, bih[16], bih[17]);

    printf("%sOrder   = %d %s%s%s%s%s", hdr, bih[18],
	bih[18] & JBG_HITOLO ? " HITOLO" : "",
	bih[18] & JBG_SEQ ? " SEQ" : "",
	bih[18] & JBG_ILEAVE ? " ILEAVE" : "",
	bih[18] & JBG_SMID ? " SMID" : "",
	bih[18] & 0xf0 ? " other" : "");

    printf("%sOptions = %d %s%s%s%s%s%s%s%s", hdr, bih[19],
	bih[19] & JBG_LRLTWO ? " LRLTWO" : "",
	bih[19] & JBG_VLENGTH ? " VLENGTH" : "",
	bih[19] & JBG_TPDON ? " TPDON" : "",
	bih[19] & JBG_TPBON ? " TPBON" : "",
	bih[19] & JBG_DPON ? " DPON" : "",
	bih[19] & JBG_DPPRIV ? " DPPRIV" : "",
	bih[19] & JBG_DPLAST ? " DPLAST" : "",
	bih[19] & 0x80 ? " other" : "");
    printf("%s%u stripes, %d layers, %d planes",
	hdr,
	((yd >> bih[1]) +  ((((1UL << bih[1]) - 1) & xd) != 0) + l0 - 1) / l0,
	bih[1] - bih[0], bih[2]);
}

void
print_config(unsigned char *c)
{
    char hdr[] = "\n\t\t\t\t";

    if (!PrintOffset && !PrintHexOffset) hdr[strlen(hdr)-1] = 0;

    if (c[1] == 1)
    {
	printf("%sfmt=%d np=%d",
	    hdr, c[0], c[1]);
	printf("%sBLACK:	X=%d, Y=%d, unk=%d, #=%d(%d)",
	    hdr, (c[2]<<8) + c[3], (c[4]<<8) + c[5], c[6], 1 << c[7], c[7]);
    }
    else if (c[1] == 4)
    {
	printf("%sfmt=%d np=%d",
	    hdr, c[0], c[1]);
	printf("%sYEL:	X=%d, Y=%d, unk=%d, #=%d(%d)",
	    hdr, (c[2]<<8) + c[3], (c[4]<<8) + c[5], c[6], 1 << c[7], c[7]);
	printf("%sMAG:	X=%d, Y=%d, unk=%d, #=%d(%d)",
	    hdr, (c[8]<<8) + c[9], (c[10]<<8) + c[11], c[12], 1 << c[13], c[13]);
	printf("%sCYA:	X=%d, Y=%d, unk=%d, #=%d(%d)",
	    hdr, (c[14]<<8) + c[15], (c[16]<<8) + c[17], c[18], 1 << c[19], c[19]);
	printf("%sBLK:	X=%d, Y=%d, unk=%d, #=%d(%d)",
	    hdr, (c[20]<<8) + c[21], (c[22]<<8) + c[23], c[24], 1 << c[25], c[25]);
    }
    else
	error(1, "config image data is not 8 or 26 bytes!\n");
}

void
proff(int curOff)
{
    if (PrintOffset)
	printf("%d:	", curOff);
    else if (PrintHexOffset)
	printf("%6x:	", curOff);
}

char *
fgetcomma(char *s, int size, int *datalen, FILE *stream)
{
    int	c;
    char *os = s;

    *datalen = 0;
    while ((c = fgetc(stream)) != EOF)
    {
	*s++ = c;
	if (c == ';')
	    break;
	if (c == '#')
	{
	    while ((c = fgetc(stream)) != EOF)
	    {
		*s++ = c;
		if (c == '=')
		    break;
		else
		{
		    *datalen *= 10;
		    *datalen += c - '0';
		}
	    }
	    break;
	}
    }
    if (c == EOF)
	return (NULL);
    *s++ = 0;
    return (os);
}

int
jbig_decode1(unsigned char ch, int pn, int page, struct jbg_dec_state *pstate,
    FILE *dfp)
{
    size_t	cnt;
    int		rc;

    rc = jbg_dec_in(pstate, &ch, 1, &cnt);
    if (rc == JBG_EOK)
    {
	int     h, w, len;
	unsigned char *image;

	//debug(0, "JBG_EOK: %d\n", pn);
	h = jbg_dec_getheight(pstate);
	w = jbg_dec_getwidth(pstate);
	image = jbg_dec_getimage(pstate, 0);
	len = jbg_dec_getsize(pstate);
	if (image)
	{
	    char        buf[512];
	    sprintf(buf, "%s-%02d-%d.pbm",
		    DecFile, page, pn);
	    dfp = fopen(buf, "w");
	    if (dfp)
	    {
		    fprintf(dfp, "P4\n%8d %8d\n", w, h);
		rc = fwrite(image, 1, len, dfp);
		fclose(dfp);
	    }
	}
	else
	    debug(0, "Missing image %dx%d!\n", h, w);
	jbg_dec_free(pstate);
    }
    return (rc);
}

void
decode(FILE *fp)
{
    int		c;
    int		rc;
    FILE	*dfp = NULL;
    int		pageNum = 1;
    int		curOff = 0;
    struct jbg_dec_state	s[5];
    unsigned char	bih[20];
    int			bihlen = 0;
    int         	pn = 0;
    int			totSize = 0;
    char		buf[100*1024];
    int			datalen;
    int			nbh = 0;
    int			firstbh = 1;

    while (fgetcomma(buf, sizeof(buf), &datalen, fp))
    {
	proff(curOff); curOff += strlen(buf);
	if (strlen(buf) >= 65)
	{
	    printf("%65.65s ...\n", buf);
	    printf("\t... %64.64s\n", buf + strlen(buf) - 64);
	}
	else
	    printf("%s\n", buf);
	if (0) {
	}
	else if (strncmp(buf, "LockPrinterWait?Event=StartOfJob", 32) == 0) {
	}
	else if (strncmp(buf, "Event=StartOfJob", 16) == 0) {
	}
	else if (strncmp(buf, "Event=StartOfPage", 17) == 0) {
	    firstbh = 1;
	}
	else if (strncmp(buf, "Event=EndOfBand", 15) == 0) {
	}
	else if (strncmp(buf, "Event=EndOfPage", 15) == 0) {
	    pn = 0;
	    nbh = 0;
	    ++pageNum;
	}
	else if (strncmp(buf, "Event=EndOfJob", 14) == 0) {
	}
	else if (strncmp(buf, "RasterObject.BitsPerPixel", 26) == 0) {
	}
	else if (strncmp(buf, "RasterObject.Planes", 19) == 0) {
	    int pl;
	    sscanf(buf+20, "%x", &pl);
	    debug(1, "planes=%x\n", pl);
	}
	else if (strncmp(buf, "RasterObject.Width", 18) == 0) {
	    int w;
	    sscanf(buf+19, "%d", &w);
	    debug(1, "width=%d\n", w);
	}
	else if (strncmp(buf, "RasterObject.Height", 19) == 0) {
	    int h;
	    sscanf(buf+20, "%d", &h);
	    debug(1, "height=%d\n", h);
	}
	else if (strncmp(buf, "RasterObject.BandHeight", 23) == 0) {
	    int bh;
	    sscanf(buf+24, "%d", &bh);
	    nbh += bh;
	    debug(1, "bandheight=%d, nbh=%d\n", bh, nbh);
	}
	else if (strncmp(buf, "RasterObject.Data", 17) == 0) {
	    if (firstbh && nbh != 0)
	    {
		firstbh = 0;
		debug(1, "firstbh\n");
		rc = fread(bih, bihlen = sizeof(bih), 1, fp);
		print_bih(bih);
		printf("\n");
		datalen -= sizeof(bih);
		if (DecFile)
		{
		    size_t      cnt;

		    jbg_dec_init(&s[pn]);
		    rc = jbg_dec_in(&s[pn], bih, bihlen, &cnt);
		    if (rc == JBG_EIMPL)
			error(1, "JBIG uses unimpl feature\n");
		}
	    }
	    curOff += datalen + 1;
	    totSize += datalen;
	    if (datalen == 20) {
		++pn;
		rc = fread(bih, bihlen = sizeof(bih), 1, fp);
		print_bih(bih);
		printf("\n");
		getc(fp);
		if (DecFile)
		{
		    size_t      cnt;

		    jbg_dec_init(&s[pn]);
		    rc = jbg_dec_in(&s[pn], bih, bihlen, &cnt);
		    if (rc == JBG_EIMPL)
			error(1, "JBIG uses unimpl feature\n");
		}
	    }
	    else {
		if (datalen)
		{
		    unsigned char ch;

		    while (datalen--)
		    {
			c = getc(fp);
			ch = c;
			if (DecFile)
			    jbig_decode1(ch, pn, pageNum, &s[pn], dfp);
		    }
		    getc(fp);
		}
	    }
	}
    }
}

int
main(int argc, char *argv[])
{
	extern int	optind;
	extern char	*optarg;
	int		c;

	while ( (c = getopt(argc, argv, "d:hoD:?h")) != EOF)
		switch (c)
		{
		case 'd': DecFile = optarg; break;
		case 'o': PrintOffset = 1; break;
		case 'h': PrintHexOffset = 1; break;
		case 'D': Debug = atoi(optarg); break;
		default: usage(); exit(1);
		}

	argc -= optind;
	argv += optind;

        if (argc > 0)
        {
            FILE        *fp;

            fp = fopen(argv[0], "r");
            if (!fp)
                error(1, "file '%s' doesn't exist\n", argv[0]);
            for (;;)
            {
                decode(fp);
                c = getc(fp); ungetc(c, fp);
                if (feof(fp))
                    break;
            }
            fclose(fp);
        }
        else
        {
	    for(;;)
	    {
		decode(stdin);
		c = getc(stdin); ungetc(c, stdin);
		if (feof(stdin))
		    break;
	    }
	}
	printf("\n");

	exit(0);
}
