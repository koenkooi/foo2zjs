/*

GENERAL
This program converts pbm (B/W) images and 1-bit-per-pixel cmyk images
(both produced by ghostscript) to Zenographics ZJ-stream format. There
is some information about the ZJS format at http://ddk.zeno.com.

With this utility, you can print to some HP, such as these:
     - HP LaserJet 2600n	mono or color
     - HP LaserJet 1600		mono or color
     - HP LaserJet CP1215	mono or color

AUTHORS
You can contact the current author at mailto:rick.richardson@comcast.net

LICENSE
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

If you want to use this program under different license conditions,
then contact the author for an arrangement.

It is possible that certain products which can be built using the jbig
software module might form inventions protected by patent rights in
some countries (e.g., by patents about arithmetic coding algorithms
owned by IBM and AT&T in the USA). Provision of this software by the
author does NOT include any licenses for any patents. In those
countries where a patent license is required for certain applications
of this software module, you will have to obtain such a license
yourself.

*/

static char Version[] = "$Id: foo2hp.c,v 1.48 2011/06/09 13:58:50 rick Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "jbig.h"
#include "zjs.h"
#include "cups.h"

/*
 * Command line options
 */
int	Debug = 0;
int	ResX = 600;
int	ResY = 600;
int	Bpp = 1;
int	PaperCode = DMPAPER_LETTER;
int	PageWidth = 600 * 8.5;
int	PageHeight = 600 * 11;
int	UpperLeftX = 0;
int	UpperLeftY = 0;
int	LowerRightX = 0;
int	LowerRightY = 0;
int	Copies = 1;
int	Duplex = DMDUPLEX_OFF;
int	SourceCode = DMBIN_AUTO;
int	MediaCode = DMMEDIA_STANDARD;
char	*Username = NULL;
char	*Filename = NULL;
int	Mode = 0;
		#define MODE_MONO	1
		#define MODE_COLOR	2

int	Color2Mono = 0;
int	BlackClears = 0;
int	AllIsBlack = 0;
int	OutputStartPlane = 1;
int	ExtraPad = 0;

int	LogicalOffsetX = 0;
int	LogicalOffsetY = 0;

int	CMYK_Offset[4] = {0, 0, 0, 0};

#define LOGICAL_CLIP_X	2
#define LOGICAL_CLIP_Y	1
int	LogicalClip = LOGICAL_CLIP_X | LOGICAL_CLIP_Y;
int	SaveToner = 0;
int	PageNum = 0;

int	IsCUPS = 0;

FILE	*EvenPages = NULL;
typedef struct
{
    off_t	b, e;
} SEEKREC;
SEEKREC	SeekRec[2000];
int	SeekIndex = 0;

long JbgOptions[5] =
{
    /* Order */
    JBG_ILEAVE | JBG_SMID,
    /* Options */
    JBG_DELAY_AT | JBG_LRLTWO | JBG_TPDON | JBG_TPBON | JBG_DPON,
    /* L0 */
    128,
    /* MX */
    16,
    /* MY */
    0
};

void
usage(void)
{
    fprintf(stderr,
"Usage:\n"
"   foo2hp [options] <pbmraw-file >zjs-file\n"
"\n"
"	Convert Ghostscript pbmraw format to a monochrome ZJS stream,\n"
"	for driving the Hewlett-Packard 2600n color laser printer\n"
"	and other Zenographics-based black and white printers.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"		-sPAPERSIZE=letter -r600x600 -sDEVICE=pbmraw \\ \n"
"		-sOutputFile=- - < testpage.ps \\ \n"
"	| foo2hp -r600x600 -g5100x6600 -p1 >testpage.zm\n"
"\n"
"   foo2hp [options] <bitcmyk-file >zjs-file\n"
"   foo2hp [options] <cups-file >zjs-file\n"
"\n"
"	Convert Ghostscript bitcmyk or cups format to a color ZJS stream,\n"
"	for driving the Hewlett-Packard 2600n color laser printer\n"
"	N.B. Color correction is expected to be performed by ghostscript.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"	    -sPAPERSIZE=letter -g5100x6600 -r600x600 -sDEVICE=bitcmyk \\ \n"
"	    -sOutputFile=- - < testpage.ps \\ \n"
"	| foo2hp -r600x600 -g5100x6600 -p1 >testpage.zc\n"
"\n"
"Normal Options:\n"
"-b bits           Bits per plane if autodetect doesn't work (1 or 2) [%d]\n"
"-c                Force color mode if autodetect doesn't work\n"
"-d duplex         Duplex code to send to printer [%d]\n"
"                    1=off, 2=longedge, 3=shortedge,\n"
//"                    4=manual longedge, 5=manual shortedge\n"
"-g <xpix>x<ypix>  Set page dimensions in pixels [%dx%d]\n"
"-m media          Media code to send to printer [%d]\n"
"		     1=plain 514=preprinted 513=letterhead 2=transparency\n"
"                    515=prepunched 265=labels 260=bond 516=recycled\n"
"                    512=color 276=tough 267=envelope 258=light 262=heavy\n"
"                    261=cardstock 268=lightglossy 269=glossy\n"
"                    270=heavyglossy 277=cover 278=photo\n"
"-p paper          Paper code to send to printer [%d]\n"
"                    1=letter, 5=legal 7=executive 9=A4 11=A5 13=B5jis\n"
"                    20=env#10, 27=envDL 28=envC5 34=envB5 37=envMonarch\n"
"-n copies         Number of copies [%d]\n"
"-r <xres>x<yres>  Set device resolution in pixels/inch [%dx%d]\n"
"-s source         Source code to send to printer [%d]\n"
"                    1=tray2 2=tray1 7=auto\n"
"                    Code numbers may vary with printer model\n"
"-t                Draft mode.  Every other pixel is white.\n"
"-J filename       Filename string to send to printer [%s]\n"
"-U username       Username string to send to printer [%s]\n"
"\n"
"Printer Tweaking Options:\n"
"-u <xoff>x<yoff>  Set offset of upper left printable in pixels [%dx%d]\n"
"-l <xoff>x<yoff>  Set offset of lower right printable in pixels [%dx%d]\n"
"-L mask           Send logical clipping values from -u/-l in ZjStream [%d]\n"
"                  0=no, 1=Y, 2=X, 3=XY\n"
"-A                AllIsBlack: convert C=1,M=1,Y=1 to just K=1\n"
"-B                BlackClears: K=1 forces C,M,Y to 0\n"
"                  -A, -B work with bitcmyk input only\n"
"-O c,m,y,k        Alignment of CMYK in rows [%d,%d,%d,%d]\n"
"-P                Do not output START_PLANE codes.  May be needed by some\n"
"                  some black and white only printers.\n"
"-X padlen         Add extra zero padding to the end of BID segments [%d]\n"
"\n"
"Debugging Options:\n"
"-S plane          Output just a single color plane from a color print [all]\n"
"                  1=Cyan, 2=Magenta, 3=Yellow, 4=Black\n"
"-D lvl            Set Debug level [%d]\n"
"-V                Version %s\n"
    , Bpp
    , Duplex
    , PageWidth , PageHeight
    , MediaCode
    , PaperCode
    , Copies
    , ResX , ResY
    , SourceCode
    , Filename ? Filename : ""
    , Username ? Username : ""
    , UpperLeftX , UpperLeftY
    , LowerRightX , LowerRightY
    , LogicalClip
    , CMYK_Offset[0], CMYK_Offset[1], CMYK_Offset[2], CMYK_Offset[3]
    , ExtraPad
    , Debug
    , Version
    );

  exit(1);
}

/*
 * Mirror1: bits 01234567 become 76543210
 */
