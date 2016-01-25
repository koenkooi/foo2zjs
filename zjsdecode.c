/*
 * $Id: zjsdecode.c,v 1.84 2014/01/24 19:25:48 rick Exp $
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

#include "zjs.h"
#include "jbig.h"

/*
 * Global option flags
 */
int	Debug = 0;
char	*RawFile;
char	*DecFile;
int	PrintOffset = 0;
int	PrintHexOffset = 0;
int	DoPad = 1;

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
"	zjsdecode [options] < zjs-file\n"
"\n"
"	Decode a ZjStream into human readable form.\n"
"\n"
"	A ZjStream is the printer language used by some Minolta/QMS and\n"
"	HP printers, such as the 2300DL and LJ-1000.\n"
"\n"
"	More information on Zenographics ZjStream can be found at:\n"
"\n"
"		http://ddk.zeno.com\n"
"\n"
"Options:\n"
"       -d basename Basename of .pbm file for saving decompressed planes\n"
"       -r basename Basename of .jbg file for saving raw planes\n"
"       -h          Print hex file offsets\n"
"       -o          Print file offsets\n"
"       -p          Don't do 4 byte padding\n"
"       -D lvl      Set Debug level [%d]\n"
    , Debug
    );

    exit(1);
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

void
decode(FILE *fp)
{
    DWORD	magic;
    ZJ_HEADER	hdr;
    int		c;
    int		rc;
    int		size;
    int		items;
    char	*codestr;
    FILE	*dfp = NULL;
    FILE	*rfp = NULL;
    int		planeNum = 1;
    int		pageNum = 0;
    int		padding;
    int		len;
    int		curOff = 0;
    struct jbg_dec_state	s[5];
    unsigned char	bih[20];
    int			bihlen = 0;
    int			imageCnt[5] = {0,0,0,0,0};
    int         	pn = 0;
    int         	incrY = 0;
    int         	bpp = 1;
    int			totSize = 0;
    int			i;
    char		*strmedia[600+1];
    char		*strpage[300+1];
    char		*strsource[] = {
			    /*00*/ "eject", "tray1", "unk", "unk", "tray2",
			    /*05*/ "unk", "unk", "auto"
			};
    #define STRARY(X, A) \
                            ((X) >= 0 && (X) < sizeof(A)/sizeof(A[0])) \
                            ? A[X] : "UNK"

    for (i = 0; i < sizeof(strmedia)/sizeof(strmedia[0]); ++i)
        strmedia[i] = "unk";
    /* Konica / HP */
    strmedia[1] = "standard / plain";
    strmedia[2] = "transparency / transparency";
    strmedia[3] = "glossy / unknown";
    strmedia[257] = "envelope";
    strmedia[258] = "unk / light";
    strmedia[259] = "letterhead";
    strmedia[260] = "unk / bond";
    strmedia[261] = "thickstock / cardstock";
    strmedia[262] = "postcard / heavy 111-130g";
    strmedia[263] = "labels / rough";
    strmedia[265] = "unk / labels";
    strmedia[267] = "unk / envelope";
    strmedia[273] = "unk / vellum";
    strmedia[276] = "unk / z3: hp tough paper";
    strmedia[282] = "unk / medium / z3: mid-weight 96-110g";
    strmedia[283] = "unk / extra heavy 131-175g";
    strmedia[284] = "unk / z3: heavy glossy 111-130g";
    strmedia[285] = "unk / z3: extra heavy glossy 131-175g";
    strmedia[286] = "unk / z3: card glossy 176-220g";
    strmedia[287] = "unk / z3: heavy envelope";
    strmedia[289] = "unk / z3: heavy rough";
    strmedia[305] = "unk / z3: hp laserjet 90g";
    strmedia[306] = "unk / z3: hp color laser matte 105g";
    strmedia[307] = "unk / z3: hp premium choice matte 120g";
    strmedia[308] = "unk / z3: hp color laser brochure matte 160g";
    strmedia[309] = "unk / z3: hp superior laser matte 160g";
    strmedia[310] = "unk / z3: hp cover matte 200g";
    strmedia[311] = "unk / z3: hp presentation glossy 130g";
    strmedia[312] = "unk / z3: hp color laser brochure glossy 160g";
    strmedia[313] = "unk / z3: hp superior laser glossy 160g";
    strmedia[314] = "unk / z3: hp color laser photo glossy 220g";
    strmedia[316] = "unk / z3: hp tri-fold color laser brochure 160g";
    strmedia[317] = "unk / z3: hp matte photo 200g";
    strmedia[318] = "unk / z3: hp professional laser glossy 130g";
    strmedia[512] = "unk / color";
    strmedia[513] = "unk / letterhead";
    strmedia[514] = "unk / preprinted";
    strmedia[515] = "unk / prepunched";
    strmedia[516] = "unk / recycled";
    strmedia[600] = "unk / unspecified";

    for (i = 0; i < sizeof(strpage)/sizeof(strpage[0]); ++i)
        strpage[i] = "unk";
    strpage[1] = "letter";
    strpage[5] = "legal";
    strpage[9] = "a4";
    strpage[7] = "executive";
    strpage[258] = "fanfold german legal";
    strpage[11] = "a5";
    strpage[70] = "z2-a6"; /* p1102 */
    strpage[13] = "b5jis";
    strpage[259] = "b5iso";
    strpage[264] = "16k 195x270";
    strpage[263] = "16k 184x260";
    strpage[257] = "16k 197x273";
    strpage[260] = "z1-postcard";
    strpage[261] = "z1-double postcard";
    strpage[262] = "z1-a6"; /* hp 1020 */
    strpage[43] = "postcard";
    strpage[82] = "z2-double postcard rotated";
    strpage[20] = "env#10";
    strpage[37] = "envMonarch";
    strpage[34] = "envB5";
    strpage[28] = "envC5";
    strpage[27] = "envDL";
    strpage[268] = "z3-4x6";
    strpage[269] = "z3-5x8";
    strpage[270] = "z3-10x15cm";

    /*
     * Zenographics ZX format
     */
    c = getc(fp);
    if (c == EOF)
    {
	printf("EOF on file reading header.\n");
	return;
    }
    ungetc(c, fp);
    if (c == '\033')
    {
	char	buf[1024];

	while (fgets(buf, sizeof(buf), fp))
	{
	    if (PrintOffset)
		printf("%d:	", curOff);
	    else if (PrintHexOffset)
		printf("%6x:	", curOff);
	    if (buf[0] == '\033')
	    {
		printf("\\033");
		fputs(buf+1, stdout);
	    }
	    else
		fputs(buf, stdout);
	    curOff += strlen(buf);
	    if (strcmp(buf, "@PJL ENTER LANGUAGE = ZJS\r\n") == 0)
		break;
	    if (0) {}
            else if (strncmp(buf, "@PJL USTATUS TIMED = ", 21) == 0)
            {
                rc = fread(buf, 52, 1, fp);
                if (rc != 1) return;
                debug(2, "buf=%s\n", buf);
                proff(curOff);
                buf[51] = 0;
                printf("%s\n", buf);
                curOff += 43;
                proff(curOff);
                printf("\\033%s\n", buf+44);
                curOff += 9;
                break;
            }
            else if (strncmp(buf, "@PJL SET JOBATTR=", 17) == 0)
            {
                rc = fread(buf, 9, 1, fp);
                if (rc != 1) return;
                buf[9] = 0;
                curOff += 9;
                proff(curOff);
                printf("\\033%s\n", buf+1);
                curOff += 9;
                break;
            }
	}
	if (feof(fp))
	{
	    printf("\n");
	    return;
	}
    }

    /*
     * Zenographics ZJS_MAGIC format
     */
    rc = fread(&magic, len = sizeof(magic), 1, fp);
    if (rc != 1)
    {
	printf("Missing ZJS Magic number\n");
	return;
    }
    if (PrintOffset)
	printf("%d:	", curOff);
    else if (PrintHexOffset)
	printf("%6x:	", curOff);
    printf("ZJS_MAGIC, 0x%lx (%.4s)\n", (long)magic, (char *) &magic);

    if (memcmp((char *) &magic, "JZJZ", 4)
	&& memcmp((char *) &magic, ",XQX", 4))
    {
	printf("	Don't understand magic number 0x%lx\n", (long)magic);
	return;
    }

    curOff += len;
    for (;;)
    {
	if (PrintOffset)
	    printf("%d:	", curOff);
	else if (PrintHexOffset)
	    printf("%6x:	", curOff);

	rc = fread(&hdr, len = sizeof(hdr), 1, fp);
	if (rc != 1) break;
	curOff += len;

	hdr.type = be32(hdr.type);
	hdr.size = be32(hdr.size);
	hdr.items = be32(hdr.items);
	hdr.reserved = be16(hdr.reserved);
	hdr.signature = be16(hdr.signature);

	#define	CODESTR(X) case X: codestr = #X;
	switch (hdr.type)
	{
	    CODESTR(ZJT_START_DOC)	break;
	    CODESTR(ZJT_END_DOC)	break;
	    CODESTR(ZJT_START_PAGE)	++pageNum;
					memset(imageCnt, 0, sizeof(imageCnt));
					totSize = 0;
					break;
	    CODESTR(ZJT_END_PAGE)	planeNum = 1;
					break;
	    CODESTR(ZJT_JBIG_BIH)	break;
	    CODESTR(ZJT_JBIG_BID)	break;
	    CODESTR(ZJT_END_JBIG)	break;
	    CODESTR(ZJT_SIGNATURE)	break;
	    CODESTR(ZJT_RAW_IMAGE)	break;
	    CODESTR(ZJT_START_PLANE)	break;
	    CODESTR(ZJT_END_PLANE)	break;
	    CODESTR(ZJT_2600N_PAUSE)	break;
	    CODESTR(ZJT_2600N)		break;

	    CODESTR(ZJT_ZX_0x0d)	break;
	    CODESTR(ZJT_ZX_0x0e)	break;
	    CODESTR(ZJT_ZX_0x0f)	break;
	    default:			codestr = NULL; break;
	}

	if (codestr)
	    printf("%s, %ld items", codestr, (long) hdr.items);
	else
	    printf("ZJT_0x%lx, %ld items", (long) hdr.type, (long) hdr.items);
	if (DoPad == 0)
	    padding = 0;
	else if (hdr.size & 3)
	{
	    printf(" (unaligned size)");
	    padding = 4 - (hdr.size & 3);
	}
	else
	    padding = 0;
	if (hdr.reserved)
	    printf(" (reserved=0x%x)", hdr.reserved);
	if (hdr.signature != 0x5a5a)
	    printf(" (funny siggy 0x%x)", hdr.signature);
	if (hdr.type == ZJT_START_PAGE)
	    printf(" [Page %d]", pageNum);
	printf("\n");
	fflush(stdout);

	items = hdr.items;
	size = hdr.size - sizeof(hdr);

	if (hdr.type == ZJT_ZX_0x0e)
	{
	    int	c;

	    if (PrintOffset)
		printf("	%d:", curOff);
	    else if (PrintHexOffset)
		printf("	%6x:", curOff);
	    printf("	Data: %d bytes (0x%x)\n", size, size);
	    fflush(stdout);
	    totSize += size;
	    
	    printf("%s0:\t", (PrintOffset||PrintHexOffset) ? "\t\t" : "\t");
	    for (i = 0; size--; ++i)
	    {
		c = fgetc(fp);
		++curOff;
		if (i < 16)
		    printf("%02x ", c);
		else if (size < 16)
		    printf("%02x ", c);
		if (i == 16)
		    printf(" ...\n%s",
			(PrintOffset||PrintHexOffset) ? "\t\t" : "\t");
		if (size == 16)
		    printf("%x:\t", i);
	    }
	    printf("\n");
	    continue;
	}

	while (items--)
	{
	    ZJ_ITEM_HEADER	ihdr;
	    int			isize;
	    DWORD		val;
	    char		buf[512];
	    int			c;

	    if (PrintOffset)
		printf("	%d:	", curOff);
	    else if (PrintHexOffset)
		printf("	%6x:	", curOff);

	    size -= sizeof(ihdr);

	    rc = fread(&ihdr, len = sizeof(ihdr), 1, fp);
	    if (rc != 1) break;
	    curOff += len;

	    ihdr.size = be32(ihdr.size);
	    ihdr.item = be16(ihdr.item);

	    isize = ihdr.size - sizeof(ihdr);
	    size -= isize;

	    switch (ihdr.item)
	    {
		CODESTR(ZJI_PAGECOUNT)			break;
		CODESTR(ZJI_DMCOLLATE)			break;
		CODESTR(ZJI_DMDUPLEX)			break;
		CODESTR(ZJI_DMPAPER)			break;
		CODESTR(ZJI_DMCOPIES)			break;
		CODESTR(ZJI_DMDEFAULTSOURCE)		break;
		CODESTR(ZJI_DMMEDIATYPE)		break;
		CODESTR(ZJI_NBIE)			break;
		CODESTR(ZJI_RESOLUTION_X)		break;
		CODESTR(ZJI_RESOLUTION_Y)		break;
		CODESTR(ZJI_OFFSET_X)			break;
		CODESTR(ZJI_OFFSET_Y)			break;
		CODESTR(ZJI_RASTER_X)			break;
		CODESTR(ZJI_RASTER_Y)			break;
		CODESTR(ZJI_MINOLTA_CUSTOM_X)		break;
		CODESTR(ZJI_MINOLTA_CUSTOM_Y)		break;
		CODESTR(ZJI_COLLATE)			break;
		CODESTR(ZJI_QUANTITY)			break;
		CODESTR(ZJI_VIDEO_BPP)			break;
		CODESTR(ZJI_VIDEO_X)			break;
		CODESTR(ZJI_VIDEO_Y)			break;
		CODESTR(ZJI_INTERLACE)			break;
		CODESTR(ZJI_PLANE)			break;
		CODESTR(ZJI_PALETTE)			break;
		CODESTR(ZJI_PAD)			break;
		CODESTR(ZJI_QMS_FINEMODE)		break;
		CODESTR(ZJI_QMS_OUTBIN)			break;
		CODESTR(ZJI_MINOLTA_PAGE_NUMBER)	break;
		CODESTR(ZJI_MINOLTA_FILENAME)		break;
		CODESTR(ZJI_MINOLTA_USERNAME)		break;
		CODESTR(ZJI_BITMAP_TYPE)		break;
		CODESTR(ZJI_BITMAP_PIXELS)		break;
		CODESTR(ZJI_BITMAP_BPP)			break;
		CODESTR(ZJI_BITMAP_STRIDE)		break;
		CODESTR(ZJI_INCRY)			break;
		CODESTR(ZJI_JBIG_BIH)			break;
		CODESTR(ZJI_RET)			break;
		CODESTR(ZJI_ECONOMODE)			break;
		CODESTR(ZJI_HP_CDOTS)			break;
		CODESTR(ZJI_HP_MDOTS)			break;
		CODESTR(ZJI_HP_YDOTS)			break;
		CODESTR(ZJI_HP_KDOTS)			break;
		CODESTR(ZJI_HP_CWHITE)			break;
		CODESTR(ZJI_HP_MWHITE)			break;
		CODESTR(ZJI_HP_YWHITE)			break;
		CODESTR(ZJI_HP_KWHITE)			break;

		// Zenographics ZJ format
		CODESTR(ZJI_ZX_0x6c)			break;
		CODESTR(ZJI_ZX_COLOR_OPT)		break;
		CODESTR(ZJI_ZX_0x6f)			break;
		default:				codestr = NULL; break;
	    }

	    switch (ihdr.type)
	    {
	    case ZJIT_UINT32:
	    case ZJIT_INT32:
		rc = fread(&val, len = sizeof(val), 1, fp);
		curOff += len;
		val = be32(val);
		isize -= 4;
		if (codestr)
		    printf("	%s, %ld (0x%lx)",
				codestr, (long) val, (long) val);
		else
		    printf("	ZJI_0x%x, %ld (0x%lx)",
				ihdr.item, (long) val, (long) val);
		if (ihdr.item == ZJI_PLANE)
		{
		    switch (planeNum = val)
		    {
		    case 0: case 4:	printf(" [black]"); break;
		    case 1:		printf(" [cyan]"); break;
		    case 2:		printf(" [yellow]"); break;
		    case 3:		printf(" [magenta]"); break;
		    }
		}
		else if (ihdr.item == ZJI_DMMEDIATYPE)
		    printf(" [%s]", STRARY(val, strmedia));
		else if (ihdr.item == ZJI_DMPAPER)
		    printf(" [%s]", STRARY(val, strpage));
		else if (ihdr.item == ZJI_DMDEFAULTSOURCE)
		    printf(" [%s]", STRARY(val, strsource));
		else if (ihdr.item == ZJI_INCRY)
		    incrY = val;
		else if (ihdr.item == ZJI_VIDEO_BPP)
		    bpp = val;
		else if (ihdr.item == ZJI_ZX_COLOR_OPT)
		{
		    switch (val)
		    {
		    case 1:	printf(" [mono]"); break;
		    case 3:	printf(" [rgb]"); break;
		    case 4:	printf(" [cmyk]"); break;
		    }
		}
		break;
	    case ZJIT_STRING:
		for (i = 0; i < sizeof(buf) - 1; )
		{
		    c = fgetc(fp);
		    if (c == EOF) break;
		    ++curOff;
		    buf[i++] = c;
		    --isize;
		    if (isize == 0 || c == 0) break;
		}
		buf[i] = 0;
		if (codestr)
		    printf("	%s, '%s'", codestr, buf);
		else
		    printf("	ZJI_0x%x, '%s'", ihdr.item, buf);
		break;
	    default:
	    case ZJIT_BYTELUT:
		rc = fread(&val, len = sizeof(val), 1, fp);
		curOff += len;
		val = be32(val);
		isize -= 4;
		if (codestr)
		    printf("	%s, BYTELUT (len=%d)", codestr, val);
		else
		    printf("	ZJI_0x%x, BYTELUT (len=%d)", ihdr.item, val);
		if (ihdr.item == ZJI_JBIG_BIH && val == 20)
		{
		    bihlen = fread(bih, 1, len = sizeof(bih), fp);
		    if (bihlen <= 0)
			isize = 0;
		    else
		    {
			isize -= bihlen;
		    	curOff += len;
		    }
		    if (bihlen == 20)
		    {
			printf("\n");
			print_bih(bih);
		    }
		}
		break;
	    }

	    if (ihdr.param != 0)
		printf(" (reserved=0x%x)", ihdr.param);
	    printf("\n");
	    fflush(stdout);

	    while (isize-- > 0)
	    {
		fgetc(fp);
		++curOff;
	    }

	    if (size <= 0 && items)
	    {
		printf("	#items is wrong!\n");
		break;
	    }
	}

	if (size)
	{
	    int	totlen = size;

	    if (PrintOffset)
		printf("	%d:", curOff);
	    else if (PrintHexOffset)
		printf("	%6x:", curOff);
	    printf("	Data: %d bytes\n", size);
	    fflush(stdout);
	    totSize += size;

	    if (hdr.type == ZJT_JBIG_BIH)
	    {
		bihlen = fread(bih, 1, len = sizeof(bih), fp);
		if (bihlen <= 0)
		    size = 0;
		else
		{
		    size -= bihlen;
		    curOff += len;
		}
		if (bihlen == 20)
		    print_bih(bih);
	    }

	    if (hdr.type == ZJT_2600N && hdr.items < 6)
	    {
		pn = planeNum;
		jbg_dec_init(&s[pn]);
		rc = jbg_dec_in(&s[pn], bih, 20, NULL);
		if (rc == JBG_EIMPL)
		    error(1, "JBIG uses unimplemented feature\n");
	    }

	    if ( (RawFile || DecFile) &&
		    (hdr.type == ZJT_JBIG_BIH || hdr.type == ZJT_JBIG_BID 
		    || hdr.type == ZJT_2600N) )
	    {
		if (hdr.type == ZJT_JBIG_BIH)
		{
		    if (RawFile)
		    {
			char	buf[512];
			sprintf(buf, "%s-%02d-%d.jbg",
				RawFile, pageNum, planeNum);
			rfp = fopen(buf, "w");
		    }
		    ++planeNum;
		    if (rfp)
			rc = fwrite(bih, bihlen, 1, rfp);
		    if (DecFile)
		    {
			size_t	cnt;

			jbg_dec_init(&s[pn]);
			rc = jbg_dec_in(&s[pn], bih, bihlen, &cnt);
			if (rc == JBG_EIMPL)
			    error(1, "JBIG uses unimplemented feature\n");
		    }
		}
		while (size--)
		{
		    int	c;
		    c = fgetc(fp);
		    ++curOff;
		    if ((totlen-size) <= 16)
		    {
			if ((totlen-size) == 1)
			    printf("\t");
			printf(" %02x", c);
			if ((totlen-size) == 16)
			    printf("\n\t...");
		    }
		    else if (size < 20)
		    {
			printf(" %02x", c);
			if (size == 0)
			    printf("\n");
		    }
		    if (rfp)
			fputc(c, rfp);
		    if (DecFile)
		    {
			size_t		cnt;
			unsigned char	ch = c;

			rc = JBG_EAGAIN;
			rc = jbg_dec_in(&s[pn], &ch, 1, &cnt);
			if (rc == JBG_EOK)
			{
			    int	h, w, len;
			    unsigned char *image;
			    int i, gray4;
			    char gray[4];

			    // debug(0, "JBG_EOK: %d\n", pn);
			    h = jbg_dec_getheight(&s[pn]);
			    w = jbg_dec_getwidth(&s[pn]);
			    image = jbg_dec_getimage(&s[pn], 0);
			    len = jbg_dec_getsize(&s[pn]);
			    if (image)
			    {
				char	buf[512];
				if (bpp == 1)
				    sprintf(buf, "%s-%02d-%d.pbm",
					DecFile, pageNum, planeNum-1);
				else
				    sprintf(buf, "%s-%02d-%d.pgm",
					DecFile, pageNum, planeNum-1);
				dfp = fopen(buf,
					    imageCnt[planeNum-1] ? "a" : "w");
				if (dfp)
				{
				    if (bpp == 1)
				    {
					if (imageCnt[planeNum-1] == 0)
					    fprintf(dfp, "P4\n%8d %8d\n", w, h);
					rc = fwrite(image, 1, len, dfp);
				    }
				    else
				    {
					if (imageCnt[planeNum-1] == 0)
					    fprintf(dfp, "P5\n%8d %8d 3\n",
						w/2, h);
					for (i = 0; i < len; ++i)
					{
					    gray4 = image[i];
					    gray[0] = ~(gray4 >> 6) & 3;
					    gray[1] = ~(gray4 >> 4) & 3;
					    gray[2] = ~(gray4 >> 2) & 3;
					    gray[3] = ~(gray4 >> 0) & 3;
					    rc = fwrite(gray, 4, 1, dfp);
					}
				    }
				    imageCnt[planeNum-1] += incrY;
				    fclose(dfp);
				}
			    }
			    else
				debug(0, "Missing image %dx%d!\n", h, w);
			    jbg_dec_free(&s[pn]);
			}
		    }
		}
		if (hdr.type == ZJT_2600N && hdr.items == 3)
		{
		    char	buf[512];
		    if (bpp == 1)
			sprintf(buf, "%s-%02d-%d.pbm",
			    DecFile, pageNum, planeNum-1);
		    else
			sprintf(buf, "%s-%02d-%d.pgm",
			    DecFile, pageNum, planeNum-1);
		    dfp = fopen(buf, "r+");
		    fseek(dfp, 12, 0);
		    fprintf(dfp, "%8d", imageCnt[planeNum-1]);
		    fclose(dfp);
		}
	    }
	    else
	    {
		while (size--)
		{
		    c = fgetc(fp);
		    ++curOff;
		    if ((totlen-size) <= 16)
		    {
			if ((totlen-size) == 1)
			    printf("\t");
			printf(" %02x", c);
			if ((totlen-size) == 16)
			    printf("\n\t...");
		    }
		    else if (size < 20)
		    {
			printf(" %02x", c);
			if (size == 0)
			    printf("\n");
		    }
		}
		if (rfp)
		{
		    fclose(rfp);
		    rfp = NULL;
		}
	    }
	}

	while (padding--)
	{
	    fgetc(fp);
	    ++curOff;
	}

	if (hdr.type == ZJT_END_DOC)
	    break;
    }
    if (rfp)
	fclose(rfp);
    printf("Total size: %d bytes\n", totSize);
}

int
main(int argc, char *argv[])
{
	extern int	optind;
	extern char	*optarg;
	int		c;

	while ( (c = getopt(argc, argv, "d:hopr:D:?h")) != EOF)
		switch (c)
		{
		case 'd': DecFile = optarg; break;
		case 'r': RawFile = optarg; break;
		case 'h': PrintHexOffset = 1; break;
		case 'o': PrintOffset = 1; break;
		case 'p': DoPad = 0; break;
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

	exit(0);
}
