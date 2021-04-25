/*
 * $Id: oakdecode.c,v 1.43 2020/02/08 13:36:18 rick Exp $
 *
 * Work in progress decoder for Oak Tech. JBIG streams (HP1500)
 *
 * The image data appears to be a bastard little-endian version of JBIG,
 * split into bands of 256 pixels (except for the last).  The data needs
 * to be directly driven into a hacked version of jbig.c.  The -r and -d
 * options are non-functional.
 *
 * In addition, the image data is left/right mirrored.
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
#include "oak.h"

/*
 * Global option flags
 */
int	Debug = 0;
char	*RawFile;
char	*DecFile;
int	PrintOffset = 0;
int	SupressImage = 0;

int	ImageRec[4];
FILE	*FpDec[4][2];
FILE	*FpRaw[4][2];

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
"	oakdecode [options] < OAKT-file\n"
"\n"
"	Decode an Oak Tech. OAKT printer stream into human readable form.\n"
"\n"
"	OAKT is the printer language used by the HP 1500 printers.\n"
"\n"
"Options:\n"
"       -d basename Basename of .pbm file for saving decompressed planes\n"
"       -r basename Basename of .jbg file for saving raw planes\n"
"       -i          Supress display of image records\n"
"       -o          Print file offsets\n"
"       -D lvl      Set Debug level [%d]\n"
    , Debug
    );

    exit(1);
}

#if 0
BIH-style  from foo2zjs/pbmtojbg...

00000000: 00 00 01 00   00 00 26 40   00 00 18 f8   00 00 00 80
00000010: 10 00 03 5c

#include <inttypes.h>
typedef uint32_t	DWORD;
typedef uint16_t	WORD;
typedef uint8_t		BYTE;

typedef struct
{
    DWORD	opt1;
    DWORD	xd;	// Oak has this little endian
    DWORD	yd;	// Oak has this little endian
    DWORD	l0;	// Oak has this little endian
    DWORD	opt2;
} OAKBIH;
#endif

void
iswap32(void *p)
{
    char *cp = (char *) p;
    char tmp;
    tmp = cp[0];
    cp[0] = cp[3];
    cp[3] = tmp;
    tmp = cp[1];
    cp[1] = cp[2];
    cp[2] = tmp;
}

/*
 * This is the standard JBIG-KIT big-endian BIH prettyprinter.
 */