unsigned char	Mirror1[256] =
{
	  0,128, 64,192, 32,160, 96,224, 16,144, 80,208, 48,176,112,240,
	  8,136, 72,200, 40,168,104,232, 24,152, 88,216, 56,184,120,248,
	  4,132, 68,196, 36,164,100,228, 20,148, 84,212, 52,180,116,244,
	 12,140, 76,204, 44,172,108,236, 28,156, 92,220, 60,188,124,252,
	  2,130, 66,194, 34,162, 98,226, 18,146, 82,210, 50,178,114,242,
	 10,138, 74,202, 42,170,106,234, 26,154, 90,218, 58,186,122,250,
	  6,134, 70,198, 38,166,102,230, 22,150, 86,214, 54,182,118,246,
	 14,142, 78,206, 46,174,110,238, 30,158, 94,222, 62,190,126,254,
	  1,129, 65,193, 33,161, 97,225, 17,145, 81,209, 49,177,113,241,
	  9,137, 73,201, 41,169,105,233, 25,153, 89,217, 57,185,121,249,
	  5,133, 69,197, 37,165,101,229, 21,149, 85,213, 53,181,117,245,
	 13,141, 77,205, 45,173,109,237, 29,157, 93,221, 61,189,125,253,
	  3,131, 67,195, 35,163, 99,227, 19,147, 83,211, 51,179,115,243,
	 11,139, 75,203, 43,171,107,235, 27,155, 91,219, 59,187,123,251,
	  7,135, 71,199, 39,167,103,231, 23,151, 87,215, 55,183,119,247,
	 15,143, 79,207, 47,175,111,239, 31,159, 95,223, 63,191,127,255,
};

/*
 * Mirror2: bits 01234567 become 67452301
 */
unsigned char	Mirror2[256] =
{
	  0, 64,128,192, 16, 80,144,208, 32, 96,160,224, 48,112,176,240,
	  4, 68,132,196, 20, 84,148,212, 36,100,164,228, 52,116,180,244,
	  8, 72,136,200, 24, 88,152,216, 40,104,168,232, 56,120,184,248,
	 12, 76,140,204, 28, 92,156,220, 44,108,172,236, 60,124,188,252,
	  1, 65,129,193, 17, 81,145,209, 33, 97,161,225, 49,113,177,241,
	  5, 69,133,197, 21, 85,149,213, 37,101,165,229, 53,117,181,245,
	  9, 73,137,201, 25, 89,153,217, 41,105,169,233, 57,121,185,249,
	 13, 77,141,205, 29, 93,157,221, 45,109,173,237, 61,125,189,253,
	  2, 66,130,194, 18, 82,146,210, 34, 98,162,226, 50,114,178,242,
	  6, 70,134,198, 22, 86,150,214, 38,102,166,230, 54,118,182,246,
	 10, 74,138,202, 26, 90,154,218, 42,106,170,234, 58,122,186,250,
	 14, 78,142,206, 30, 94,158,222, 46,110,174,238, 62,126,190,254,
	  3, 67,131,195, 19, 83,147,211, 35, 99,163,227, 51,115,179,243,
	  7, 71,135,199, 23, 87,151,215, 39,103,167,231, 55,119,183,247,
	 11, 75,139,203, 27, 91,155,219, 43,107,171,235, 59,123,187,251,
	 15, 79,143,207, 31, 95,159,223, 47,111,175,239, 63,127,191,255,
};

/*
 * Mirror4: bits 01234567 become 45670123
 */
unsigned char	Mirror4[256] =
{
	  0, 16, 32, 48, 64, 80, 96,112,128,144,160,176,192,208,224,240,
	  1, 17, 33, 49, 65, 81, 97,113,129,145,161,177,193,209,225,241,
	  2, 18, 34, 50, 66, 82, 98,114,130,146,162,178,194,210,226,242,
	  3, 19, 35, 51, 67, 83, 99,115,131,147,163,179,195,211,227,243,
	  4, 20, 36, 52, 68, 84,100,116,132,148,164,180,196,212,228,244,
	  5, 21, 37, 53, 69, 85,101,117,133,149,165,181,197,213,229,245,
	  6, 22, 38, 54, 70, 86,102,118,134,150,166,182,198,214,230,246,
	  7, 23, 39, 55, 71, 87,103,119,135,151,167,183,199,215,231,247,
	  8, 24, 40, 56, 72, 88,104,120,136,152,168,184,200,216,232,248,
	  9, 25, 41, 57, 73, 89,105,121,137,153,169,185,201,217,233,249,
	 10, 26, 42, 58, 74, 90,106,122,138,154,170,186,202,218,234,250,
	 11, 27, 43, 59, 75, 91,107,123,139,155,171,187,203,219,235,251,
	 12, 28, 44, 60, 76, 92,108,124,140,156,172,188,204,220,236,252,
	 13, 29, 45, 61, 77, 93,109,125,141,157,173,189,205,221,237,253,
	 14, 30, 46, 62, 78, 94,110,126,142,158,174,190,206,222,238,254,
	 15, 31, 47, 63, 79, 95,111,127,143,159,175,191,207,223,239,255,
};

/*
 * Mirror24: bits 01234567 become 10325476
 */
unsigned char   Mirror24[256] =
{
          0,  2,  1,  3,  8, 10,  9, 11,  4,  6,  5,  7, 12, 14, 13, 15,
         32, 34, 33, 35, 40, 42, 41, 43, 36, 38, 37, 39, 44, 46, 45, 47,
         16, 18, 17, 19, 24, 26, 25, 27, 20, 22, 21, 23, 28, 30, 29, 31,
         48, 50, 49, 51, 56, 58, 57, 59, 52, 54, 53, 55, 60, 62, 61, 63,
        128,130,129,131,136,138,137,139,132,134,133,135,140,142,141,143,
        160,162,161,163,168,170,169,171,164,166,165,167,172,174,173,175,
        144,146,145,147,152,154,153,155,148,150,149,151,156,158,157,159,
        176,178,177,179,184,186,185,187,180,182,181,183,188,190,189,191,
         64, 66, 65, 67, 72, 74, 73, 75, 68, 70, 69, 71, 76, 78, 77, 79,
         96, 98, 97, 99,104,106,105,107,100,102,101,103,108,110,109,111,
         80, 82, 81, 83, 88, 90, 89, 91, 84, 86, 85, 87, 92, 94, 93, 95,
        112,114,113,115,120,122,121,123,116,118,117,119,124,126,125,127,
        192,194,193,195,200,202,201,203,196,198,197,199,204,206,205,207,
        224,226,225,227,232,234,233,235,228,230,229,231,236,238,237,239,
        208,210,209,211,216,218,217,219,212,214,213,215,220,222,221,223,
        240,242,241,243,248,250,249,251,244,246,245,247,252,254,253,255,
};

void
rotate_bytes_180(unsigned char *sp, unsigned char *ep, unsigned char *mirror)
{
    unsigned char tmp;

    while (sp < ep)
    {
	tmp = mirror[*sp];
	*sp = mirror[*ep];
	*ep = tmp;
	++sp;
	--ep;
    }
    if (sp == ep)
	*sp = mirror[*sp];
}

void
debug(int level, char *fmt, ...)
{
    va_list ap;

    if (Debug < level)
	return;

    setvbuf(stderr, (char *) NULL, _IOLBF, BUFSIZ);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void
error(int fatal, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fatal)
	exit(fatal);
}

static void
chunk_write_rsvd(unsigned long type, unsigned int rsvd,
		    unsigned long items, unsigned long size, FILE *fp)
{
    ZJ_HEADER	chunk;
    int		rc;

    chunk.type = be32(type);
    chunk.items = be32(items);
    chunk.size = be32(sizeof(ZJ_HEADER) + size);
    chunk.reserved = be16(rsvd);
    chunk.signature = 0x5a5a;
    rc = fwrite(&chunk, 1, sizeof(ZJ_HEADER), fp);
    if (rc == 0) error(1, "fwrite(1): rc == 0!\n");
}

static void
chunk_write(unsigned long type, unsigned long items, unsigned long size,
	    FILE *fp)
{
    chunk_write_rsvd(type, 0, items, size, fp);
}

