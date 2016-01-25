/*
 * $Id: gipddecode.c,v 1.29 2014/01/24 19:25:47 rick Exp $
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

/*
 * Dell 1125 MFP
 * Lexmark X500
 *
 * Granite Systems, inc. a.k.a Monotype Imaging
 * Granite Image Printer Driver (gipddecode)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "slx.h"
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
"	gipddecode [options] < zjs-file\n"
"\n"
"	Decode Granite Image Printer Driver (GIPD) stream into human readable\n"
"	form. Granite Systems was acquired by Monotype Imaging.\n"
"\n"
"	A GIPD stream is the printer language used by some Lexmark\n"
"	printers, such as the X500 and Dell printers, such as the 1125 MFP.\n"
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

typedef struct
{
    char	data[128-4];
    char	magic[4];
} OFST;
typedef struct
{
    char	magic[4];
    int		len;
    int		len2;
} GDI;

void
decode(FILE *fp)
{
    DWORD	magic;
    SL_HEADER	hdr;
    int		c;
    int		rc;
    int		size;
    int		items;
    char	*codestr;
    FILE	*dfp = NULL;
    FILE	*rfp = NULL;
    int		pageNum = 0;
    int		padding;
    int		len;
    int		curOff = 0;
    struct jbg_dec_state	s[5];
    unsigned char	bih[20];
    //DWORD	bih[5];
    int			bihlen = 0;
    int			imageCnt[5] = {0,0,0,0,0};
    int         	pn = 0;
    int         	incrY = 0;
    int			totSize = 0;
    OFST	ofst;
    GDI		gdi;
    DWORD	dbuf[256];
    int		ocnt = 0;
    int		bcnt = 0;
    int		ver;
    int		i;

    for (;;)
    {
	rc = fread(&ofst, sizeof(ofst), 1, fp);
	if (rc != 1)
	    error(1, "aaa\n");
	if (memcmp(ofst.magic, "OFST", 4) != 0)
	{
	    error(1, "magic %s\n", ofst.magic);
	    exit(1);
	}
	else
	{
	    if (ocnt < 5 || ocnt >= 1275)
	    {
		if (PrintOffset)
		    printf("%d:	", curOff);
		else if (PrintHexOffset)
		    printf("%6x:	", curOff);
		printf("OFST	%d	len=%d\n", ocnt, (int) sizeof(ofst));
	    }
	    else if (ocnt == 5)
		printf("\t[SNIP]\n");
	    ++ocnt;
	}
	curOff += sizeof(ofst);
	c = getc(fp);
	ungetc(c, fp);
	if (c == 'G')
	    break;
	if (c == '\033')
	    break;
	//debug(0, "got\n");
    }

    totSize = curOff;

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
	    if (strcmp(buf, "@PJL SET REVERSEPRINT=OFF\r\n") == 0)
		break;
	}
	if (feof(fp))
	{
	    printf("\n");
	    return;
	}
    }

    c =  getc(fp);
    ungetc(c, fp);
    ver = 0; //Dell
    if (c == '\033')
    {
	rc = fread(&ofst, 9, 1, fp);	// %-12345X
	ver = 1; //Lexmark
	if (PrintOffset)
	    printf("%d:	", curOff);
	else if (PrintHexOffset)
	    printf("%6x:	", curOff);
	printf("\\033%-8.8s\n", ofst.data+1);
	curOff += 9;
    }
    for (;;)
    {
	if (PrintOffset)
	    printf("%d:	", curOff);
	else if (PrintHexOffset)
	    printf("%6x:	", curOff);
	rc = fread(&gdi, sizeof(gdi), 1, fp);	// %-12345X
	curOff += sizeof(gdi);
	if (memcmp(gdi.magic, "JIDG", 4) == 0)
	{
	    printf("%-4.4s\n", gdi.magic);
	    printf("Total Size = %d (0x%x)\n",
		curOff - totSize, curOff - totSize);
	    return;
	}
	if (memcmp(gdi.magic, "PIDG", 4) == 0)
	{
	    printf("%-4.4s\n", gdi.magic);
	    continue;
	}
	if (memcmp(gdi.magic, "GDI", 3) != 0)
	    error(1, "Missing GDIJ (%-4.4s)\n", gdi.magic);
	if (gdi.magic[3] == 'B')
	{
	    if (ver == 0)
		gdi.len =  0x20 + be32(gdi.len2);
	    else
		gdi.len =  0xBC + be32(gdi.len2);
	}
	else
	    gdi.len = be32(gdi.len);
	// debug(0, "%.4s	len=%d\n", gdi.magic, gdi.len);
	gdi.len -= sizeof(gdi);
	switch (gdi.magic[3])
	{
	case 'J':
	    printf("%.4s	len=%d\n", gdi.magic, gdi.len);
	    bcnt = 0;
	    {
		int	unk0, unk1, unk2, unk3, unk4;
		int	unk5, unk6, unk7, unk8;
		int	paper;
		int	i;

		rc = fread(&dbuf, gdi.len, 1, fp);
		curOff += gdi.len;

		unk0 = be32(dbuf[0]);
		unk1 = be32(dbuf[1]);
		unk2 = be32(dbuf[2]);
		unk3 = be32(dbuf[3]);
		unk4 = be32(dbuf[4]);
		unk5 = be32(dbuf[5]);
		unk6 = be32(dbuf[6]);
		unk7 = be32(dbuf[7]);
		unk8 = be32(dbuf[8]);
		paper = be32(dbuf[9]);
		printf("\tunk0=%d, unk1=%d, unk2=%d, unk3=%d, unk4=%d\n",
		    unk0, unk1, unk2, unk3, unk4);
		printf("\tunk5=%d(0x%x), unk6=%d, unk7=%d, unk8=%d, paper=%d\n",
		    unk5, unk5, unk6, unk7, unk8, paper);
		gdi.len -= 4*10;
		printf("\t");
		for (i = 0; i < gdi.len; i += 4)
		    printf("%d, ", dbuf[10+i/4]);
		printf("\n");
		gdi.len = 0;
	    }
	    curOff += gdi.len;
	    //printf("cr=%x\n", curOff);
	    while (gdi.len--)
		getc(fp);
	    break;
	case 'P':
	    printf("%.4s	len=%d\n", gdi.magic, gdi.len);
	    bcnt = 0;
	    {
		int	w254; 
		int	h254;
		int	nplane;
		int	i;
		char	*buf = (char *) dbuf;

		rc = fread(&dbuf, gdi.len, 1, fp);
		curOff += gdi.len;

		w254 = (buf[0] << 8) + buf[1];
		h254 = (buf[2] << 8) + buf[3];
		nplane = buf[5*4];
		printf("\tnplane = %d, w254 = %d, h254 = %d\n",
		    nplane, w254, h254);
		
		printf("\t");
		for (i = 0; i < gdi.len; i += 4)
		    printf("%x, ", be32(dbuf[i/4]));
		printf("\n");
		gdi.len = 0;
	    }
	    curOff += gdi.len;
	    //printf("cr=%x\n", curOff);
	    while (gdi.len--)
		getc(fp);
	    break;
	case 'B':
	    //printf("cr=%x\n", curOff);
	    rc = fread(&dbuf, 3*4, 1, fp);
	    curOff += 3*4;
	    //debug(0, "%x\n", be32(gdi.len2));
	    //debug(0, "%x\n", be32(dbuf[0]));
	    //debug(0, "%x\n", be32(dbuf[1]));
	    //debug(0, "%x\n", be32(dbuf[2]));
	    gdi.len =  0x14 + be32(gdi.len2) + be32(dbuf[0])
			+ be32(dbuf[1]) + be32(dbuf[2]);
	    gdi.len -= sizeof(gdi);
	    
	    printf("%.4s	%2d	len=%d (0x%x)\n",
		gdi.magic, bcnt++, gdi.len, gdi.len);
	    //print_bih((unsigned char *)(dbuf+3));
	    if (0) {
		int	i;

		rc = fread(&dbuf, gdi.len, 1, fp);
		printf("\t0x%x\n", be32(dbuf[0]));
		printf("\t");
		for (i = 0; i < 4; i += 4)
		    printf("%d, ", be32(dbuf[i/4]));
		printf("\n");
		// gdi.len = 0;
	    }
	    curOff += gdi.len;
	    //printf("cr=%x\n", curOff);
	    i = 0;
	    if (Debug > 0)
		printf("\t");
	    while (gdi.len--)
	    {
		c = getc(fp);
		if (Debug > 0 && i++ < 48)
		{
		    printf("%02x, ", c);
		    if (i < 48 && (i % 16) == 0)
			printf("\n\t");
		}
	    }
	    if (Debug > 0)
		printf("\n");
	    break;
	default:
	    error(1, "unknown type '%c'\n", gdi.magic[3]);
	    break;
	}
    }

    /*
     * Software Imaging K.K.  SLX_MAGIC format
     */
    rc = fread(&magic, len = sizeof(magic), 1, fp);
    if (rc != 1)
    {
	printf("Missing SLX Magic number\n");
	return;
    }
    if (PrintOffset)
	printf("%d:	", curOff);
    else if (PrintHexOffset)
	printf("%6x:	", curOff);
    printf("SLX_MAGIC, 0x%lx (%.4s)\n", (long)magic, (char *) &magic);

    if (memcmp((char *) &magic, "\245SLX", 4) &&
	memcmp((char *) &magic, "XLS\245", 4))
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
	    CODESTR(SLT_START_DOC)	break;
	    CODESTR(SLT_END_DOC)	break;
	    CODESTR(SLT_START_PAGE)	++pageNum;
					memset(imageCnt, 0, sizeof(imageCnt));
					totSize = 0;
					break;
	    CODESTR(SLT_END_PAGE)	pn = 0;
					break;
	    CODESTR(SLT_JBIG_BIH)	break;
	    CODESTR(SLT_JBIG_BID)	break;
	    CODESTR(SLT_END_JBIG)	break;
	    CODESTR(SLT_SIGNATURE)	break;
	    CODESTR(SLT_RAW_IMAGE)	break;
	    CODESTR(SLT_START_PLANE)	break;
	    CODESTR(SLT_END_PLANE)	break;
	    CODESTR(SLT_2600N_PAUSE)	break;
	    CODESTR(SLT_2600N)		break;
	    default:			codestr = NULL; break;
	}

	if (codestr)
	    printf("%s, %ld items", codestr, (long) hdr.items);
	else
	    printf("SLT_0x%lx, %ld items", (long) hdr.type, (long) hdr.items);
	if (hdr.size & 3)
	{
	    printf(" (unaligned size)");
	    padding = 4 - (hdr.size & 3);
	}
	else
	    padding = 0;
	if (hdr.reserved)
	    printf(" (reserved=0x%x)", hdr.reserved);
	if (hdr.signature != 0xa5a5)
	    printf(" (funny siggy 0x%x)", hdr.signature);
	if (hdr.type == SLT_START_PAGE)
	    printf(" [Page %d]", pageNum);
	if (hdr.type == SLT_JBIG_BIH)
	{
	    switch (++pn)
	    {
	    case 1:	printf(" [black]"); break;
	    case 2:	printf(" [cyan]"); break;
	    case 3:	printf(" [magenta]"); break;
	    case 4:	printf(" [yellow]"); break;
	    }
	}
	printf("\n");
	fflush(stdout);

	items = hdr.items;
	size = hdr.size - sizeof(hdr);

	while (items--)
	{
	    SL_ITEM_HEADER	ihdr;
	    int			isize;
	    DWORD		val;
	    char		buf[512];
	    int			i, c;

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
		CODESTR(SLI_PAGECOUNT)			break;
		CODESTR(SLI_DMCOLLATE)			break;
		CODESTR(SLI_DMDUPLEX)			break;
		CODESTR(SLI_DISPLAY)			break;
		CODESTR(SLI_DISPLAY_PC)			break;
		CODESTR(SLI_PRINT_SLOWLY)		break;
		CODESTR(SLI_REMOVE_SLEEP_MODE)		break;
		CODESTR(SLI_USE_SEP_SHEETS)		break;
		CODESTR(SLI_COUNT)			break;

		CODESTR(SLI_DMPAPER)			break;
		CODESTR(SLI_DMCOPIES)			break;
		CODESTR(SLI_DMDEFAULTSOURCE)		break;
		CODESTR(SLI_DMMEDIATYPE)		break;
		CODESTR(SLI_NBIE)			break;
		CODESTR(SLI_RESOLUTION_X)		break;
		CODESTR(SLI_RESOLUTION_Y)		break;
		CODESTR(SLI_OFFSET_X)			break;
		CODESTR(SLI_OFFSET_Y)			break;
		CODESTR(SLI_RASTER_X)			break;
		CODESTR(SLI_RASTER_Y)			break;
		CODESTR(SLI_CUSTOM_X)			break;
		CODESTR(SLI_CUSTOM_Y)			break;
		CODESTR(SLI_VIDEO_X)			break;
		CODESTR(SLI_VIDEO_Y)			break;
		default:				codestr = NULL; break;
	    }

	    switch (ihdr.type)
	    {
	    case SLIT_UINT32:
		rc = fread(&val, len = sizeof(val), 1, fp);
		curOff += len;
		val = be32(val);
		isize -= 4;
		if (codestr)
		    printf("	%s, %ld (0x%lx) %s",
				codestr, (long) val, (long) val,
				ihdr.type == SLIT_INT32 ? "(int)" : "");
		else
		    printf("	SLI_0x%x, %ld (0x%lx) %s",
				ihdr.item, (long) val, (long) val,
				ihdr.type == SLIT_INT32 ? "(int)" : "");
		if (ihdr.item == SLI_NBIE)
		   ; // pn = (val & 7);
		break;
	    case SLIT_INT32:
	    case SLIT_STRING:
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
		    printf("	SLI_0x%x, '%s'", ihdr.item, buf);
		break;
	    default:
	    case SLIT_BYTELUT:
		rc = fread(&val, len = sizeof(val), 1, fp);
		curOff += len;
		val = be32(val);
		isize -= 4;
		if (codestr)
		    printf("	%s, BYTELUT (len=%d)", codestr, val);
		else
		    printf("	SLI_0x%x, BYTELUT (len=%d)", ihdr.item, val);
		if (0) // ihdr.item == SLI_JBIG_BIH && val == 20)
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
	    if (PrintOffset)
		printf("	%d:", curOff);
	    else if (PrintHexOffset)
		printf("	%6x:", curOff);
	    printf("	Data: %d bytes\n", size);
	    fflush(stdout);
	    totSize += size;

	    if (hdr.type == SLT_JBIG_BIH)
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

	    if ( (RawFile || DecFile) &&
		    (hdr.type == SLT_JBIG_BIH || hdr.type == SLT_JBIG_BID 
		    || hdr.type == SLT_2600N) )
	    {
		if (hdr.type == SLT_JBIG_BIH)
		{
		    if (RawFile)
		    {
			char	buf[512];
			sprintf(buf, "%s-%02d-%d.jbg",
				RawFile, pageNum, pn);
			rfp = fopen(buf, "w");
		    }
		    if (rfp)
			rc = fwrite(bih, bihlen, 1, rfp);
		    if (DecFile)
		    {
			size_t	cnt;

			// debug(1, "pn = %d\n", pn);
			imageCnt[pn] = 0;
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

			    // debug(1, "JBG_EOK: %d\n", pn);
			    h = jbg_dec_getheight(&s[pn]);
			    w = jbg_dec_getwidth(&s[pn]);
			    image = jbg_dec_getimage(&s[pn], 0);
			    len = jbg_dec_getsize(&s[pn]);
			    if (image)
			    {
				char	buf[512];
				sprintf(buf, "%s-%02d-%d.pbm",
					DecFile, pageNum, pn);
				dfp = fopen(buf,
					    imageCnt[pn] ? "a" : "w");
				if (dfp)
				{
				    if (imageCnt[pn] == 0)
					fprintf(dfp, "P4\n%8d %8d\n", w, h);
				    imageCnt[pn] += incrY;
				    rc = fwrite(image, 1, len, dfp);
				    fclose(dfp);
				}
			    }
			    else
				debug(0, "Missing image %dx%d!\n", h, w);
			    jbg_dec_free(&s[pn]);
			}
		    }
		}
	    }
	    else
	    {
		while (size--)
		{
		    fgetc(fp);
		    ++curOff;
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

	if (hdr.type == SLT_END_DOC)
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
