/*
 * $Id: hbpldecode.c,v 1.66 2014/09/25 14:28:15 rick Exp $
 */

/*b
 * Copyright (C) 2011-2014
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
 * Authors: Rick Richardson <rick.richardson@comcast.net>
 * 	    Peter Korf <peter@niendo.de> (HBPL version 2)
 * 	    Dave Coffin <dcoffin@cybercom.net> (HBPL version 1)
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
"	hbpldecode [options] < hbpl-file\n"
"\n"
"	Decode a HBPL stream into human readable form.  HBPL is\n"
"	Host Based Printer Language.\n"
"\n"
"	There are two versions of HBPL in existence.\n"
"\n"
"	Version one is an HBPL stream with Huffman RLE data. This data is\n"
"	used by the Dell 1250c, Dell C1660w, Epson AcuLaser C1700, Fuji-Xerox\n"
"	cp105b, and similar printers. These printers are supported by\n"
"	foo2hbpl1-wrapper et al.\n"
"\n"
"	Version two is an HBPL stream with JBIG encoded data. This data\n"
"	is used by the Xerox WorkCentre 6015, Fuji Xerox Docuprint CM205\n"
"	Dell 1355c, and similar printers. These printers are supported by\n"
"	foo2hbpl2-wrapper et al.\n"
"\n"
"	Both versions can be decoded by hbpldecode.\n"
"\n"
"Options:\n"
"       -d basename Basename of .pbm file for saving decompressed planes\n"
"       -r basename Basename of .jbg file for saving raw planes\n"
"       -o          Print file offsets\n"
"       -h          Print hex file offsets\n"
"       -D lvl      Set Debug level [%d]\n"
    , Debug
    );

    exit(1);
}

/*
 * Hexdump stream data
 */
void
hexdump(FILE *fp, int decmode, char *lbl1, char *lbln, \
	    const void *vdata, int length)
{
	int			s;
	int			i;
	int			n;
	unsigned char		c;
	unsigned char		buf[16];
	const unsigned char	*data = vdata;

	if (length == 0)
	{
		fprintf(fp, "%s [length 0]\n", lbl1);
		return;
	}
	for (s = 0; s < length; s += 16)
	{
		fprintf(fp, "%s", s ? lbln : lbl1);
		fprintf(fp, decmode ? "%8d:" : "%08x:", s);
		n = length - s; if (n > 16) n = 16;
		for (i = 0; i < 16; ++i)
		{
			if (i == 8)
				fprintf(fp, " ");
			if (i < n)
				fprintf(fp, decmode ? " %3d" : " %02x",
				    buf[i] = data[s+i]);
			else
				fprintf(fp, "   ");
		}
		if (!decmode)
		{
		    fprintf(fp, "  ");
		    for (i = 0; i < n; ++i)
		    {
			    if (i == 8)
				    fprintf(fp, " ");
			    c = buf[i];
			    if (c >= ' ' && c < 0x7f)
				    fprintf(fp, "%c", c);
			    else
				    fprintf(fp, ".");
		    }
		}
		fprintf(fp, "\n");
	}
}

static int
getLEdword(unsigned char buf[4])
{
    return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | (buf[0] << 0);
}

static int
getLEword(unsigned char buf[2])
{
    return (buf[1] << 8) | (buf[0] << 0);
}

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

void
proff(int curOff)
{
    if (PrintOffset)
        printf("%d:     ", curOff);
    else if (PrintHexOffset)
        printf("%6x:    ", curOff);
}

/*
 * Version 2 stuff
 */