static void
item_uint32_write(unsigned short item, unsigned long value, FILE *fp)
{
    ZJ_ITEM_UINT32 item_uint32;
    int		rc;

    item_uint32.header.size = be32(sizeof(ZJ_ITEM_UINT32));
    item_uint32.header.item = be16(item);
    item_uint32.header.type = ZJIT_UINT32;
    item_uint32.header.param = 0;
    item_uint32.value = be32(value);
    rc = fwrite(&item_uint32, 1, sizeof(ZJ_ITEM_UINT32), fp);
    if (rc == 0) error(1, "fwrite(2): rc == 0!\n");
}

static int
item_str_write(unsigned short item, char *str, FILE *fp)
{
    int			lenpadded;
    ZJ_ITEM_HEADER	hdr;
    int			rc;

    lenpadded = 4 * ((strlen(str)+1 + 3) / 4);

    hdr.size = be32(sizeof(hdr) + lenpadded);
    hdr.item = be16(item);
    hdr.type = ZJIT_STRING;
    hdr.param = 0;
    if (fp)
    {
	rc = fwrite(&hdr, sizeof(hdr), 1, fp);
	if (rc == 0) error(1, "fwrite(3): rc == 0!\n");
	rc = fwrite(str, lenpadded, 1, fp);
	if (rc == 0) error(1, "fwrite(4): rc == 0!\n");
    }
    return (sizeof(hdr) + lenpadded);
}

static int
item_bytelut_write(unsigned short item, int size, unsigned char *p, FILE *fp)
{
    int			lenpadded;
    ZJ_ITEM_HEADER	hdr;
    DWORD		val;
    int			rc;

    lenpadded = 4 * ((size + 3) / 4);

    hdr.size = be32(sizeof(hdr) + 4 + lenpadded);
    hdr.item = be16(item);
    hdr.type = ZJIT_BYTELUT;
    hdr.param = 0;
    if (fp)
    {
	val = be32(size);
	rc = fwrite(&hdr, sizeof(hdr), 1, fp);
	if (rc == 0) error(1, "fwrite(5): rc == 0!\n");
	rc = fwrite(&val, 4, 1, fp);
	if (rc == 0) error(1, "fwrite(6): rc == 0!\n");
	rc = fwrite(p, lenpadded, 1, fp);
	if (rc == 0) error(1, "fwrite(7): rc == 0!\n");
    }
    return (sizeof(hdr) + 4 + lenpadded);
}

/*
 * A linked list of compressed data
 */
typedef struct _BIE_CHAIN{
    unsigned char	*data;
    size_t		len;
    struct _BIE_CHAIN	*next;
} BIE_CHAIN;

void
free_chain(BIE_CHAIN *chain)
{
    BIE_CHAIN	*next;
    next = chain;
    while ((chain = next))
    {
	next = chain->next;
	if (chain->data)
	    free(chain->data);
	free(chain);
    }
}

int
write_bitmap_plane(int planeNum, int eof, int incry, BIE_CHAIN **root, FILE *fp)
{
    BIE_CHAIN	*current = *root;
    BIE_CHAIN	*next;
    int		i, len, pad_len;
    #define	PADTO		4

    debug(3, "Write Plane %d\n", planeNum); 

    /* error handling */
    if (!current) 
	error(1,"There is no JBIG!\n"); 
    if (!current->next)
	error(1,"There is no or wrong JBIG header!\n"); 
    if (current->len != 20)
	error(1,"wrong BIH length\n"); 

    if (planeNum)
    {
	int size = 0;

	for (current = *root; current && current->len; current = current->next)
	{
	    if (current == *root)
		{}
		//size = current->len;
	    else
	    {
		len = current->len;
		next = current->next;
		if (!next || !next->len)
		    pad_len = ExtraPad + PADTO * ((len+PADTO-1)/PADTO) - len;
		else
		    pad_len = 0;
		size += len + pad_len;
	    }
	}
	if (eof)
	{
	    debug(3, "Write Plane %d %d\n", planeNum, eof); 
	    chunk_write_rsvd(ZJT_2600N, 0x24,
			    3, size + 3*sizeof(ZJ_ITEM_UINT32), fp);
	    item_uint32_write(ZJI_PLANE, planeNum, fp);
	    item_uint32_write(ZJI_INCRY, incry, fp);
	    item_uint32_write(0x67, 1, fp);
	}
	else
	{
	    chunk_write_rsvd(ZJT_2600N, 0xc,
			    1, size + 1*sizeof(ZJ_ITEM_UINT32), fp);
	    item_uint32_write(ZJI_PLANE, planeNum, fp);
	}
    }

    for (current = *root; current && current->len; current = current->next)
    {
	if (current == *root)
	{
	}
	else
	{
	    len = current->len;
	    next = current->next;
	    if (!next || !next->len)
		pad_len = ExtraPad + PADTO * ((len+PADTO-1)/PADTO) - len;
	    else
		pad_len = 0;
	    //chunk_write(ZJT_JBIG_BID, 0, len + pad_len, fp);
	    fwrite(current->data, 1, len, fp);
	    for (i = 0; i < pad_len; i++ )
		putc(0, fp);
	}
    }

    free_chain(*root);

    return 0;
}

void
start_bitmap_page(int w, int h, int nbie, FILE *ofp)
{
    static int		pageno = 0;
    int			nitems;

    nitems = 13;
    chunk_write_rsvd(ZJT_START_PAGE, 0x9c,
	nitems, nitems * sizeof(ZJ_ITEM_UINT32), ofp);
    item_uint32_write(ZJI_PLANE,               nbie,           ofp);
    item_uint32_write(ZJI_DMPAPER,             PaperCode,      ofp);
    item_uint32_write(ZJI_DMCOPIES,            Copies,         ofp);
    item_uint32_write(ZJI_DMDEFAULTSOURCE,     SourceCode,     ofp);
    item_uint32_write(ZJI_DMMEDIATYPE,         MediaCode,      ofp);
    item_uint32_write(ZJI_NBIE,                nbie,           ofp);
    item_uint32_write(ZJI_RESOLUTION_X,        ResX,           ofp);
    item_uint32_write(ZJI_RESOLUTION_Y,        ResY,           ofp);
    item_uint32_write(ZJI_RASTER_X,            w*Bpp,          ofp);
    item_uint32_write(ZJI_RASTER_Y,            h,              ofp);
    item_uint32_write(ZJI_VIDEO_BPP,           Bpp,            ofp);
    item_uint32_write(ZJI_VIDEO_X,             w,              ofp);
    item_uint32_write(ZJI_VIDEO_Y,             h,              ofp);
    // RES_X == ResX*Bpp, VIDEO_X == w; HP has a bug!!!! 

    //item_uint32_write(ZJI_OFFSET_X,            LogicalOffsetX, ofp);
    //item_uint32_write(ZJI_OFFSET_Y,            LogicalOffsetY, ofp);
    ++pageno;
    //item_uint32_write(ZJI_MINOLTA_PAGE_NUMBER, pageno,       ofp);
    if (IsCUPS)
	fprintf(stderr, "PAGE: %d %d\n", pageno, Copies);
}

void
end_page(int np, FILE *ofp)
{
    int		nitems;

    nitems = 8;
    chunk_write_rsvd(ZJT_END_PAGE, 0x60,
	nitems, nitems * sizeof(ZJ_ITEM_UINT32), ofp);
    item_uint32_write(0x8200,            (np>1) ? 1 : 0,           ofp);
    item_uint32_write(0x8201,            (np>1) ? 1 : 0,           ofp);
    item_uint32_write(0x8202,            (np>1) ? 1 : 0,           ofp);
    item_uint32_write(0x8203,            1,           		   ofp);
    item_uint32_write(0x8204,            (np>1) ? 1 : 0,           ofp);
    item_uint32_write(0x8205,            (np>1) ? 1 : 0,           ofp);
    item_uint32_write(0x8206,            (np>1) ? 1 : 0,           ofp);
    item_uint32_write(0x8207,            1,           		   ofp);
}

/*
 * This creates a linked list of compressed data.  The first item
 * in the list is the BIH and is always 20 bytes in size.  Each following
 * item is 65536 bytes in length.  The last item length is whatever remains.
 */
