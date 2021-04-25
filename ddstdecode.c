/*
 * $Id: ddstdecode.c,v 1.7 2017/03/25 15:01:32 rick Exp $
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

#include "ddst.h"
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
"	ddstdecode [options] < zjs-file\n"
"\n"
"	Decode a Ricoh DDST stream into human readable form.\n"
"\n"
"	A Ricoh DDST stream is the printer language used by some Ricoh\n"
"	printers. From what I can tell, it is pbmtojbg wrapped with some PJL.\n"
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
	printf("%d:	", curOff);
    else if (PrintHexOffset)
	printf("%6x:	", curOff);
}

int		curOff = 0;
int		pageNum = 0;

void
decode(FILE *fp)
{
    DWORD	magic;
    XQX_HEADER	hdr;
    int		c;
    int		rc;
    int		i;
    char	*codestr;
    FILE	*dfp = NULL;
    int		planeNum = 4;
    int		len;
    struct jbg_dec_state	s[5];
    unsigned char	bih[20];
    int			bihlen = 0;
    int			imageCnt[5] = {0,0,0,0,0};
    int         	pn = 0;
    int         	incrY = 0;
    int			totSize = 0;
    int			imagelen = 0;
    int			startPagestatus = 0;

    /*
     * <unknown> XQX format
     */
    c = getc(fp);
    if (c == EOF)
    {
	printf("EOF on file reading header.\n");
	return;
    }
    ungetc(c, fp);
    if (c == '\033' || c == '@')
    {
	char	buf[1024];

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
	    if (0) {}
	    else if (strncmp(buf, "@PJL SET PAGESTATUS=START", 23) == 0)
	    {
		startPagestatus = 2;
		++pageNum;
	    }
	    else if (strncmp(buf, "@PJL SET PAGESTATUS=END", 21) == 0)
	    {
		startPagestatus = 0;
	    }
	    else if (strncmp(buf, "@PJL SET IMAGELEN=", 18) == 0)
	    {
		imagelen = atoi(&buf[18]);
		debug(2, "imagelen=%d\n", imagelen);
		break;
		proff(curOff);
		buf[51] = 0;
		printf("%s\n", buf);
		curOff += 43;
		proff(curOff);
		printf("\\033%s\n", buf+44);
		curOff += 9;
		break;
	    }
	}
	if (feof(fp))
	    return;
    }

    proff(curOff);
    printf("DDST_JBIG_DATA_BEGIN %d bytes\n", imagelen);
    for (i = 0; i < imagelen; )
    {
	unsigned char buf[4024*1024];
//printf("fread\n");
	rc = fread(buf, 1, imagelen, fp);
	if (i == 0 && startPagestatus == 2)
	{
	    memcpy(bih, buf, bihlen = 20);
//printf("data: %x\n", buf[3]);
	    if (DecFile && startPagestatus == 2)
	    {
		size_t	cnt;

		startPagestatus = 1;
		jbg_dec_init(&s[pn]);
		rc = jbg_dec_in(&s[pn], bih, bihlen, &cnt);
		if (rc == JBG_EIMPL)
		    error(1, "JBIG uses unimplemented feature\n");
	    }
	    print_bih(bih);
	}
	i += rc;
	curOff += rc;
rc = 0;
	if (rc == 0)
	{
	    debug(1, "imagelen = %d startPagestatus = %d\n",
		imagelen, startPagestatus);
	    for (i = 0; i < imagelen; ++i)
	    {
//printf("data: %x\n", buf[3]);
		c = buf[i];
		if (DecFile)
		{
		    size_t		cnt;
		    unsigned char	ch = c;

		    rc = JBG_EAGAIN;
		    rc = jbg_dec_in(&s[pn], &ch, 1, &cnt);
//printf("in: rc=%d ch=%x cnt=%ld\n", rc, ch, cnt);
		    if (rc == JBG_EOK)
		    {
			int	h, w, len;
			unsigned char *image;

//printf("JBG_OK!\n");
			// debug(0, "JBG_EOK: %d\n", pn);
			h = jbg_dec_getheight(&s[pn]);
			w = jbg_dec_getwidth(&s[pn]);
			image = jbg_dec_getimage(&s[pn], 0);
			len = jbg_dec_getsize(&s[pn]);
			//debug(0, "OK image len = %d\n", len);
			if (image)
			{
			    char	buf[512];
			    sprintf(buf, "%s-%02d-%d.pbm",
				    DecFile, pageNum, planeNum);
			    dfp = fopen(buf,
					imageCnt[planeNum] ? "a" : "w");
			    if (dfp)
			    {
				if (imageCnt[planeNum] == 0)
				    fprintf(dfp, "P4\n%8d %8d\n", w, h);
				imageCnt[planeNum] += incrY;
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
	    break;
	}
    }
    proff(curOff);
    printf("DDST_JBIG_DATA_END\n");
    return;
    
    /*
     * ??? XQX_MAGIC format
     */
    rc = fread(&magic, len = sizeof(magic), 1, fp);
    if (rc != 1)
    {
	printf("Missing XQX Magic number\n");
	return;
    }

    proff(curOff);
    printf("XQX_MAGIC, 0x%lx (%.4s)\n", (long)magic, (char *) &magic);

    if (memcmp((char *) &magic, ",XQX", 4))
    {
	printf("	Don't understand magic number 0x%lx\n", (long)magic);
	return;
    }

    curOff += len;
    for (;;)
    {
	proff(curOff);

	rc = fread(&hdr, len = sizeof(hdr), 1, fp);
	if (rc != 1) break;
	curOff += len;

	hdr.type = be32(hdr.type);
	hdr.items = be32(hdr.items);

	#define	CODESTR(X) case X: codestr = #X;
	switch (hdr.type)
	{
	    CODESTR(XQX_START_DOC)	break;
	    CODESTR(XQX_END_DOC)	break;
	    CODESTR(XQX_START_PAGE)	++pageNum; break;
	    CODESTR(XQX_END_PAGE)	break;
	    CODESTR(XQX_START_PLANE)	break;
	    CODESTR(XQX_END_PLANE)	break;
	    CODESTR(XQX_JBIG)		break;
	    default:			codestr = NULL; break;
	}

	if (codestr)
	    printf("%s(%ld), %ld items",
		    codestr, (long) hdr.type, (long) hdr.items);
	else
	    printf("XQX_0x%lx, %ld items", (long) hdr.type, (long) hdr.items);

	if (hdr.type == XQX_START_PAGE)
	    printf(" [Page %d]", pageNum);
	printf("\n");

	if (hdr.type == XQX_JBIG)
	{
	    for (i = 0; i < hdr.items; ++i)
	    {
		c = fgetc(fp);
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

			// debug(0, "JBG_EOK: %d\n", pn);
			h = jbg_dec_getheight(&s[pn]);
			w = jbg_dec_getwidth(&s[pn]);
			image = jbg_dec_getimage(&s[pn], 0);
			len = jbg_dec_getsize(&s[pn]);
			if (image)
			{
			    char	buf[512];
			    sprintf(buf, "%s-%02d-%d.pbm",
				    DecFile, pageNum, planeNum);
			    dfp = fopen(buf,
					imageCnt[planeNum] ? "a" : "w");
			    if (dfp)
			    {
				if (imageCnt[planeNum] == 0)
				    fprintf(dfp, "P4\n%8d %8d\n", w, h);
				imageCnt[planeNum] += incrY;
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
	    curOff += hdr.items;
	    totSize += hdr.items;
	}
	else if (hdr.type == 2 && hdr.items == 0)
	    break;
	else
	{
	    for (i = 0; i < hdr.items; ++i)
	    {
		XQX_ITEM_HEADER	item;
		DWORD		val;
		int		j;

		proff(curOff);
		rc = fread(&item, len = sizeof(item), 1, fp);
		if (rc != 1) break;
		curOff += len;

		item.type = be32(item.type);
		item.size = be32(item.size);
		switch (item.type)
		{
		    CODESTR(XQXI_DMDUPLEX)		break;
		    CODESTR(XQXI_DMDEFAULTSOURCE)	break;
		    CODESTR(XQXI_DMMEDIATYPE)		break;
		    CODESTR(XQXI_RESOLUTION_X)		break;
		    CODESTR(XQXI_RESOLUTION_Y)		break;
		    CODESTR(XQXI_RASTER_X)		break;
		    CODESTR(XQXI_RASTER_Y)		break;
		    CODESTR(XQXI_VIDEO_BPP)		break;
		    CODESTR(XQXI_VIDEO_X)		break;
		    CODESTR(XQXI_VIDEO_Y)		break;
		    CODESTR(XQXI_ECONOMODE)		break;
		    CODESTR(XQXI_DMPAPER)		break;
		    CODESTR(XQXI_DUPLEX_PAUSE)		break;
		    CODESTR(XQXI_BIH)			break;
		    CODESTR(XQXI_END)			break;
		    default:			codestr = NULL; break;
		}
		if (item.size == 4)
		{
		    rc = fread(&val, len = sizeof(val), 1, fp);
		    if (rc != 1) break;
		    val = be32(val);
		    if (codestr)
			printf("	%s, %ld (0x%lx)",
				    codestr, (long) val, (long) val);
		    else
			printf("	XQXI_0x%x, %ld (0x%lx)",
				    item.type, (long) val, (long) val);
		}
		else if (item.size == 20)
		{
		    rc = fread(bih, bihlen = sizeof(bih), 1, fp);
		    if (rc != 1) break;
		    printf("	%s(0x%lx)\n", codestr, (long) item.type);
		    print_bih(bih);
		    if (DecFile)
		    {
			size_t	cnt;

			jbg_dec_init(&s[pn]);
			rc = jbg_dec_in(&s[pn], bih, bihlen, &cnt);
			if (rc == JBG_EIMPL)
			    error(1, "JBIG uses unimplemented feature\n");
		    }
		}
		else
		{
		    printf("	XQXI_0x%lx, %ld size,",
			    (long) item.type, (long) item.size);

		    for (j = 0; j < item.size; ++j)
		    {
			c = fgetc(fp);
			printf(" %02x" , c);
		    }
		}
		curOff += item.size;
		printf("\n");
	    }
	}
    }
    printf("Total size: %d bytes\n", totSize);
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