void
decode_image(char *filename, int pagenum, int planenum,
		unsigned char *bih, unsigned char *jbig, int jbiglen)
{
    FILE			*dfp;
    struct jbg_dec_state        s;
    size_t			cnt;
    unsigned char		*image;
    char			buf[512];
    int				rc;

    if (filename == 0)
	return;
    jbg_dec_init(&s);
    rc = jbg_dec_in(&s, bih, 20, &cnt);
    if (rc == JBG_EIMPL)
	error(1, "JBIG uses unimplemented feature\n");
    rc = jbg_dec_in(&s, jbig, jbiglen, &cnt);
    if (rc == JBG_EOK)
    {
	int	h, w, len;
	h = jbg_dec_getheight(&s);
	w = jbg_dec_getwidth(&s);
	image = jbg_dec_getimage(&s, 0);
	len = jbg_dec_getsize(&s);
	if (image)
	{
	    sprintf(buf, "%s-%02d-%d.pbm",
		    filename, pagenum, planenum);
	    dfp = fopen(buf, "w");
	    if (dfp)
	    {
		fprintf(dfp, "P4\n%8d %8d\n", w, h);
		rc = fwrite(image, 1, len, dfp);
		fclose(dfp);
		dfp = NULL;
	    }
	}
	else
	    debug(0, "Missing image %dx%d!\n", h, w);
    }
    jbg_dec_free(&s);
}