void
output_jbig(unsigned char *start, size_t len, void *cbarg)
{
    BIE_CHAIN	*current, **root = (BIE_CHAIN **) cbarg;
    int		size = 65536;	// Printer does strange things otherwise.
    int		i, state;
    unsigned char ch;

    // Delete everything after ff 02
    state = 0;
    for (i = 0; i < len; ++i)
    {
	ch = start[i];
	switch (state)
	{
	case 0:
	    if (ch == 0xff) state = 0xff;
	    break;
	case 0xff:
	    if (ch == 0x02) { len = i + 1; goto out; }
	    if (state == 0xff) state = 0;
	    break;
	}
    }
out:

    if ( (*root) == NULL)
    {
	(*root) = malloc(sizeof(BIE_CHAIN));
	if (!(*root))
	    error(1, "Can't allocate space for chain\n");

	(*root)->data = NULL;
	(*root)->next = NULL;
	(*root)->len = 0;
	size = 20;
	if (len != 20)
	    error(1, "First chunk must be BIH and 20 bytes long\n");
    }

    current = *root;  
    while (current->next)
	current = current->next;

    while (len > 0)
    {
	int	amt, left;

	if (!current->data)
	{
	    current->data = malloc(size);
	    if (!current->data)
		error(1, "Can't allocate space for compressed data\n");
	}

	left = size - current->len;
	amt = (len > left) ? left : len;
	memcpy(current->data + current->len, start, amt); 
	current->len += amt;
	len -= amt;
	start += amt;

	if (current->len == size)
	{
	    current->next = malloc(sizeof(BIE_CHAIN));
	    if (!current->next)
		error(1, "Can't allocate space for chain\n");
	    current = current->next;
	    current->data = NULL;
	    current->next = NULL;
	    current->len = 0;
	}
    }
}

int
write_bitmap_page(int w, int h, int np, unsigned char *bitmaps[4], FILE *ofp)
{
    int		x, y;
    int		p;
    int		size;
    int		i;
    DWORD	bih[5];
    int		w16;

    start_bitmap_page(w, h, np, ofp);
    if (Bpp == 2)
	w16 = (w + 63) & ~63;
    else
	w16 = (w + 127) & ~127;
    debug(2, "w16 = %d\n", w16);

    for (p = 0; p < np; ++p)
    {
	int	nitems;

	nitems = 7;
	size = nitems * sizeof(ZJ_ITEM_UINT32);
	++nitems;
	bih[0] = 1 << 8;
	bih[1] = w16*Bpp;
	bih[2] = 100;
	bih[3] = 128;
	bih[4] = (16 << 24) | ((JBG_ILEAVE|JBG_SMID) << 8) |
		    ((JBG_LRLTWO | JBG_TPDON | JBG_TPBON | JBG_DPON) << 0);
	for (i = 0; i < 5; ++i)
	    bih[i] = be32(bih[i]);
	size += item_bytelut_write(0, 20, (unsigned char *) bih, NULL);

	chunk_write_rsvd(ZJT_2600N, 0x74, nitems, size, ofp);
	item_uint32_write(ZJI_BITMAP_TYPE,    1,           		ofp);
	item_uint32_write(ZJI_BITMAP_PIXELS,  w16*Bpp,         		ofp);
	item_uint32_write(ZJI_BITMAP_STRIDE,  w16*Bpp,         		ofp);
	item_uint32_write(ZJI_INCRY,          100,         		ofp);
	item_uint32_write(ZJI_BITMAP_BPP,     1,           		ofp);
	item_uint32_write(ZJI_VIDEO_BPP,      Bpp,        	 	ofp);
	item_uint32_write(ZJI_PLANE,          (np==1) ? 4 : p+1,	ofp);
	item_bytelut_write(ZJI_JBIG_BIH, 20, (unsigned char *) bih,	ofp);

	if (Bpp == 2)
	{
	    for (y = 0; y < h; ++y)
		for (x = 0; x < w16*Bpp/8; ++x)
		    bitmaps[p][y*w16*Bpp/8 + x]
			    = Mirror24[ bitmaps[p][y*w16*Bpp/8 + x] ];
	}
    }

    for (y = 0; y < h; y += 100)
    {
	for (p = 0; p < np; ++p)
	{
	    struct jbg_enc_state se; 
	    BIE_CHAIN		*chain = NULL;
	    int	len, eof;

	    len = h - y;
	    if (len > 100)
		len = 100;
	    eof = (y+100) >= h;
	    jbg_enc_init(&se, w16*Bpp, len, 1, &bitmaps[p],
				output_jbig, &chain);
	    jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
				JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	    jbg_enc_out(&se);
	    jbg_enc_free(&se);

	    write_bitmap_plane((np==1) ? 4 : p+1, eof, len, &chain, ofp);
	    bitmaps[p] += (100*w16*Bpp + 7) / 8;
	}
    }

    end_page(np, ofp);
    return 0;
}

void
start_doc(FILE *fp)
{
    char		header[4] = "JZJZ";	// Big-endian data
    int			nitems;
    int			size;
    int			rc;

    rc = fwrite(header, 1, sizeof(header), fp);
    if (rc == 0) error(1, "fwrite(10): rc == 0!\n");

    nitems = 3;
    size = nitems * sizeof(ZJ_ITEM_UINT32);
    if (Username)
    {
	++nitems;
	size += item_str_write(0, Username, NULL);
    }
    if (Filename)
    {
	++nitems;
	size += item_str_write(0, Filename, NULL);
    }

    chunk_write_rsvd(ZJT_START_DOC, 0x24, nitems, size, fp);
    item_uint32_write(ZJI_DMCOLLATE, 0,		fp);
    item_uint32_write(ZJI_DMDUPLEX,  Duplex,	fp);
    item_uint32_write(ZJI_PAGECOUNT, 0,		fp);

    if (Username)
	item_str_write(ZJI_MINOLTA_USERNAME, Username, fp);
    if (Filename)
	item_str_write(ZJI_MINOLTA_FILENAME, Filename, fp);
}

void
end_doc(FILE *fp)
{
    int			nitems;

    nitems = 0;
    chunk_write(ZJT_END_DOC , nitems, nitems * sizeof(ZJ_ITEM_UINT32), fp);
}

void
load_tray2(FILE *fp)
{
    int			nitems;

    nitems = 0;
    chunk_write(ZJT_2600N_PAUSE, nitems, nitems * sizeof(ZJ_ITEM_UINT32), fp);
}

static int AnyColor;

void
cmyk_planes(unsigned char *plane[4], unsigned char *raw, int w, int h)
{
    int			rawbpl = (w+1) / 2;
    int			bpl = (w + 7) / 8;
    int			i;
    int			x, y;
    unsigned char	byte;
    unsigned char	mask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    int			aib = AllIsBlack;
    int			bc = BlackClears;

    bpl = (bpl + 15) & ~15;
    AnyColor = 0;
    for (i = 0; i < 4; ++i)
	memset(plane[i], 0, bpl * (h + abs(CMYK_Offset[i])) );

    //
    // Unpack the combined plane into individual color planes
    //
    // TODO: this can be speeded up using a 256 or 65536 entry lookup table
    //
    for (y = 0; y < h; ++y)
    {
	for (x = 0; x < w; ++x)
	{
	    byte = raw[y*rawbpl + x/2];

	    if (aib && (byte & 0xE0) == 0xE0)
	    {
		plane[3][y*bpl + x/8] |= mask[x&7];
	    }
	    else if (byte & 0x10)
	    {
		plane[3][y*bpl + x/8] |= mask[x&7];
		if (!bc)
		{
		    if (byte & 0x80) plane[0][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x40) plane[1][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x20) plane[2][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0xE0) AnyColor |= byte;
		}
	    }
	    else
	    {
		if (byte & 0x80) plane[0][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x40) plane[1][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x20) plane[2][y*bpl + x/8] |= mask[x&7];
		if (byte & 0xE0) AnyColor |= byte;
	    }

	    ++x;
	    if (aib && (byte & 0x0E) == 0x0E)
	    {
		plane[3][y*bpl + x/8] |= mask[x&7];
	    }
	    else if (byte & 0x1)
	    {
		plane[3][y*bpl + x/8] |= mask[x&7];
		if (!bc)
		{
		    if (byte & 0x8) plane[0][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x4) plane[1][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x2) plane[2][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0xE) AnyColor |= byte;
		}
	    }
	    else
	    {
		if (byte & 0x8) plane[0][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x4) plane[1][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x2) plane[2][y*bpl + x/8] |= mask[x&7];
		if (byte & 0xE) AnyColor |= byte;
	    }
	}
    }
    debug(2, "BlackClears = %d; AnyColor = %s %s %s\n",
	    BlackClears,
	    (AnyColor & 0x88) ? "Cyan" : "",
	    (AnyColor & 0x44) ? "Magenta" : "",
	    (AnyColor & 0x22) ? "Yellow" : ""
	    );
}