void
print_bih(unsigned char bih[20])
{
    unsigned int xd, yd, l0;

    xd = (bih[4] << 24) | (bih[5] << 16) | (bih[6] << 8) | (bih[7] << 0);
    yd = (bih[8] << 24) | (bih[9] << 16) | (bih[10] << 8) | (bih[11] << 0);
    l0 = (bih[12] << 24) | (bih[13] << 16) | (bih[14] << 8) | (bih[15] << 0);

    printf("		DL = %d, D = %d, P = %d, - = %d, XY = %d x %d\n",
	 bih[0], bih[1], bih[2], bih[3], xd, yd);

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

#if 0
00000260: 4f 41 4b 54 40 00 00 00 3c 00 00 00 00 00 01 00 | OAKT@...<....... |
00000270: 60 13 00 00 80 00 00 00 80 00 00 00 20 00 03 58 | `........... ..X |
00000280: 02 00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 | ................ |
00000290: 00 00 00 00 00 00 00 00 50 41 44 5f 50 41 44 5f | ........PAD_PAD_ |
000002a0: ff 02 50 41 44 5f 50 41 44 5f 50 41 44 5f 50 41 | ..PAD_PAD_PAD_PA |

000002b0: 4f 41 4b 54 40 00 00 00 3c 00 00 00 00 00 01 00 | OAKT@...<....... |
000002c0: 60 13 00 00 80 00 00 00 80 00 00 00 20 00 03 58 | `........... ..X |
000002d0: 02 00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 | ................ |
000002e0: 00 00 00 00 01 00 00 00 50 41 44 5f 50 41 44 5f | ........PAD_PAD_ |
000002f0: ff 02 50 41 44 5f 50 41 44 5f 50 41 44 5f 50 41 | ..PAD_PAD_PAD_PA |

0000ba20: 4f 41 4b 54 40 00 00 00 3c 00 00 00 00 00 01 00 | OAKT@...<....... |
0000ba30: 60 13 00 00 00 01 00 00 00 01 00 00 20 00 03 58 | `........... ..X |
0000ba40: 90 0d 00 00 a0 0d 00 00 00 00 00 00 40 02 00 00 | ............@... |
0000ba50: 00 00 00 00 00 00 00 00 50 41 44 5f 50 41 44 5f | ........PAD_PAD_ |
0000ba60: a6 ab 23 26 a0 78 7e 82 25 30 39 8b 95 16 32 4e | ..#&.x~.%09...2N |
...
0000c7f0: 50 41 44 5f 50 41 44 5f 50 41 44 5f 50 41 44 5f | PAD_PAD_PAD_PAD_ |

0000c800: 4f 41 4b 54 40 00 00 00 3c 00 00 00 00 00 01 00 | OAKT@...<....... |
0000c810: 60 13 00 00 00 01 00 00 00 01 00 00 20 00 03 58 | `........... ..X |
0000c820: 02 00 00 00 10 00 00 00 00 00 00 00 40 02 00 00 | ............@... |
0000c830: 00 00 00 00 01 00 00 00 50 41 44 5f 50 41 44 5f | ........PAD_PAD_ |
0000c840: ff 02 50 41 44 5f 50 41 44 5f 50 41 44 5f 50 41 | ..PAD_PAD_PAD_PA |

000124d0: 4f 41 4b 54 40 00 00 00 3c 00 00 00 00 00 01 00 | OAKT@...<....... |
000124e0: 60 13 00 00 00 01 00 00 00 01 00 00 20 00 03 58 | `........... ..X |
000124f0: fe 16 00 00 10 17 00 00 00 00 00 00 40 02 00 00 | ............@... |
00012500: 03 00 00 00 00 00 00 00 50 41 44 5f 50 41 44 5f | ........PAD_PAD_ |
00012510: 4c 8b 1a 0b 4d 74 19 a8 64 fa 91 c0 02 10 36 8c | L...Mt..d.....6. |
...
00013bf0: 11 f6 d3 e1 ca 98 ed b3 1a c3 2d a1 db 34 a9 db | ..........-..4.. |
00013c00: 04 f4 b8 2e 53 cb d3 be b3 e4 8a 3c ff 02 50 41 | ....S......<..PA |
00013c10: 44 5f 50 41 44 5f 50 41 44 5f 50 41 44 5f 50 41 | D_PAD_PAD_PAD_PA |

00013c20: 4f 41 4b 54 40 00 00 00 3c 00 00 00 00 00 01 00 | OAKT@...<....... |
00013c30: 60 13 00 00 00 01 00 00 00 01 00 00 20 00 03 58 | `........... ..X |
00013c40: 7e 02 00 00 90 02 00 00 00 00 00 00 40 02 00 00 | ~...........@... |
00013c50: 03 00 00 00 01 00 00 00 50 41 44 5f 50 41 44 5f | ........PAD_PAD_ |


typedef struct
{
    char	magic[4];	
    DWORD	len;
    DWORD	type;
} OAK_HDR;
#endif

typedef struct
{
    WORD	unk;
    char	string[64];
    WORD	pad;
} HDR_0D;

typedef struct
{
    char	datetime[32];	// Date/time in string format (with NL)
    DWORD	time_t;		// Time in seconds since the Unix epoch
    WORD	year;		// e.g. 2003
    WORD	tm_mon;		// Month-1
    WORD	tm_mday;	// Day of month (1-31)
    WORD	tm_hour;	// Hour (0-23)
    WORD	tm_min;		// Minute (0-59)
    WORD	tm_sec;		// Second (0-59)
    DWORD	pad;
} HDR_0C;

typedef struct
{
    OAKBIH	bih;
    DWORD	datalen;
    DWORD	padlen;
    DWORD	unk1C;
    DWORD	y;		// Y offset of this chunk
    DWORD	plane;		// 0=, 1=, 2=, 3=K
    DWORD	subplane;	// 0 or 1
    DWORD	pad[2];
} HDR_3C;

typedef struct
{
    DWORD	unk0;
    DWORD	unk1;
    DWORD	w;
    DWORD	h;
    DWORD	resx;
    DWORD	resy;
    DWORD	nbits;
    DWORD	unk7[4];
} HDR_3X;

void
decode(FILE *fp)
{
    OAK_HDR	hdr;
    int		rc;
    int		size;
    int		plane = 0;
    int		subplane;
    int		pageNum = 0;
    int		len;
    int		i, j;
    int		c;
    char	*p;
    int		curOff = 0;
    int		dwords[128];
    short	words[128];
    unsigned char	bytes[128];
    char	buf[512];
    HDR_0D	hdr0d;
    HDR_0C	hdr0c;
    HDR_3C	hdr3c;
    HDR_3X	hdr3x[4];
    int		firstPlane;
    size_t	cnt;
    char	*ibuf;
    struct jbg_dec_state	s[4][2];
    int		height[4][2];
    int		width[4][2];
    char        *strpaper[300+1];
    char        *strtype[15+1];
    #define STRARY(X, A) \
                            ((X) >= 0 && (X) < sizeof(A)/sizeof(A[0])) \
                            ? A[X] : "UNK"

    for (i = 0; i < sizeof(strpaper)/sizeof(strpaper[0]); ++i)
        strpaper[i] = "unk";
    strpaper[1] = "letter";
    strpaper[3] = "ledger";
    strpaper[5] = "legal";
    strpaper[6] = "statement";
    strpaper[7] = "executive";
    strpaper[8] = "A3";
    strpaper[9] = "A4";
    strpaper[11] = "A5";
    strpaper[12] = "B4";
    strpaper[13] = "B5jis";
    strpaper[14] = "folio";
    strpaper[19] = "env#9";
    strpaper[20] = "env#10";
    strpaper[27] = "envDL";
    strpaper[28] = "envC5";
    strpaper[30] = "envC4";
    strpaper[37] = "envMonarch";
    strpaper[257] = "A6";
    strpaper[258] = "B6";
    strpaper[259] = "B5iso";
    strpaper[260] = "env6";
    strpaper[296] = "CUSTOM";

    for (i = 0; i < sizeof(strtype)/sizeof(strtype[0]); ++i)
        strtype[i] = "unk";
    strtype[0] = "AutoSelect";
    strtype[1] = "Plain";
    strtype[2] = "Preprinted";
    strtype[3] = "Letterhead";
    strtype[4] = "GrayscaleTransparency";
    strtype[5] = "Prepunched";
    strtype[6] = "Labels";
    strtype[7] = "Bond";
    strtype[8] = "Recycled";
    strtype[9] = "Color";
    strtype[10] = "Cardstock";
    strtype[11] = "Heavy";
    strtype[12] = "Envelope";
    strtype[13] = "Light";
    strtype[14] = "Tough";

    for (;;)
    {
	static int	first3c = 1;

	rc = fread(&hdr, 1, len = sizeof(hdr), fp);
	if (rc <=0)
	    break;
	if (rc != len)
	{
	    debug(0, "Expected OAK header, got short read: %d bytes\n", rc);
	    break;
	}

	if (hdr.type == 0x3c && first3c)
	{
	    printf("\t"
		    "\t%8s %4s %4s %4s %8s %5s %5s %3s %4s %s %s\n",
		    "bih0", "w", "h", "l0", "bih5", "dlen", "plen",
		    "unk", "yOff", "P", "subP"
		  );
	    first3c = 0;
	}

	if (hdr.type != 0x3c || !SupressImage || !ImageRec[plane])
	{
	    if (PrintOffset)
		printf("%x:	", curOff);
	    printf("%02x (%d)", hdr.type, hdr.len);
	}

	curOff += len;
	size = hdr.len;
	size -= sizeof(hdr);

	switch (hdr.type)
	{
	case 0x0d:	// first record
	    rc = fread(&hdr0d, len = sizeof(hdr0d), 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    printf(" %x %s", hdr0d.unk, hdr0d.string);
	    break;
	case 0x0c:	// time
	    rc = fread(&hdr0c, len = sizeof(hdr0c), 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    p = strchr(hdr0c.datetime, '\n');
	    if (p) *p = 0;
	    printf(" %s", hdr0c.datetime);
	    printf(", %d", hdr0c.time_t);
	    printf(", %d/%02d/%02d %02d:%02d:%02d",
		    hdr0c.year, hdr0c.tm_mon+1, hdr0c.tm_mday,
		    hdr0c.tm_hour, hdr0c.tm_min, hdr0c.tm_sec);
	    break;
	case 0x0a:	// filename
	case 0x1f:	// Driver
	    if (hdr.type == OAK_TYPE_FILENAME)
		printf(" filename=");
	    else
		printf(" driver=");
	    curOff += size;
	    while (size--)
	    {
		c = fgetc(fp);
		if (c == EOF)
		    break;
		else if (c) putchar(c);
		else break;
	    }
	    if (size > 0)
		while (size--)
		    fgetc(fp);
	    break;
	case 0x0f:
	    rc = fread(dwords, len = 5*4, 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    printf("	Duplex=0x%x	Short=0x%x", dwords[0], dwords[1]);
	    break;

	case 0x14:
	    printf(" (no args)");
	    ++pageNum;
	    curOff += size;
	    while (size--)
		fgetc(fp);
	    break;
	case 0x28:
	    rc = fread(dwords, len = 1*4, 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    switch (dwords[0])
	    {
	    case 1:	printf(" Source=Tray1"); break;
	    case 2:	printf(" Source=Tray2"); break;
	    case 4:	printf(" Source=ManualFeed"); break;
	    case 7:	printf(" Source=Auto"); break;
	    default:	printf(" Source=%d", dwords[0]); break;
	    }
	    break;
	case 0x29:
	    rc = fread(bytes, len = 17*4, 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    printf(" PaperType=%s(%d) UNK8=%d,%d,%d, str='%s'",
		    STRARY(bytes[0], strtype), bytes[0],
		    bytes[1], bytes[2], bytes[3], &bytes[4]);
	    break;
	case 0x2a:
	    rc = fread(dwords, len = 5*4, 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    printf("	Copies=0x%x	Duplex=0x%x", dwords[0], dwords[1]);
	    break;
	case 0x2b:
	    rc = fread(dwords, len = 5*4, 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    printf("	papercode=%s(%d)",
		STRARY(dwords[0], strpaper), dwords[0]);
	    printf("	xwid=%d", dwords[1]);
	    printf("	ywid=%d", dwords[2]);
	    printf("	UNK=0x%x", dwords[3]);
	    break;
	case 0x32:
	    firstPlane = 0;
	    goto prplanes;
	case 0x33:
	    firstPlane = 3;
	prplanes:
	    printf("\n\tunk0	unk1	w	h	resx	resy	nBits");
	    for (i = firstPlane; i < 4; ++i)
	    {
		rc = fread(&hdr3x[i], len = sizeof(HDR_3X), 1, fp);
		if (rc != 1) goto out;
		curOff += len;
		size -= len;
		printf("\n\t0x%x\t0x%x\t%d\t%d\t%d\t%d\t0x%x",
			hdr3x[i].unk0,
			hdr3x[i].unk1,
			hdr3x[i].w,
			hdr3x[i].h,
			hdr3x[i].resx,
			hdr3x[i].resy,
			hdr3x[i].nbits);
	    }
	    curOff += size;
	    while (size-- > 0)
		fgetc(fp);
	    break;
	case 0x15:
	    printf(" (no args)");
	    curOff += size;
	    while (size--)
		fgetc(fp);
	    break;
	case 0x3c:
	    // rc = fread(dwords, len = 48, 1, fp);
	    rc = fread(&hdr3c, len = sizeof(hdr3c), 1, fp);
	    if (rc != 1)
	    {
		debug(0, "Short read of hdr3c\n");
		goto out;
	    }
	    curOff += len;

	    plane = hdr3c.plane;
	    subplane = hdr3c.subplane;
	    if (!SupressImage || !ImageRec[plane])
	    {
		printf(
		    "\t0x%08x %4d %4d %4d 0x%08x %5d %5d 0x%03x %4d %d %d\n",
			hdr3c.bih.opt1,
			hdr3c.bih.xd,
			hdr3c.bih.yd,
			hdr3c.bih.l0,
			hdr3c.bih.opt2,
			hdr3c.datalen,
			hdr3c.padlen,
			hdr3c.unk1C,
			hdr3c.y,
			hdr3c.plane,
			hdr3c.subplane);
	    }

	    if (RawFile && !FpRaw[plane][subplane])
	    {
		sprintf(buf, "%s-%02d-%d-%d.jbg",
			RawFile, pageNum, plane, subplane);
		FpRaw[plane][subplane] = fopen(buf, "w");
	    }
	    if (DecFile && !FpDec[plane][subplane])
	    {
		height[plane][subplane] = 0;
		sprintf(buf, "%s-%02d-%d-%d.pbm",
			DecFile, pageNum, plane, subplane);
		FpDec[plane][subplane] = fopen(buf, "w");
		fprintf(FpDec[plane][subplane], "P4\n%8d %8d\n", 0, 0);
	    }

	    if (FpDec[plane][subplane])
		jbg_dec_init(&s[plane][subplane]);

	    if (1||hdr3c.subplane == 0)
	    {
		static int testend = 1;
		static char *cp = (char *) &testend;
		static int first_bih = 1;
		// JBIGKIT is bigendian, Oak is little-endian
		// swap the BIH header words before passing it to
		// jbig-kit.  N.B. - is there any issues with endianess
		// inside the compressed stream itself????
		if (*cp == 1)
		{
		    iswap32(&hdr3c.bih.xd);
		    iswap32(&hdr3c.bih.yd);
		    iswap32(&hdr3c.bih.l0);
		}
		if (!SupressImage || !ImageRec[plane])
		    if (first_bih)
		    {
			first_bih = 0;
			print_bih((unsigned char *) &hdr3c);
		    }
		if (0 && *cp == 1)
		{
		    iswap32(&hdr3c.bih.xd);
		    iswap32(&hdr3c.bih.yd);
		    iswap32(&hdr3c.bih.l0);
		}
		if (FpRaw[plane][subplane])
		    rc = fwrite(&hdr3c.bih, 1, 20, FpRaw[plane][subplane]);
		if (FpDec[plane][subplane])
		{
		    rc = jbg_dec_in(&s[plane][subplane],
			    (unsigned char *)&hdr3c.bih, 20, &cnt);
		}
	    }

	    ImageRec[plane]++;

	    // image data
	    if (!hdr3c.padlen)
		break;
	    size = hdr3c.datalen;
	    ibuf = malloc(size);
	    rc = fread(ibuf, 1, size, fp);
	    if (rc <= 0)
		break;
	    curOff += size;
	    if (FpRaw[plane][subplane])
		rc = fwrite(ibuf, 1, size, FpRaw[plane][subplane]);
	    if (FpDec[plane][subplane])
	    {
		unsigned char *image;

		rc = JBG_EAGAIN;
		p = ibuf;
		while (size > 0 &&
			(rc == JBG_EAGAIN || rc == JBG_EOK))
		{
		    rc = jbg_dec_in(&s[plane][subplane],
			(unsigned char *) p, size, &cnt);
		    p += cnt;
		    size -= cnt;
		}
		if (rc)
		    debug(0, "rc= %d size=%d\n", rc, size);

		height[plane][subplane]
		    += jbg_dec_getheight(&s[plane][subplane]);
		width[plane][subplane]
		    = jbg_dec_getwidth(&s[plane][subplane]);
		image = jbg_dec_getimage(&s[plane][subplane], 0);
		if (image)
		    rc = fwrite(image, 1, jbg_dec_getsize(&s[plane][subplane]),
			FpDec[plane][subplane]);
		else
		    debug(0, "Missing image p=%d/%d %dx%d!\n",
			    plane, subplane,
			    jbg_dec_getwidth(&s[plane][subplane]),
			    jbg_dec_getheight(&s[plane][subplane]));
		jbg_dec_free(&s[plane][subplane]);
	    }
	    free(ibuf);

	    size = hdr3c.padlen - hdr3c.datalen;
	    curOff += size;
	    while (size--)
		c = fgetc(fp);
	    continue;
	case 0x17:
	    printf(" (no args)");
	    curOff += size;
	    for (i = 0; i < 4; ++i)
	    {
		for (j = 0; j < 2; ++j)
		{
		    if (FpRaw[i][j])
		    {
			fclose(FpRaw[i][j]);
			FpRaw[i][j] = NULL;
		    }
		    if (FpDec[i][j])
		    {
			fseek(FpDec[i][j], 0, 0);
			fprintf(FpDec[i][j], "P4\n%8d %8d\n",
				width[i][j], height[i][j]);
			fclose(FpDec[i][j]);
			FpDec[i][j] = NULL;
		    }
		}
	    }
	    while (size--)
		fgetc(fp);
	    break;
	case 0x18:
	    rc = fread(words, len = (1+1)*2, 1, fp);
	    if (rc != 1) goto out;
	    curOff += len;
	    printf(" UNK=%x", words[0]);
	    break;
	case 0x0b:
	    printf(" (no args)");
	    curOff += size;
	    while (size--)
		fgetc(fp);
	    break;
	default:
	    curOff += size;
	    while (size--)
		fgetc(fp);
	}

	printf("\n");
    }
out:
    return;
}

int
main(int argc, char *argv[])
{
	extern int	optind;
	extern char	*optarg;
	int		c;

	while ( (c = getopt(argc, argv, "d:ior:D:?h")) != EOF)
		switch (c)
		{
		case 'd': DecFile = optarg; break;
		case 'r': RawFile = optarg; break;
		case 'i': SupressImage = 1; break;
		case 'o': PrintOffset = 1; break;
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
	    decode(fp);
            fclose(fp);
	}
	else
	    decode(stdin);

	exit(0);
}