void
decode2(FILE *fp, int curOff)
{
    // int		c;
    int		rc;
    // FILE	*dfp = NULL;
    int		pageNum = 1;
    int		len;
    // int		curOff = 0;
    // struct jbg_dec_state	s[5];
    // unsigned char	bih[4][20];
    // int			imageCnt[4] = {0,0,0,0};
    // int         	pn = 0;
    unsigned char	header[4];
    unsigned char	buf[512];
    int		w, h, wh_total, res, color, mediatype, papersize;
    int		p, offbih[4];
    #define STRARY(X, A) \
	((X) >= 0 && (X) < sizeof(A)/sizeof(A[0])) \
	? A[X] : "UNK"
    char *strsize[] = {
	/*00*/	"Custom", "A4", "B5", "A5", "Letter",
	/*05*/	"Executive", "FanFoldGermanLegal", "Legal", "unk", "env#10",
	/*10*/	"envMonarch", "envC5", "envDL", "unk", "unk",
	};

    char *strtype[] = {
	/*00*/	"unk", "Plain", "Bond", "LwCard", "LwGCard",
	/*05*/	"Labels", "Envelope", "Recycled", "Plain-side2", "Bond-side2",
	/*10*/	"LwCard-side2", "LwGCard-side2", "Recycled-side2",
	};

    proff(curOff); printf("[hbpldecode2]\n");
    for (;;)
    {
	len = 4;
	rc = fread(header, 1, len, fp);
	if (rc != len)
	{
	    error(1, "len=%d, but EOF on file\n", len);
	    return;
	}

	proff(curOff);
	
	if (header[1] == '%' && header[2] == '-') //end of file
	  len = 15;
	else
	{
	    if (header[1] == 'J' && header[2] == 'P')
		len = 60;	// JP doesn't have len
	    else 
		len = header[3];
	    printf("RECTYPE %c%c - size=%d ", header[1], header[2], len);
	}
	
	curOff += len+4;
	rc = fread(buf, 1, len, fp);
	if (rc != len)
	{
	    error(1, "len=%d, but EOF on file\n", len);
	    return;
	}

	if (0) {}
	else if (header[1] == '%' && header[2] == '-')
	{ 
	    buf[len] = 0x00;
	    printf("\\033%%-%c%s", header[3], buf);
	    return;
	} 
	else if (header[1] == 'J' && header[2] == 'P')
	{
	    printf("[Job Parameters]\n");

	    hexdump(stdout, 0, "", "", buf, len);
	    hexdump(stdout, 1, "", "", buf, len);
	    printf("\t\tsize/source(?) = %d(0x%02x)\n", buf[11], buf[11]);
	}
	else if (header[1] == 'D' && header[2] == 'M')
	{
	    printf("[DM]\n");
	    hexdump(stdout, 0, "", "", buf, len);
	}
	
	else if (header[1] == 'P' && header[2] == 'S')
	{
	    unsigned char	*mbuf;

	    printf("[Page Start]\n");
	    if (Debug)
		hexdump(stdout, 0, "", "", buf, len);
	    w = getLEdword(&buf[0]);
	    h = getLEdword(&buf[4]);
	    wh_total = getLEdword(&buf[8]);
	    res = getLEword(&buf[20]);
	    papersize = buf[16];
	    mediatype = buf[17];
	    color = buf[18];
	    printf("\t\tw,h = %dx%d, wh_total = %d, res = %d, color = %d\n",
		w, h, wh_total, res, color);
	    printf("\t\tmediatype = %s(%d), papersize = %s(%d)\n",
		STRARY(mediatype, strtype), mediatype,
		STRARY(papersize, strsize), papersize);

	    for (p = 0; p < 4; ++p)
	    {
		// offsets at 26, 30, 34, 38
		offbih[p] = getLEdword(&buf[22 + p*4]);
		printf("\t\toffbih[%d] = %d (0x%x)\n", p, offbih[p], offbih[p]);
	    }

	    len = getLEdword(&buf[12]);
	    mbuf = malloc(len);
	    if (!mbuf)
		error(1, "malloc on mbuf, size=%d failed\n", len);
	    rc = fread(mbuf, 1, len, fp);
	    if (rc != len)
	    {
		error(1, "len=%d, but EOF on file\n");
	    }
	    if (Debug > 2) hexdump(stdout, 0, "", "", mbuf, len);
	    if (color == 1)
	    {
		proff(curOff);
		printf("Yellow BIH:\n");
		print_bih(mbuf);
		printf("\t\t... %d(0x%x) of yellow data skipped...\n",
		    offbih[0]-20, offbih[0]-20);
		decode_image(DecFile, pageNum, 3,
		    mbuf, mbuf+20, offbih[0]-20);

		proff(curOff + offbih[0]);
		printf("Magenta BIH:\n");
		print_bih(mbuf + offbih[0]);
		printf("\t\t... %d(0x%x) of magenta data skipped...\n",
		    offbih[1]-20, offbih[1]-20);
		decode_image(DecFile, pageNum, 2,
		    mbuf, mbuf+20+offbih[0], offbih[1]-20);

		proff(curOff + offbih[0] + offbih[1]);
		printf("Cyan BIH:\n");
		print_bih(mbuf + offbih[0] + offbih[1]);
		printf("\t\t... %d(0x%x) of cyan data skipped...\n",
		    offbih[2]-20, offbih[2]-20);
		decode_image(DecFile, pageNum, 1,
		    mbuf, mbuf+20+offbih[0]+offbih[1], offbih[2]-20);

		proff(curOff + offbih[0] + offbih[1] + offbih[2]);
		printf("Black BIH:\n");
		print_bih(mbuf + offbih[0] + offbih[1] + offbih[2]);
		printf("\t\t... %d(0x%x) of black data skipped...\n",
		    offbih[3]-20, offbih[3]-20);
		decode_image(DecFile, pageNum, 4,
		    mbuf, mbuf+20+offbih[0]+offbih[1]+offbih[2], offbih[3]-20);
	    }
	    else
	    {
		proff(curOff);
		printf("Black BIH:\n");
		// hexdump(stdout, 0, "", "", &bih[0], 20);
		print_bih(mbuf);
		printf("\t\t... %d(0x%x) of black data skipped...\n",
		    offbih[3]-20, offbih[3]-20);
		decode_image(DecFile, pageNum, 0, mbuf, mbuf+20, offbih[3]-20);
	    }
	    free(mbuf);
	    curOff += len;
	    ++pageNum;
	}
	else if (header[1] == 'P' && header[2] == 'E')
	{
	    printf("[Page End]\n");
	    hexdump(stdout, 0, "", "", buf, len);
	}
	else
	{
	    printf("[Unknown]\n");
	    hexdump(stdout, 0, "", "", buf, len);
	}
    }
}