int
cmyk_page(unsigned char *raw, int w, int h, FILE *ofp)
{
    int i;
    int	bpl = (w + 7) / 8;
    int	bpl16 = (bpl + 15) & ~15;
    unsigned char *plane[4], *bitmaps[4];

    for (i = 0; i < 4; ++i)
    {
	plane[i] = malloc(bpl16 * (h + abs(CMYK_Offset[i])) );
	if (!plane[i]) error(3, "Cannot allocate space for bit plane\n");
	debug(1, "malloc plane[%d] = %x\n", i, plane[i]);
    }

    cmyk_planes(plane, raw, w, h);
    for (i = 0; i < 4; ++i)
    {
	if (Debug >= 9)
	{
	    FILE *dfp;
	    char fname[256];

	    sprintf(fname, "xxxplane%d", i);
	    dfp = fopen(fname, "w");
	    if (dfp)
	    {
		fwrite(plane[i], bpl*h, 1, dfp);
		fclose(dfp);
	    }
	}

	bitmaps[i] = plane[i] + CMYK_Offset[i] * bpl16;
    }

    if (Color2Mono)
	write_bitmap_page(w, h, 1, &bitmaps[Color2Mono-1], ofp);
    else if (AnyColor)
	write_bitmap_page(w, h, 4, bitmaps, ofp);
    else
	write_bitmap_page(w, h, 1, &bitmaps[3], ofp);

    for (i = 0; i < 4; ++i)
    {
	debug(1, "free plane[%d] = %x\n", i, plane[i]);
	free(plane[i]);
    }
    return 0;
}

int
pksm_page(unsigned char *plane[4], int w, int h, FILE *ofp)
{
    int i, j;
    unsigned char *bitmaps[4];

    int w16;
    int bpl;

    if (Bpp == 2)
        w16 = (w + 63) & ~63;
    else
        w16 = (w + 127) & ~127;

    // bytes per line
    bpl = (w16 * Bpp + 7) / 8;

    if (AnyColor && (AllIsBlack || BlackClears))
    {
	for (i = 0; i < h * bpl; ++i) 
	{
	    for (j = 0; j < 8; j += Bpp) 
	    {
  	        unsigned char mask = (Bpp == 2 ? 0x03 : 0x01) << j;

		if ((BlackClears && (plane[3][i] & mask) == mask) ||
		    (AllIsBlack &&
		    (plane[0][i] & plane[1][i] & plane[2][i] & mask) == mask)) 
		{
		    plane[0][i] &= ~mask;
		    plane[1][i] &= ~mask;
		    plane[2][i] &= ~mask;
		    plane[3][i] |= mask;
		}
	    }
	}
    }


    for (i = 0; i < 4; ++i) 
    {
         if (CMYK_Offset[i]) 
	 {
	     unsigned char *tmp = malloc(h * bpl);
	     if (!tmp) error(3, "Cannot allocate space for bitmap\n");
	     debug(1, "malloc bitmaps[%d] = %x\n", i, tmp);

	     if (CMYK_Offset[i] < 0) 
	     {
	         memcpy(tmp, plane[i] - CMYK_Offset[i] * bpl,
				    (h + CMYK_Offset[i]) * bpl);
		 memset(tmp + (h + CMYK_Offset[i]) * bpl, 0,
				    -CMYK_Offset[i] * bpl);
	     }
	     else 
	     {
	         memcpy(tmp + (CMYK_Offset[i] * bpl), plane[i],
				    (h - CMYK_Offset[i])  * bpl);
		 memset(tmp, 0, CMYK_Offset[i] * bpl);
	     }

	     free(plane[i]);
	     plane[i] = tmp;
	 }
	 bitmaps[i] = plane[i];
    }
  
    if (Color2Mono)
	write_bitmap_page(w, h, 1, &bitmaps[Color2Mono-1], ofp);
    else if (AnyColor)
	write_bitmap_page(w, h, 4, bitmaps, ofp);
    else
	write_bitmap_page(w, h, 1, &bitmaps[3], ofp);

    return 0;
}

int
pbm_page(unsigned char *buf, int w, int h, FILE *ofp)
{
    unsigned char	*bitmaps[1];

    if (SaveToner && Bpp == 1)
    {
	int	x, y;
	int	bpl, bpl16;

	bpl = (w + 7) / 8;
	bpl16 = (bpl + 15) & ~15;

	for (y = 0; y < h; y += 2)
	    for (x = 0; x < bpl16; ++x)
		buf[y*bpl16 + x] &= 0x55;
	for (y = 1; y < h; y += 2)
	    for (x = 0; x < bpl16; ++x)
		buf[y*bpl16 + x] &= 0xaa;
    }

    bitmaps[0] = buf;

    write_bitmap_page(w, h, 1, bitmaps, ofp);

    return 0;
}

int
read_and_clip_image(unsigned char *buf,
			int rawBpl, int rightBpl, int pixelsPerByte,
			int bpl, int h, int bpl16, FILE *ifp)
{
    unsigned char	*rowbuf, *rowp;
    int			y;
    int			rc;

    rowbuf = malloc(rawBpl);
    if (!rowbuf)
	error(1, "Can't allocate row buffer\n");

    // Clip top rows
    if (UpperLeftY)
    {
	for (y = 0; y < UpperLeftY; ++y)
	{
	    rc = fread(rowbuf, rawBpl, 1, ifp);
	    if (rc == 0)
		goto eof;
	    if (rc != 1)
		error(1, "Premature EOF(1) on input at y=%d\n", y);
	}
    }

    // Copy the rows that we want to image
    rowp = buf;
    for (y = 0; y < h; ++y, rowp += bpl16)
    {
	// Clip left pixel *bytes*
	if (UpperLeftX)
	{
	    rc = fread(rowbuf, UpperLeftX / pixelsPerByte, 1, ifp);
	    if (rc == 0 && y == 0 && !UpperLeftY)
		goto eof;
	    if (rc != 1)
		error(1, "Premature EOF(2) on input at y=%d\n", y);
	}

	if (bpl != bpl16)
	    memset(rowp, 0, bpl16);
	rc = fread(rowp, bpl, 1, ifp);
	if (rc == 0 && y == 0 && !UpperLeftY && !UpperLeftX)
	    goto eof;
	if (rc != 1)
		error(1, "Premature EOF(3) on input at y=%d\n", y);

	// Clip right pixels
	if (rightBpl != bpl)
	{
	    rc = fread(rowbuf, rightBpl - bpl, 1, ifp);
	    if (rc != 1)
		error(1, "Premature EOF(4) on input at y=%d\n", y);
	}
    }

    // Clip bottom rows
    if (LowerRightY)
    {
	for (y = 0; y < LowerRightY; ++y)
	{
	    rc = fread(rowbuf, rawBpl, 1, ifp);
	    if (rc != 1)
		error(1, "Premature EOF(5) on input at y=%d\n", y);
	}
    }

    free(rowbuf);
    return (0);

eof:
    free(rowbuf);
    return (EOF);
}

int
cmyk_pages(FILE *ifp, FILE *ofp)
{
    unsigned char	*buf;
    int			rawW, rawH, rawBpl;
    int			rightBpl;
    int			w, h, bpl;
    int			rc;

    //
    // Save the original Upper Right clip values as the logical offset,
    // because we may adjust them slightly below, in the interest of speed.
    //
    if (LogicalClip & LOGICAL_CLIP_X)
	LogicalOffsetX = UpperLeftX;
    if (LogicalClip & LOGICAL_CLIP_Y)
	LogicalOffsetY = UpperLeftY;

    rawW = PageWidth;
    rawH = PageHeight;
    rawBpl = (PageWidth + 1) / 2;

    // We only clip multiples of 2 pixels off the leading edge, and
    // add any remainder to the amount we clip from the right edge.
    // Its fast, and good enough for government work.
    LowerRightX += UpperLeftX & 1;
    UpperLeftX &= ~1;

    w = rawW - UpperLeftX - LowerRightX;
    h = rawH - UpperLeftY - LowerRightY;
    bpl = (w + 1) / 2;
    rightBpl = (rawW - UpperLeftX + 1) / 2;

    buf = malloc(bpl * h);
    if (!buf)
	error(1, "Unable to allocate page buffer of %d x %d = %d bytes\n",
		rawW, rawH, rawBpl * rawH);

    for (;;)
    {
	rc = read_and_clip_image(buf, rawBpl, rightBpl, 2, bpl, h, bpl, ifp);
	if (rc == EOF)
	    goto done;

	++PageNum;
	if (Duplex == DMDUPLEX_LONGEDGE && (PageNum & 1) == 0)
	    rotate_bytes_180(buf, buf + bpl * h - 1, Mirror4);
	if (Duplex == DMDUPLEX_MANUALLONG && (PageNum & 1) == 0)
	    rotate_bytes_180(buf, buf + bpl * h - 1, Mirror4);

	if ((PageNum & 1) == 0 && EvenPages)
	{
	    SeekRec[SeekIndex].b = ftell(EvenPages);
	    cmyk_page(buf, w, h, EvenPages);
	    SeekRec[SeekIndex].e = ftell(EvenPages);
	    debug(1, "CMYK Page: %d	%ld	%ld\n",
		PageNum, SeekRec[SeekIndex].b, SeekRec[SeekIndex].e);
	    SeekIndex++;
	}
	else
	    cmyk_page(buf, w, h, ofp);
    }

done:
    free(buf);
    return 0;
}

static unsigned long
getint(FILE *fp)
{
    int c;
    unsigned long i = 0;
    int rc;

    while ((c = getc(fp)) != EOF && !isdigit(c))
	if (c == '#')
	    while ((c = getc(fp)) != EOF && !(c == 13 || c == 10)) ;
    if (c != EOF)
    {
	ungetc(c, fp);
	rc = fscanf(fp, "%lu", &i);
	if (rc != 1) error(1, "fscanf: rc == 0!\n");
    }
    return i;
}

static void
skip_to_nl(FILE *fp)
{
    for (;;)
    {
	int c;
	c =  getc(fp);
	if (c == EOF)
	    error(1, "Premature EOF on input stream\n");
	if (c == '\n')
	    return;
    }
}

int
pbm_header(FILE *ifp, int *p4eatenp, int *wp, int *hp)
{
    int	c1, c2;

    if (*p4eatenp)
	*p4eatenp = 0;  // P4 already eaten in main
    else
    {
	c1 = getc(ifp);
	if (c1 == EOF)
	    return 0;
	c2 = getc(ifp);
	if (c1 != 'P' || c2 != '4')
	    error(1, "Not a pbmraw data stream\n");
    }

    skip_to_nl(ifp);

    *wp = getint(ifp);
    *hp = getint(ifp);
    skip_to_nl(ifp);
    return 1;
}

int
pksm_pages(FILE *ifp, FILE *ofp)
{
    unsigned char	*plane[4];
    int			rawW, rawH, rawBpl;
    int			saveW = 0, saveH = 0;
    int			rightBpl;
    int			w, h, bpl;
    int			bpl16;
    int			i;
    int			rc;
    int			p4eaten = 1;

    //
    // Save the original Upper Right clip values as the logical offset,
    // because we may adjust them slightly below, in the interest of speed.
    //
    if (LogicalClip & LOGICAL_CLIP_X)
	LogicalOffsetX = UpperLeftX;
    if (LogicalClip & LOGICAL_CLIP_Y)
	LogicalOffsetY = UpperLeftY;

    for (;;)
    {
	++PageNum;
	AnyColor = 0;

	for (i = 0; i < 4; ++i)
	{
	    if (pbm_header(ifp, &p4eaten, &rawW, &rawH) == 0)
	    {
		if (i == 0)
		    goto eof;
		else
		    error(1, "Premature EOF(pksm) on page %d hdr, plane %d\n",
			    PageNum, i);
	    }
	    if (i == 0)
	    {
		saveW = rawW;
		saveH = rawH;
	    }
	    if (saveW != rawW)
		error(1, "Image width changed from %d to %d on plane %d\n",
			saveW, rawW, i);
	    if (saveH != rawH)
		error(1, "Image height changed from %d to %d on plane %d\n",
			saveH, rawH, i);

	    rawBpl = (rawW + 7) / 8;

	    // We only clip multiples of 8 pixels off the leading edge, and
	    // add any remainder to the amount we clip from the right edge.
	    // Its fast, and good enough for government work.
	    LowerRightX += UpperLeftX & 7;
	    UpperLeftX &= ~7;

	    w = rawW - UpperLeftX - LowerRightX;
	    h = rawH - UpperLeftY - LowerRightY;
	    bpl = (w + 7) / 8;
	    rightBpl = (rawW - UpperLeftX + 7) / 8;

	    bpl16 = (bpl + 15) & ~15;
	    plane[i] = malloc(bpl16 * h);
	    if (!plane[i])
		error(1, "Can't allocate plane buffer\n");

	    rc = read_and_clip_image(plane[i],
				    rawBpl, rightBpl, 8, bpl, h, bpl16, ifp);
	    if (rc == EOF)
		error(1, "Premature EOF(pksm) on page %d data, plane %d\n",
			    PageNum, i);

	    if (Debug >= 9)
	    {
		FILE *dfp;
		char fname[256];

		sprintf(fname, "xxxplane%d", i);
		dfp = fopen(fname, "w");
		if (dfp)
		{
		    fwrite(plane[i], bpl*h, 1, dfp);
		    fclose(dfp);
		}
	    }

	    // See if we can optimize this to be a monochrome page
	    if (!AnyColor && i != 3)
	    {
		unsigned char *p, *e;

		for (p = plane[i], e = p + bpl16*h; p < e; ++p)
		    if (*p)
		    {
			AnyColor |= 1<<i;
			break;
		    }
	    }

	    if (Duplex == DMDUPLEX_LONGEDGE && (PageNum & 1) == 0)
		rotate_bytes_180(plane[i], plane[i] + bpl16 * h - 1, Mirror1);
	    if (Duplex == DMDUPLEX_MANUALLONG && (PageNum & 1) == 0)
		rotate_bytes_180(plane[i], plane[i] + bpl16 * h - 1, Mirror1);
	}

	debug(2, "AnyColor = %s %s %s\n",
		    (AnyColor & 0x01) ? "Cyan" : "",
		    (AnyColor & 0x02) ? "Magenta" : "",
		    (AnyColor & 0x04) ? "Yellow" : ""
		    );

	if ((PageNum & 1) == 0 && EvenPages)
	{
	    SeekRec[SeekIndex].b = ftell(EvenPages);
	    pksm_page(plane, w, h, EvenPages);
	    SeekRec[SeekIndex].e = ftell(EvenPages);
	    debug(1, "PKSM Page: %d	%ld	%ld\n",
		PageNum, SeekRec[SeekIndex].b, SeekRec[SeekIndex].e);
	    SeekIndex++;
	}
	else
	    pksm_page(plane, w, h, ofp);

	for (i = 0; i < 4; ++i)
	    free(plane[i]);
    }
eof:
    return (0);
}