/*
 * Version 1 stuff
 */

unsigned short
get2(FILE *fp)
{
    unsigned char buf[2];
    if (fread (buf, 2, 1, fp))
	return getLEword (buf);
    return 0xffff;
}

unsigned
get4(FILE *fp)
{
    unsigned char buf[4];
    if (fread (buf, 4, 1, fp))
	return getLEdword(buf);
    return 0xffffffff;
}

typedef struct stream
{
    unsigned char *p;
    unsigned buf, bits;
} STREAM;

unsigned int
getbits(STREAM *s, int nbits)
{
    while (s->bits < nbits)
    {
	s->buf = (s->buf << 8) + *s->p++;
	s->bits += 8;
    }
    s->bits -= nbits;
    return s->buf << (32-s->bits-nbits) >> (32-nbits);
}

char
gethuff(STREAM *s, const char *huff)
{
    int i;
    i = getbits(s,huff[0]) * 2;
    s->bits += huff[0] - huff[i+2];
    return huff[i+1];
}

/*
   Runlengths are integers between 1 and 17057 encoded as follows:

	1	00
	2	010
	3	011
	4	100 0
	5	100 1
	6	101 00
	7	101 01
	8	101 10
	9	101 11
	10	110 0000
	11	110 0001
	12	110 0010
	   ...
	25	110 1111
	26	111 000 000
	27	111 000 001
	28	111 000 010
	29	111 000 011
	   ...
	33	111 000 111
	34	111 001 000
	   ...
	41	111 001 111
	42	111 010 000
	50	111 011 0000
	66	111 100 00000
	98	111 101 000000
	162	111 110 000000000
	674	111 111 00000000000000
	17057	111 111 11111111111111
*/
unsigned int
get_len(STREAM *s)
{
    const short code[] = { 3,3,3,4,5,6,9,14,26,34,42,50,66,98,162,674 };
    int i;

    switch (getbits(s,3))
    {
    case 0:
    case 1: s->bits++;
	  return  1;
    case 2: return  2;
    case 3: return  3;
    case 4: return  4 + getbits(s,1);
    case 5: return  6 + getbits(s,2);
    case 6: return 10 + getbits(s,4);
    }
    i = getbits(s,3);
    return code[8+i] + getbits(s,code[i]);
}

/*
   CMYK byte differences are encoded as follows:

	 0	000
	+1	001
	-1	010
	 2	011s0	s = 0 for +, 1 for -
	 3	011s1
	 4	100s00
	 5	100s01
	 6	100s10
	 7	100s11
	 8	101s000
	 9	101s001
	    ...
	 14	101s110
	 15	101s111
	 16	110s00000
	 17	110s00001
	 18	110s00010
	    ...
	 46	110s11110
	 47	110s11111
	 48	1110s00000
	 49	1110s00001
	    ...
	 78	1110s11110
	 79	1110s11111
	 80	1111s000000
	 81	1111s000001
	    ...
	 126	1111s101110
	 127	1111s101111
	 128	11111110000
*/
signed char
get_diff(STREAM *s)
{
    const short code[] = { 1,2,3,5,5,6,2,4,8,16,48,80 };
    int i, sign;

    switch (i = getbits(s, 3))
    {
      case 0: return 0;
      case 1: return 1;
      case 2: return -1;
      case 7: i += getbits(s, 1);
    }
    sign = getbits(s, 1);
    i = code[i+3] + getbits(s, code[i-3]);
    return sign ? -i:i;
}