int
pbm_pages(FILE *ifp, FILE *ofp)
{
    unsigned char	*buf;
    int			rawW, rawH, rawBpl;
    int			rightBpl;
    int			w, h, bpl;
    int			bpl16;
    int			rc;
    int			p4eaten = 1;

    //
    // Save the original Upper Right clip values as the logical offset,
    // because we may adjust them slightly below, in the interest of speed.
    //
    if (LogicalClip & LOGICAL_CLIP_X)
	LogicalOffsetX = UpperLeftX;
    if (LogicalClip & LOGICAL_CLIP_Y)
	LogicalOffsetY = UpperLeftY;

    for (;;)
    {
	if (pbm_header(ifp, &p4eaten, &rawW, &rawH) == 0)
	    break;

	rawBpl = (rawW + 7) / 8;

	// We only clip multiples of 8 pixels off the leading edge, and
	// add any remainder to the amount we clip from the right edge.
	// Its fast, and good enough for government work.
	LowerRightX += UpperLeftX & 7;
	UpperLeftX &= ~7;

	w = rawW - UpperLeftX - LowerRightX;
	h = rawH - UpperLeftY - LowerRightY;
	bpl = (w + 7) / 8;
	rightBpl = (rawW - UpperLeftX + 7) / 8;

	bpl16 = (bpl + 15) & ~15;
	buf = malloc(bpl16 * h);
	if (!buf)
	    error(1, "Can't allocate page buffer\n");

	rc = read_and_clip_image(buf, rawBpl, rightBpl, 8, bpl, h, bpl16, ifp);
	if (rc == EOF)
	    error(1, "Premature EOF(pbm) on input stream\n");

	++PageNum;
	if (Duplex == DMDUPLEX_LONGEDGE && (PageNum & 1) == 0)
	    rotate_bytes_180(buf, buf + bpl16 * h - 1, Mirror1);

	if ((PageNum & 1) == 0 && EvenPages)
	{
	    if (Duplex == DMDUPLEX_MANUALLONG)
		rotate_bytes_180(buf, buf + bpl16 * h - 1, Mirror1);
	    SeekRec[SeekIndex].b = ftell(EvenPages);
	    pbm_page(buf, w, h, EvenPages);
	    SeekRec[SeekIndex].e = ftell(EvenPages);
	    debug(1, "PBM Page: %d	%ld	%ld\n",
		PageNum, SeekRec[SeekIndex].b, SeekRec[SeekIndex].e);
	    SeekIndex++;
	}
	else
	    pbm_page(buf, w, h, ofp);

	free(buf);
    }
    return (0);
}

int
cups_pages(FILE *ifp, FILE *ofp)
{
    unsigned char	*plane[4];
    int			p, np = 1;
    int			bpc;
    int			bpp;
    int			co, cs;
    int			rawW, rawH, rawBpl;
    int			rightBpl;
    int			w, h, bpl, bpl16;
    int			rc;
    int			version = 0, swapped = 0, compress = 0;
    int			seek;
    unsigned		magic;
    cups_page_header_t	c1;
    cups_page_header2_t	c2;

    if (fread(&magic, sizeof(magic), 1, ifp) != 1)
	error(1, "Premature EOF reading magic number\n");
    switch (magic)
    {
    case CUPS_RASTER_SYNCv1:	version = 1; swapped = 0; compress = 0; break;
    case CUPS_RASTER_REVSYNCv1:	version = 1; swapped = 1; compress = 0; break;
    case CUPS_RASTER_SYNCv2:	version = 2; swapped = 0; compress = 1; break;
    case CUPS_RASTER_REVSYNCv2:	version = 2; swapped = 1; compress = 1; break;
    case CUPS_RASTER_SYNC:	version = 3; swapped = 0; compress = 0; break;
    case CUPS_RASTER_REVSYNC:	version = 3; swapped = 1; compress = 0; break;
    default:  error(1, "Illegal cups magic number '%4.4s'\n", (char *) &magic);
	break;
    }
    debug(2, "CUPS magic number '%4.4s', version=%d, swapped=%d, compress=%d\n",
	(char *) &magic, version, swapped, compress);

    if (swapped)
	error(1, "Don't know how to swap '%4.4s'\n", (char *) &magic);
    if (compress)
	error(1, "Don't know how to decompress '%4.4s'\n", (char *) &magic);

    //
    // Save the original Upper Right clip values as the logical offset,
    // because we may adjust them slightly below, in the interest of speed.
    //
    if (LogicalClip & LOGICAL_CLIP_X)
	LogicalOffsetX = UpperLeftX;
    if (LogicalClip & LOGICAL_CLIP_Y)
	LogicalOffsetY = UpperLeftY;

    // We only clip multiples of 4 pixels off the leading edge, and
    // add any remainder to the amount we clip from the right edge.
    // Its fast, and good enough for government work.
    LowerRightX += UpperLeftX & 3;
    UpperLeftX &= ~3;

    AnyColor = 1;
    for (;;)
    {
	seek = sizeof(magic); 
	if (fread(&c1, sizeof(c1), 1, ifp) != 1)
	    break;
	seek += sizeof(c1);
	if (version != 1)
	{
	    if (fread(&c2, sizeof(c2), 1, ifp) != 1)
		error(1, "Premature EOF skipping CUPS header V2\n");
	    seek += sizeof(c2);
	}

	++PageNum;

	rawW = c1.cupsWidth;
	rawH = c1.cupsHeight;
	// c1.cupsMediaType
	bpc = c1.cupsBitsPerColor;
	if (bpc != 2)
	    error(1, "Illegal number of bits per color (%d)\n", bpc);
	bpp = c1.cupsBitsPerPixel;
	rawBpl = c1.cupsBytesPerLine;
	co = c1.cupsColorOrder;
	if (co != CUPS_ORDER_PLANAR)
	    error(1, "Illegal color order (%d)\n", co);
	cs = c1.cupsColorSpace;
	switch (cs)
	{
	case CUPS_CSPACE_K:	np = 1; break;
	case CUPS_CSPACE_CMYK:	np = 4; break;
	default:	error(1, "Illegal color space (%d)\n", cs); break;
	}
	debug(2,"data at %ld\n", seek);

	debug(1, "%s: %d x %d x %d, %d (%d)\n",
		cs==3 ? "mono" : "color", rawW, rawH, bpp, rawBpl, rawBpl*8);

	w = rawW - UpperLeftX - LowerRightX;
	h = rawH - UpperLeftY - LowerRightY;
	bpl = (w + 3) / 4;
	debug(2, "w = %d, bpl = %d\n", w, bpl);
	rightBpl = (rawW - UpperLeftX + 3) / 4;

	bpl16 = (bpl + 15) & ~15;
	debug(2, "rawBpl = %d, bpl16 = %d\n", rawBpl, bpl16);

	for (p = 0; p < np; ++p)
	{
	    plane[p] = malloc(bpl16 * h);
	    if (!plane[p])
		error(1, "Unable to allocate plane[%d] of %d x %d = %d bytes\n",
			    p, rawW, rawH, rawBpl * rawH);
	}

	for (p = 0; p < np; ++p)
	{
	    rc = read_and_clip_image(plane[p], rawBpl, rightBpl, 4,
					bpl, h, bpl16, ifp);
	    if (rc == EOF)
		error(1, "Premature EOF on plane[%d]\n", p);

	    if (Duplex == DMDUPLEX_LONGEDGE && (PageNum & 1) == 0)
		rotate_bytes_180(plane[p], plane[p] + bpl16 * h - 1, Mirror1);
	    if (Duplex == DMDUPLEX_MANUALLONG && (PageNum & 1) == 0)
		rotate_bytes_180(plane[p], plane[p] + bpl16 * h - 1, Mirror1);
	}

	if ((PageNum & 1) == 0 && EvenPages)
	{
	    SeekRec[SeekIndex].b = ftell(EvenPages);
	    if (np == 1)
		pbm_page(plane[0], w, h, EvenPages);
	    else
		pksm_page(plane, w, h, EvenPages);
	    SeekRec[SeekIndex].e = ftell(EvenPages);
	    debug(1, "CUPS Page: %d	%ld	%ld\n",
		PageNum, SeekRec[SeekIndex].b, SeekRec[SeekIndex].e);
	    SeekIndex++;
	}
	else
	{
	    if (np == 1)
		pbm_page(plane[0], w, h, ofp);
	    else
		pksm_page(plane, w, h, ofp);
	}

	for (p = 0; p < np; ++p)
	    free(plane[p]);
    }

    return 0;
}

void
blank_page(FILE *ofp)
{
    int			w, h, bpl, bpl16;
    unsigned char	*plane;
    
    w = PageWidth - UpperLeftX - LowerRightX;
    h = PageHeight - UpperLeftY - LowerRightY;
    if (Bpp == 1)
	bpl = (w + 7) / 8;
    else
	bpl = (w + 3) / 4;
    bpl16 = (bpl + 15) & ~15;

    plane = malloc(bpl16 * h);
    if (!plane)
	error(1, "Unable to allocate blank plane (%d bytes)\n", bpl16*h);
    memset(plane, 0, bpl16*h);

    pbm_page(plane, w, h, ofp);
    ++PageNum;
    free(plane);
}

int
parse_xy(char *str, int *xp, int *yp)
{
    char	*p;

    if (!str || str[0] == 0) return -1;

    *xp = strtoul(str, &p, 10);
    if (str == p) return -2;
    while (*p && (*p < '0' || *p > '9'))
	++p;
    str = p;
    if (str[0] == 0) return -3;
    *yp = strtoul(str, &p, 10);
    if (str == p) return -4;
    return (0);
}

void
do_one(FILE *in)
{
    int	mode;

    mode = getc(in);
    if (mode == 't' || (mode >= '2' && mode <='5') )
    {
	ungetc(mode, in);
	cups_pages(in, stdout);
    }
    else if (Mode == MODE_COLOR)
    {
	if (mode != 'P')
	{
	    ungetc(mode, in);
	    cmyk_pages(in, stdout);
	}
	else
	{
	    mode = getc(in);
	    if (mode == '4')
		pksm_pages(in, stdout);
	    else
		error(1, "Not a pksmraw file!\n");
	}
    }
    else
    {
	if (mode != 'P')
	    error(1, "Not a pbm file!\n");
	mode = getc(in);
	if (mode == '4')
	    pbm_pages(in, stdout);
	else
	    error(1, "Not a pbmraw file!\n");
    }
}

int
main(int argc, char *argv[])
{
    int	c;
    int	rc;
    int i, j;

    while ( (c = getopt(argc, argv,
		    "b:cd:g:n:m:p:r:s:tu:l:L:ABO:PJ:S:U:X:D:V?h")) != EOF)
	switch (c)
	{
	case 'b':	Bpp = atoi(optarg);
			if (Bpp != 1 && Bpp != 2)
			    error(1, "Illegal value '%s' for -b\n", optarg);
			break;
	case 'c':	Mode = MODE_COLOR; break;
	case 'S':	Color2Mono = atoi(optarg);
			Mode = MODE_COLOR;
			if (Color2Mono < 0 || Color2Mono > 4)
			    error(1, "Illegal value '%s' for -C\n", optarg);
			break;
	case 'd':	Duplex = atoi(optarg); break;
	case 'g':	if (parse_xy(optarg, &PageWidth, &PageHeight))
			    error(1, "Illegal format '%s' for -g\n", optarg);
			if (PageWidth < 0 || PageWidth > 1000000)
			    error(1, "Illegal X value '%s' for -g\n", optarg);
			if (PageHeight < 0 || PageHeight > 1000000)
			    error(1, "Illegal Y value '%s' for -g\n", optarg);
			break;
	case 'm':	MediaCode = atoi(optarg); break;
	case 'n':	Copies = atoi(optarg); break;
	case 'p':	PaperCode = atoi(optarg); break;
	case 'r':	if (parse_xy(optarg, &ResX, &ResY))
			    error(1, "Illegal format '%s' for -r\n", optarg);
			break;
	case 's':	SourceCode = atoi(optarg); break;
	case 't':	SaveToner = 1; break;
	case 'u':
			if (strcmp(optarg, "0") == 0)
			    break;
			if (parse_xy(optarg, &UpperLeftX, &UpperLeftY))
			    error(1, "Illegal format '%s' for -u\n", optarg);
			break;
	case 'l':
			if (strcmp(optarg, "0") == 0)
			    break;
			if (parse_xy(optarg, &LowerRightX, &LowerRightY))
			    error(1, "Illegal format '%s' for -l\n", optarg);
			break;
	case 'L':	LogicalClip = atoi(optarg);
			if (LogicalClip < 0 || LogicalClip > 3)
			    error(1, "Illegal value '%s' for -L\n", optarg);
			break;
	case 'A':	AllIsBlack = !AllIsBlack; break;
	case 'B':	BlackClears = !BlackClears; break;
	case 'J':	if (optarg[0]) Filename = optarg; break;
	case 'O':
			rc = sscanf(optarg, "%d,%d,%d,%d",
			    &CMYK_Offset[0], &CMYK_Offset[1],
			    &CMYK_Offset[2], &CMYK_Offset[3]);
			if (rc != 4)
			    error(1, "Alignment error '%s' for -O c,m,y,k\n",
				optarg);
			break;
	case 'P':	OutputStartPlane = !OutputStartPlane; break;
	case 'U':	if (optarg[0]) Username = optarg; break;
	case 'X':	ExtraPad = atoi(optarg); break;
	case 'D':	Debug = atoi(optarg); break;
	case 'V':	printf("%s\n", Version); exit(0);
	default:	usage(); exit(1);
	}

    if (UpperLeftX < 0 || UpperLeftX >= PageWidth)
	error(1, "Illegal X value '%d' for -u\n", UpperLeftX);
    if (UpperLeftY < 0 || UpperLeftY >= PageHeight)
	error(1, "Illegal Y value '%d' for -u\n", UpperLeftY);
    if (LowerRightX < 0 || LowerRightX >= PageWidth)
	error(1, "Illegal X value '%d' for -l\n", LowerRightX);
    if (LowerRightY < 0 || LowerRightY >= PageHeight)
	error(1, "Illegal Y value '%d' for -l\n", LowerRightY);

    argc -= optind;
    argv += optind;

    if (getenv("DEVICE_URI"))
	IsCUPS = 1;

    start_doc(stdout);

    switch (Duplex)
    {
    case DMDUPLEX_LONGEDGE:
    case DMDUPLEX_SHORTEDGE:
    case DMDUPLEX_MANUALLONG:
    case DMDUPLEX_MANUALSHORT:
	EvenPages = tmpfile();
	break;
    }

    if (argc == 0)
    {
	do_one(stdin);
    }
    else
    {
	for (i = 0; i < argc; ++i)
	{
	    FILE	*ifp;

	    ifp = fopen(argv[i], "r");
	    if (!ifp)
		error(1, "Can't open '%s' for reading\n", argv[i]);
	    do_one(ifp);
	    fclose(ifp);
	}
    }

    /*
     *	Do manual duplex
     */
    if (EvenPages)
    {
	// Handle odd page count
	if ( (PageNum & 1) == 1)
	{
	    SeekRec[SeekIndex].b = ftell(EvenPages);
	    blank_page(EvenPages);
	    SeekRec[SeekIndex].e = ftell(EvenPages);
	    debug(1, "Blank Page: %d	%ld	%ld\n",
		PageNum, SeekRec[SeekIndex].b, SeekRec[SeekIndex].e);
	    SeekIndex++;
	}

	// Manual Pause
	load_tray2(stdout);

	// Write even pages in reverse order
	for (i = SeekIndex-1; i >= 0; --i)
	{
	    debug(1, "EvenPage: %d	%ld	%ld\n",
		i, SeekRec[i].b, SeekRec[i].e);
	    fseek(EvenPages, SeekRec[i].b, 0L);
	    for (j = 0; j < (SeekRec[i].e - SeekRec[i].b); ++j)
	        putc(getc(EvenPages), stdout);
	}
	fclose(EvenPages);
    }

    end_doc(stdout);

    exit(0);
}