void
decode1(FILE *fp, int ilen, int page, int color, int width, int height)
{
    static const char huff[2][68] =
    {
	{
	    5,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
	    0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
	    5,2,5,2,5,2,5,2,5,2,5,2,5,2,5,2,
	    1,3,1,3,1,3,1,3,2,5,3,5,4,5,6,5
	},
	{
	    5,5,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,
	    5,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,
	    0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,
	    1,3,1,3,1,3,1,3,2,5,3,5,4,5,6,5
	},
    };
    unsigned char *in, *stop;
    struct stream stream[5];
    int size, hsel = 0, bit = 0, token, raw, dir, run, r, s;
    int off = 0, row, col;
    union { int i; unsigned char c[4]; } *kcmy = NULL;
    char name[512], (*rgb)[3], rotor[]="01234";
    int dirs[] = { -1, 0, -1, 1, 2 };
    FILE *dfp;

    if (!(in = malloc (ilen))) return;
    r = fread (in, 1, ilen, fp);
    if (!DecFile)
    {
	free (in);
	printf ("Page %d image found -- use \"-d basename\" to decode\n", page);
	return;
    }
    size = width * (height+2) * 4;
    if (!size || size / width / (height+2) != 4 || !(kcmy = malloc (size)))
    {
	free (in);
	error (1, "Invalid dimensions for HBPLv1\n");
    }
    sprintf (name, "%s-%02d.p%cm", DecFile, page, color ? 'p':'g');
    printf ("Decoding page %d to %s ...\n", page, name);

    memset (stream, 0, sizeof stream);
    stream[0].p = in+48;
    for (s = 0; s < 4; s++)
	stream[s+1].p = stream[s].p + getLEdword (in+32+s*4);
    stop = stream[1].p;
    for (r = 1; r < 5; r++)
	dirs[r] -= width;
    if (!color) dirs[4] = -8;
  
    memset (kcmy, -color, size);
    rgb = (void *) kcmy;
    kcmy += width+1;

    while (stream[0].p < stop && off < height * width)
    {
	token = gethuff (stream, huff[hsel]);
	switch (token)
	{
	case 6:
	    hsel = !hsel;
	    getbits (stream, 1);
	    break;
	case 5:
	    for (s = 0; s <= color*3; s++)
		kcmy[off].c[s] = kcmy[off-1].c[s] + get_diff (stream+1+s);
	    off++;
	    bit = 0;
	    break;
	default:
	    run = get_len (stream);
	    raw = token + bit;
	    dir = dirs[rotor[raw]-'0'];
	    bit = (run < 17057);
	    while (run--)
	    {
		kcmy[off].i = kcmy[off+dir].i;
		off++;
	    }
	    if (raw)
	    {
		s = rotor[raw];
		for (r = raw; r; r--)
		    rotor[r] = rotor[r-1];
		rotor[0] = s;
	    }
	    break;
	}
    }
    free (in);
    if (!(dfp = fopen (name, "w")))
	error (1, name);
    fprintf (dfp, "P%d %d %d 255\n", 5+color, width, height);
    for (off = row = 0; row < height; row++)
    {
	for (col = 0; col < width; col++)
	{
	    if (color)
		for (s = 0; s < 3; s++)
		    rgb[col][s] = (kcmy[off].c[0]^255) *
				(kcmy[off].c[s+1]^255) / 255;
	    else
		rgb[0][col] = kcmy[off].c[0]^255;
	    off++;
	}
	fwrite (rgb, color*2+1, width, dfp);
    }
    fclose (dfp);
    free (rgb);
}

int
parse1(FILE *fp, int *curOff)
{
    int rectype, stoptype, type, subtype;
    int val[2] = { 0,0 }, page = 0, color = 0, width = 0, height = 0;
    int i;
    char *strsize[256] = {
	/*00*/	"Letter", "Legal", "A4", "Executive", "unk",
	/*05*/	"unk", "env#10", "envMonarch", "envC5", "envDL",
	/*10*/	"unk", "B5", "unk", "unk", "unk",
	/*15*/	"A5", "unk", "unk", "unk", "unk",
	};
    // 205 == "folio",  Sheesh
    for (i = 0; i < 256; ++i)
	if (strsize[i] == NULL)
	    strsize[i] = "Custom";
    strsize[205] = "folio";	// 8.5x13

    proff(*curOff); printf("[hbpldecode1]\n");
    while ((proff(*curOff), (*curOff)++, rectype = fgetc(fp)) != EOF)
    {
	printf("RECTYPE '%c' [0x%x]:\n", rectype, rectype);
	stoptype = 0;
	switch (rectype)
	{
	case 0x41:  stoptype = 0x83;	break;
	case 0x43:  stoptype = 0xA2;	break;
	case 0x52:  stoptype = 0xA4;	break;
	case 0x20:
	case 0x51:
	case 0x53:
	case 0x44:  break;
	case 0x42:  return 0;
	default:
		    (*curOff)--;
		    ungetc (rectype, fp);
		    printf ("Unknown rectype 0x%x at 0x%x(%d)\n",
				    rectype, *curOff, *curOff);
		    return 1;
	}
	if (!stoptype) continue;
	do
	{
	    type = fgetc(fp);
	    (*curOff)++;
again:	    switch ((*curOff)++, subtype = fgetc(fp))
	    {
	    case 0xa1: val[0] = fgetc(fp);  (*curOff)++;   break;
	    case 0xc2: val[1] = get2(fp);   *curOff += 2;
	    case 0xa2: val[0] = get2(fp);   *curOff += 2;  break;
	    case 0xc4: val[1] = get4(fp);   *curOff += 4;
	    case 0xc3:
	    case 0xa4: val[0] = get4(fp);   *curOff += 4;  break;
	    case 0xb1: goto again;
	    default: error (1, "Unknown subtype 0x%02x\n", subtype);
	    }
	    proff(*curOff);
	    printf("	%x %x: ", type, subtype);
	    switch (type)
	    {
	    case 0x94:
		printf("%d [paper=%s]\n", val[0], STRARY(val[0], strsize));
		break;
	    case 0x95:
		printf("%dx%d%s\n", val[1], val[0],
			val[0] ? " [WxH in 0.1mm units]":"");
		break;
	    case 0x99:
		printf("%d [page]\n", val[0]);
		page = val[0];
		break;
	    case 0x9d:
		printf("0x%x [%s]\n", val[0], val[0] == 9 ? "Mono" : "Color");
		color = val[0] != 9;
		break;
	    case 0x9a:
	    case 0xa2:
		printf("%dx%d (0x%x x 0x%x) [WxH]\n",
			val[1], val[0], val[1], val[0]);
		width  = val[1];
		height = val[0];
		break;
	    case 0xa4:
		printf("%d (0x%x) bytes of data...\n", val[0], val[0]);
		decode1 (fp, val[0], page, color, width, height);
		*curOff += val[0];
		break;
	    default:
		printf("0x%x\n", val[0]);
	    }
	} while (type != stoptype);
    }
    return 0;
}

void
decode(FILE *fp)
{
    int		c;
    // int		rc;
    // FILE	*dfp = NULL;
    // int		pageNum = 1;
    int		len;
    int		curOff = 0;
    //struct jbg_dec_state	s[5];
    // unsigned char	bih[4][20];
    // int			imageCnt[4] = {0,0,0,0};
    // int         	pn = 0;
    char		buf[70000];

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
	    if ((strcmp(buf, "@PJL ENTER LANGUAGE=HBPL\r\n") == 0)
		|| (strcmp(buf, "@PJL ENTER LANGUAGE=HBPL\n") == 0))
		break;
	}
    }

    c = getc(fp);
    ungetc(c, fp);
    if (c == 0x1b)
    {
	// Decode version 2, ESC based
	decode2(fp, curOff);
	goto done;
    }

    if (parse1 (fp, &curOff))
    {
	printf ("Continuing with hexdump...\n");
	if ( (len = fread(buf, 1, sizeof(buf), fp)) )
	hexdump (stdout, 0, "", "", buf, len);
	exit(1);
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
	if (strcmp(buf, "@PJL ENTER LANGUAGE=HBPL\n") == 0)
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
