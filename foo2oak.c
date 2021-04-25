/*

GENERAL
This program converts pbm (B/W) images, 2-bit pgm (grayscale), and
1-bit- or 2-bit-per-pixel cmyk images (all produced by ghostscript)
to Oak Technolgies JBIG format.

With this utility, you can print to some HP printers, such as these:
    - HP LaserJet 1500
    - Kyocera Mita KM-1635:   -z1 (rotate 90)
    - Kyocera Mita KM-2035:   -z1 (rotate 90)

BUGS AND DEFICIENCIES
    - Needs to do color correction
    - Needs to support a better input color file format which includes
      explicit page boundary indications.

EXAMPLES

AUTHORS
Rick Richardson.  It also uses Markus Kuhn's jbig-kit compression
library (included, but also available at
http://www.cl.cam.ac.uk/~mgk25/jbigkit/).

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


# ./usb_printerid /dev/usb/lp0
GET_DEVICE_ID string:
MFG:Hewlett-Packard;CMD:OAKRAS,DW-PCL;MDL:hp color LaserJet 
1500;CLS:PRINTER;DES:Hewlett-Packard color LaserJet 
1500;MEM:16MB;1284.4DL:4d,4e,1;MSZ:10000000;FDT:0;CAL:00020811213C568F02060C1825446BA800030914203F5B9901040C151E3E60AD;
Status: 0x18

*/

/*
 * TODO: Handle 2 bit mono and color output
 */

static char Version[] = "$Id: foo2oak.c,v 1.70 2018/05/04 18:16:50 rick Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include "jbig.h"
#include "oak.h"

/*
 * Command line options
 */
int	Debug = 0;
int	ZeroTime = 0;
int	ResX = 600;
int	ResY = 600;
int	Bpp = 1;
int	PaperCode = OAK_PAPER_LETTER;
int	PageWidth = 600 * 8.5;
int	PageHeight = 600 * 11;
int	UpperLeftX = 0;
int	UpperLeftY = 0;
int	LowerRightX = 0;
int	LowerRightY = 0;
int	Copies = 1;
int	Duplex = 1;
	    #define DUPLEX_NONE		1
	    #define DUPLEX_LONG_EDGE	2
	    #define DUPLEX_SHORT_EDGE	3

int	SourceCode = OAK_SOURCE_AUTO;
int	MediaCode = OAK_MEDIA_AUTO;
char	*Username = NULL;
char	*Filename = NULL;
int	Mode = 0;
		#define	MODE_MONO	1
		#define	MODE_COLOR	2
int	Model = 0;
		#define MODEL_HP1500    0
		#define MODEL_KM1635    1
		#define MODEL_LAST	1

int	Color2Mono = 0;
int	BlackClears = 0;
int	AllIsBlack = 0;

int	LogicalOffsetX = 0;
int	LogicalOffsetY = 0;

#define LOGICAL_CLIP_X	2
#define LOGICAL_CLIP_Y	1
int	LogicalClip = LOGICAL_CLIP_X | LOGICAL_CLIP_Y;

int	IsCUPS = 0;
int	Mirror = 1;

/*
 * I now believe this is a YMCK printer as far as plane output ordering goes.
 */
#define	PL_C	2
#define	PL_M	1
#define	PL_Y	0
#define	PL_K	3

void
usage(void)
{
    fprintf(stderr,
"Usage:\n"
"   foo2oak [options] <pbmraw-file >OAKT-file\n"
"\n"
"	Convert Ghostscript pbm format to a 1-bpp monochrome OAKT stream,\n"
"	for driving the HP LaserJet 1500 color laser printer\n"
"	and other OAKT-based black and white printers.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"		-sPAPERSIZE=letter -r600x600 -sDEVICE=pbmraw \\ \n"
"		-sOutputFile=- - < testpage.ps \\ \n"
"	| foo2oak -r600x600 -g5100x6600 -p1 >testpage.oak\n"
"\n"
"   foo2oak [options] <pgmraw-file >OAKT-file\n"
"\n"
"	Convert Ghostscript pgm format to a 2-bpp monochrome OAKT stream,\n"
"	for driving the HP LaserJet 1500 color laser printer\n"
"	and other OAKT-based black and white printers.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"		-sPAPERSIZE=letter -r600x600 -sDEVICE=pgmraw \\ \n"
"		-sOutputFile=- - < testpage.ps \\ \n"
"	| foo2oak -r600x600 -g5100x6600 -p1 >testpage.oak\n"
"\n"
"   foo2oak [options] <bitcmyk-file >OAKT-file\n"
"\n"
"	Convert Ghostscript bitcmyk format to a 1-bpp color OAKT stream,\n"
"	for driving the HP LaserJet 1500 color laser printer.\n"
"	N.B. Color correction is expected to be performed by ghostscript.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"	    -sPAPERSIZE=letter -g5100x6600 -r600x600 -sDEVICE=bitcmyk \\ \n"
"	    -sOutputFile=- - < testpage.ps \\ \n"
"	| foo2oak -r600x600 -g5100x6600 -p1 >testpage.oak\n"
"\n"
"Normal Options:\n"
"-b bits           Bits per plane if autodetect doesn't work (1 or 2) [%d]\n"
"-c                Force color mode if autodetect doesn't work\n"
"-d duplex         Duplex code to send to printer [%d]\n"
"                    1=off, 2=longedge, 3=shortedge\n"
"-g <xpix>x<ypix>  Set page dimensions in pixels [%dx%d]\n"
"-m media          Media code to send to printer [%d]\n"
"                    0=auto 1=plain 2=preprinted 3=letterhead 4=transparency\n"
"                    5=prepunched 6=labels 7=bond 8=recycled 9=color\n"
"                    10=cardstock 11=heavy 12=envelope 13=light 14=tough\n"
"                    15=vellum 16=rough 19=thick 20=highqual\n"
"-p paper          Paper code to send to printer [%d]\n"
"                   1=letter, 3=ledger, 5=legal, 6=statement, 7=executive,\n"
"		    8=A3, 9=A4, 11=A5, 12=B4, 13=B5jis, 14=folio, 19=env9,\n"
"		    20=env10, 27=envDL, 28=envC5, 30=envC4, 37=envMonarch,\n"
"		    257=A6, 258=B6, 259=B5iso, 260=env6\n"
"-n copies         Number of copies [%d]\n"
"-r <xres>x<yres>  Set device resolution in pixels/inch [%dx%d]\n"
"-s source         Source code to send to printer [%d]\n"
"                    1=tray1 2=tray2 4=manual 7=auto\n"
"                    Code numbers may vary with printer model\n"
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
"-M mirror         Mirror bytes (0=KM-1635/KM-2035, 1=HP CLJ 1500) [%d]\n"
"-z model          Model [%d]\n"
"                    0=HP-1500, 1=KM-1635/2035\n"
"\n"
"Debugging Options:\n"
"-S plane          Output just a single color plane from a color print [all]\n"
"                  %d=Cyan, %d=Magenta, %d=Yellow, %d=Black\n"
"-D lvl            Set Debug level [%d]\n"
"-V                Version %s\n"
    , Duplex
    , Bpp
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
    , Mirror
    , Model
    , PL_C, PL_M, PL_Y, PL_K
    , Debug
    , Version
    );

  exit(1);
}

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

/*
 * bit mirroring arrays
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

void
mirror_bytes(unsigned char *sp, int bpl, unsigned char *mirror)
{
    unsigned char *ep = sp + bpl - 1;
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

/*
 * This creates a linked list of compressed data.  The first item
 * in the list is the BIH and is always 20 bytes in size.  Each following
 * item is 65536 bytes in length.  The last item length is whatever remains.
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

void
output_jbig(unsigned char *start, size_t len, void *cbarg)
{
    BIE_CHAIN	*current, **root = (BIE_CHAIN **) cbarg;
    int		size = 65536;	// Printer does strange things otherwise.

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
oak_record(FILE *fp, int type, void *payload, int paylen)
{
    OAK_HDR	hdr;
    static char	pad[] = "PAD_PAD_PAD_PAD_";
    static int	pageno = 0;
    int		rc;

    memcpy(hdr.magic, OAK_HDR_MAGIC, sizeof(hdr.magic));
    hdr.type = type;
    hdr.len = (sizeof(hdr) + paylen + 15) & ~0x0f;

    rc = fwrite(&hdr, 1, sizeof(hdr), fp);
    if (rc == 0) error(1, "fwrite(1): rc == 0!\n");
    if (payload && paylen)
    {
	rc = fwrite(payload, 1, paylen, fp);
	if (rc == 0) error(1, "fwrite(2): rc == 0!\n");
    }
    if (hdr.len - (sizeof(hdr) + paylen))
    {
	rc = fwrite(pad, 1, hdr.len - (sizeof(hdr) + paylen), fp);
	if (rc == 0) error(1, "fwrite(3): rc == 0!\n");
    }

    if (type == OAK_TYPE_START_PAGE)
    {
	++pageno;
	if (IsCUPS)
	    fprintf(stderr, "PAGE: %d %d\n", pageno, Copies);
    }

    return 0;
}

void
start_doc(FILE *fp)
{
    OAK_OTHER		recother;
    OAK_TIME		rectime;
    OAK_FILENAME	recfile;
    OAK_DUPLEX		recduplex;
    OAK_DRIVER		recdriver;
    time_t		now;
    struct tm		*tm;

    if (Model == MODEL_KM1635)
    {
	memset(&recdriver, 0, sizeof(recdriver));
	strncpy(recdriver.string, Version+5, sizeof(recdriver.string) - 1);
	recdriver.string[sizeof(recdriver.string) - 1] = 0;
	oak_record(fp, OAK_TYPE_DRIVER, &recdriver, sizeof(recdriver));
    }
    memset(&recother, 0, sizeof(recother));
    recother.unk = 1;			// TODO
    strcpy(recother.string, "OTHER");	// TODO: Username????
    oak_record(fp, OAK_TYPE_OTHER, &recother, sizeof(recother));

    memset(&rectime, 0, sizeof(rectime));
    time(&now);
    if (ZeroTime)
	now = 0;
    strcpy(rectime.datetime, ctime(&now));
    rectime.time_t = now;
    tm = localtime(&now);
    rectime.year = tm->tm_year + 1900;
    rectime.tm_mon = tm->tm_mon;
    rectime.tm_mday = tm->tm_mday;
    rectime.tm_hour = tm->tm_hour;
    rectime.tm_min = tm->tm_min;
    rectime.tm_sec = tm->tm_sec;
    oak_record(fp, OAK_TYPE_TIME, &rectime, sizeof(rectime));

    memset(&recfile, 0, sizeof(recfile));
    strcpy(recfile.string, Filename ? Filename : "stdin");
    oak_record(fp, OAK_TYPE_FILENAME, &recfile, sizeof(recfile));

    memset(&recduplex, 0, sizeof(recduplex));
    recduplex.duplex = (Duplex > DUPLEX_NONE) ? 1 : 0;
    recduplex.short_edge = (Duplex == DUPLEX_SHORT_EDGE) ? 1 : 0;
    oak_record(fp, OAK_TYPE_DUPLEX, &recduplex, sizeof(recduplex));
}

void
end_doc(FILE *fp)
{
    oak_record(fp, OAK_TYPE_END_DOC, NULL, 0);
}

void
cmyk_planes(unsigned char *plane[4], unsigned char *raw, int w, int h)
{
    int			rawbpl = (w + 1) / 2;
    int			bpl = (w + 7) / 8;
    int			i;
    int			x, y;
    unsigned char	byte;
    unsigned char	mask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    int			aib = AllIsBlack;
    int			bc = BlackClears;

    for (i = 0; i < 4; ++i)
	memset(plane[i], 0, bpl * h);

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
		plane[PL_K][y*bpl + x/8] |= mask[x&7];
	    }
	    else if (byte & 0x10)
	    {
		plane[PL_K][y*bpl + x/8] |= mask[x&7];
		if (!bc)
		{
		    if (byte & 0x80) plane[PL_C][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x40) plane[PL_M][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x20) plane[PL_Y][y*bpl + x/8] |= mask[x&7];
		}
	    }
	    else
	    {
		if (byte & 0x80) plane[PL_C][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x40) plane[PL_M][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x20) plane[PL_Y][y*bpl + x/8] |= mask[x&7];
	    }

	    ++x;
	    if (aib && (byte & 0x0E) == 0x0E)
	    {
		plane[PL_K][y*bpl + x/8] |= mask[x&7];
	    }
	    else if (byte & 0x1)
	    {
		plane[PL_K][y*bpl + x/8] |= mask[x&7];
		if (!bc)
		{
		    if (byte & 0x8) plane[PL_C][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x4) plane[PL_M][y*bpl + x/8] |= mask[x&7];
		    if (byte & 0x2) plane[PL_Y][y*bpl + x/8] |= mask[x&7];
		}
	    }
	    else
	    {
		if (byte & 0x8) plane[PL_C][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x4) plane[PL_M][y*bpl + x/8] |= mask[x&7];
		if (byte & 0x2) plane[PL_Y][y*bpl + x/8] |= mask[x&7];
	    }
	}
    }
}

void
pgm_subplanes(unsigned char *subplane[2], unsigned char *raw, int w, int h)
{
    int			bpl = (w + 7) / 8;
    int			x, y;
    unsigned char	byte;
    unsigned char	ormask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    unsigned char	andmask[8] = { 0x7f, ~64, ~32, ~16, ~8, ~4, ~2, ~1 };
    double		*carry0, *carry1, *carrytmp;
    double		sum;
    static int		dir = 1;
    int			xs, xe;
static int cnt[4];
int tot = 0;

    // TODO: convert this to scaled integer arithmetic

    carry0 = (double *) calloc((w+2), sizeof(*carry0));
    if (!carry0)
	error(1, "Could not allocate space for carries\n");
    carry1 = (double *) calloc((w+2), sizeof(*carry0));
    if (!carry1)
	error(1, "Could not allocate space for carries\n");
    ++carry0; ++carry1;
    for (y = 0; y < h; ++y)
    {
	if (dir > 0)
	    { dir = -1; xs = w-1; xe = -1; }
	else
	    { dir = 1; xs = 0; xe = w; }
	for (x = xs; x != xe; x += dir)
	{
	    byte = 255 - raw[y*w + x];
	    sum = byte + carry0[x];

++tot;
	    if (byte == 255 || sum >= 255)
	    {	// Full black
++cnt[2];
		byte = 255;
		subplane[1][y*bpl + x/8] |= ormask[x&7];
		subplane[0][y*bpl + x/8] &= andmask[x&7];
	    }
	    else if (sum >= (70.0/100)*255)
	    {	// Dark gray
++cnt[3];
		byte = 255;
		subplane[1][y*bpl + x/8] |= ormask[x&7];
		subplane[0][y*bpl + x/8] |= ormask[x&7];
	    }
	    else if (sum > 0)
	    {	// Light gray
++cnt[1];
		byte = (40.0/100)*255;
		subplane[1][y*bpl + x/8] &= andmask[x&7];
		subplane[0][y*bpl + x/8] |= ormask[x&7];
	    }
	    else
	    {	// Full white
++cnt[0];
		byte = 0;
		subplane[1][y*bpl + x/8] &= andmask[x&7];
		subplane[0][y*bpl + x/8] &= andmask[x&7];
	    }

	    // Compute the carry that must be distributed
	    sum -= byte;
	    carry0[x+dir] += (sum*7)/16.0;
	    carry1[x-dir] += (sum*3)/16.0;
	    carry1[x    ] += (sum*5)/16.0;
	    carry1[x+dir] += (sum*1)/16.0;
	}
	// Advance carry buffers
	carrytmp = carry0; carry0 = carry1; carry1 = carrytmp;
	memset(carry1-1, 0, (w+2) * sizeof(*carry1));
    }
debug(1, "%d	%d %d %d %d\n", tot, cnt[3], cnt[2], cnt[1], cnt[0]);

    free(--carry0); free(--carry1);
}

void
cups_planes(unsigned char *plane[4][2], unsigned char *raw, int w, int h)
{
    int			bpl = (w + 7) / 8;
    int			i, sub;
    int			x, y;
    unsigned char	byte;
    unsigned char	mask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    int			aib = AllIsBlack;
    int			bc = BlackClears;

    // Whitewash all planes
    for (i = 0; i < 4; ++i)
	for (sub = 0; sub < 2; ++sub)
	    memset(plane[i][sub], 0, bpl * h);

    //
    // Unpack the combined plane into individual color planes
    //
    // TODO: this can be speeded up using a 256 or 65536 entry lookup table
    //
    for (y = 0; y < h; ++y)
    {
	for (x = 0; x < w; ++x)
	{
	    byte = raw[y*w + x];

	    if (aib && (byte & 0xfc) == 0xfc)
	    {
		plane[PL_K][1][y*bpl + x/8] |= mask[x&7];
		plane[PL_K][0][y*bpl + x/8] |= mask[x&7];
	    }
	    else if (bc && (byte & 0x03) == 0x03)
	    {
		plane[PL_K][1][y*bpl + x/8] |= mask[x&7];
		plane[PL_K][0][y*bpl + x/8] |= mask[x&7];
	    }
	    else
	    {
		if (byte & 128) plane[PL_C][1][y*bpl + x/8] |= mask[x&7];
		if (byte & 64)  plane[PL_C][0][y*bpl + x/8] |= mask[x&7];
		if (byte & 32)  plane[PL_M][1][y*bpl + x/8] |= mask[x&7];
		if (byte & 16)  plane[PL_M][0][y*bpl + x/8] |= mask[x&7];
		if (byte & 8)   plane[PL_Y][1][y*bpl + x/8] |= mask[x&7];
		if (byte & 4)   plane[PL_Y][0][y*bpl + x/8] |= mask[x&7];
		if (byte & 2)   plane[PL_K][1][y*bpl + x/8] |= mask[x&7];
		if (byte & 1)   plane[PL_K][0][y*bpl + x/8] |= mask[x&7];
	    }
	}
    }
}

long JbgOptions[5] =
{
    /* Order */
    JBG_ILEAVE | JBG_SMID,
    /* Options */
    JBG_DELAY_AT | JBG_LRLTWO | JBG_TPDON | JBG_TPBON,
    /* L0 */
    128,
    /* MX */
    16,
    /* MY */
    0
};

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

static void
fill_image_plane_unknown(OAK_IMAGE_PLANE *ip)
{
    int	i;

    for (i = 0; i < 16; ++i)
	ip->unk[i] = i;
}

int
cmyk_page(unsigned char *raw, int w, int h, FILE *ofp)
{
    WORD		endpage_arg;
    DWORD		source_arg;
    OAK_MEDIA		recmedia;
    OAK_COPIES		reccopies;
    OAK_PAPER		recpaper;
    OAK_IMAGE_MONO	recmono;
    OAK_IMAGE_COLOR	reccolor;

    int			p, p0, p3;
    int			y;
    int			bpl = (w+7)/8;
    unsigned char	*plane[4];

    debug(1, "cmyk_page: w=%d, h=%d, bitbpl=%d\n", w, h, bpl);

    if (Color2Mono)
	p0 = p3 = Color2Mono - 1;
    else
	{ p0 = 0; p3 = 3; }
    for (p = p0; p <= p3 ; ++p)
    {
	plane[p] = malloc(bpl * h);
	if (!plane[p]) error(3, "Cannot allocate space for bit plane\n");
    }

    cmyk_planes(plane, raw, w, h);

    oak_record(ofp, OAK_TYPE_START_PAGE, NULL, 0);

    // TODO: page parms
    source_arg = SourceCode;
    oak_record(ofp, OAK_TYPE_SOURCE, &source_arg, sizeof(source_arg));

    recmedia.media = MediaCode;
    recmedia.unk8[0] = 2;					// TODO
    recmedia.unk8[1] = 0;					// TODO
    recmedia.unk8[2] = 0;					// TODO
    memset(recmedia.string, ' ', sizeof(recmedia.string));
    strcpy(recmedia.string, "");				// TODO
    oak_record(ofp, OAK_TYPE_MEDIA, &recmedia, sizeof(recmedia));

    reccopies.copies = Copies;
    reccopies.duplex = Duplex - 1;
    oak_record(ofp, OAK_TYPE_COPIES, &reccopies, sizeof(reccopies));

    recpaper.paper = PaperCode;
    if (Model == MODEL_KM1635)
    {
	recpaper.w1200 = PageWidth * 600 / ResX;
	recpaper.h1200 = PageHeight * 600 / ResY;
	switch (PaperCode)
	{
	case 1: case 9: case 13:	recpaper.unk = 1; break;
	default:			recpaper.unk = 0; break;
	}
    }
    else
    {
	recpaper.w1200 = PageWidth * 1200 / ResX;
	recpaper.h1200 = PageHeight * 1200 / ResY;
	recpaper.unk = 0;						// TODO
    }
    oak_record(ofp, OAK_TYPE_PAPER, &recpaper, sizeof(recpaper));

    // image header (32/33)
    if (p0 == p3)
    {
	recmono.plane.unk0 = 0;
	recmono.plane.unk1 = 0;
	recmono.plane.w = w;				// TODO
	recmono.plane.h = h;				// TODO
	recmono.plane.resx = ResX;
	recmono.plane.resy = ResY;
	recmono.plane.nbits = 1;
	fill_image_plane_unknown(&recmono.plane);
	oak_record(ofp, OAK_TYPE_IMAGE_MONO, &recmono, sizeof(recmono));
    }
    else
    {
	for (p = p0; p <= p3; ++p)
	{
	    reccolor.plane[p].unk0 = 0;
	    reccolor.plane[p].unk1 = 0;
	    reccolor.plane[p].w = w;			// TODO
	    reccolor.plane[p].h = h;			// TODO
	    reccolor.plane[p].resx = ResX;
	    reccolor.plane[p].resy = ResY;
	    reccolor.plane[p].nbits = 1;
	    fill_image_plane_unknown(&reccolor.plane[p]);
	}
	oak_record(ofp, OAK_TYPE_IMAGE_COLOR, &reccolor, sizeof(reccolor));
    }

    oak_record(ofp, OAK_TYPE_START_IMAGE, NULL, 0);

    //
    // Mirror the bits in the image
    //
    // TODO: combine this operation with cmyk_planes
    //
    if (Mirror)
	for (p = p0; p <= p3; ++p)
	    for (y = 0; y < h; ++y)
		mirror_bytes(plane[p] + y*bpl, bpl, Mirror1);

    //
    // Output the image stripes
    //
    #define N 256
    for (y = 0; y < h; y += N)
    {
	struct jbg_enc_state	se;
	unsigned char		*bitmaps[1];
	BIE_CHAIN		*chain;
	BIE_CHAIN		*current;
	OAK_IMAGE_DATA		recdata;
	int			chainlen;
	int			padlen;
	static char		pad[] = "PAD_PAD_PAD_PAD_";
	int			rc;

	int	lines = (h-y) > N ? N : (h-y);

	for (p = p0; p <= p3; ++p)
	{
	    bitmaps[0] = plane[p] + y * ((w+7)/8);
	    chain = NULL;

	    memset(&recdata.bih, 0, sizeof(recdata.bih));
	    recdata.datalen = 0;
	    recdata.padlen = 0;
	    recdata.unk1C = 0;					// TODO
	    recdata.y = y;
	    recdata.plane = p;
	    recdata.subplane = 0;

	    if (lines < N)
		JbgOptions[2] = lines;
	    else
		JbgOptions[2] = N;
	    jbg_enc_init(&se, w, lines, 1, bitmaps, output_jbig, &chain);
	    jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
				JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	    jbg_enc_out(&se);
	    jbg_enc_free(&se);

	    if (chain->len != 20)
		error(1, "Program error: missing BIH at start of chain\n");
	    chainlen = 0;
	    for (current = chain->next; current; current = current->next)
		chainlen += current->len;

	    // Copy in the BIH
	    memcpy(&recdata.bih, chain->data, sizeof(recdata.bih));

	    // Oak is little-endian, but JBIG-kit is big-endian
	    iswap32(&recdata.bih.xd);
	    iswap32(&recdata.bih.yd);
	    iswap32(&recdata.bih.l0);

	    recdata.datalen = chainlen;
	    recdata.padlen = (recdata.datalen + 15) & ~0x0f;
	    oak_record(ofp, OAK_TYPE_IMAGE_DATA, &recdata, sizeof(recdata));
	    for (current = chain->next; current; current = current->next)
	    {
		rc = fwrite(current->data, 1, current->len, ofp);
		if (rc == 0) error(1, "fwrite(4): rc == 0!\n");
	    }
	    padlen = recdata.padlen - recdata.datalen;  
	    if (padlen)
	    {
		rc = fwrite(pad, 1, padlen, ofp);
		if (rc == 0)
		    error(1, "fwrite(5): padlen=%d rc == 0!\n", padlen);
	    }

	    free_chain(chain);
	}
    }

    for (p = p0; p <= p3; ++p)
	free(plane[p]);
 
    oak_record(ofp, OAK_TYPE_END_IMAGE, NULL, 0);

    endpage_arg = 1;	// Color
    oak_record(ofp, OAK_TYPE_END_PAGE, &endpage_arg, sizeof(endpage_arg));

    return 0;
}

int
pbm_page(unsigned char *buf, int w, int h, FILE *ofp)
{
    WORD		endpage_arg;
    DWORD		source_arg;
    OAK_MEDIA		recmedia;
    OAK_COPIES		reccopies;
    OAK_PAPER		recpaper;
    OAK_IMAGE_MONO	recmono;

    int			y;
    int			bpl = (w+7)/8;

    oak_record(ofp, OAK_TYPE_START_PAGE, NULL, 0);

    // TODO: page parms
    source_arg = SourceCode;
    oak_record(ofp, OAK_TYPE_SOURCE, &source_arg, sizeof(source_arg));

    recmedia.media = MediaCode;
    recmedia.unk8[0] = 2;					// TODO
    recmedia.unk8[1] = 0;					// TODO
    recmedia.unk8[2] = 0;					// TODO
    memset(recmedia.string, ' ', sizeof(recmedia.string));
    strcpy(recmedia.string, "");				// TODO
    oak_record(ofp, OAK_TYPE_MEDIA, &recmedia, sizeof(recmedia));

    reccopies.copies = Copies;
    reccopies.duplex = Duplex - 1;
    oak_record(ofp, OAK_TYPE_COPIES, &reccopies, sizeof(reccopies));

    recpaper.paper = PaperCode;
    if (Model == MODEL_KM1635)
    {
	recpaper.w1200 = PageWidth * 600 / ResX;
	recpaper.h1200 = PageHeight * 600 / ResY;
	switch (PaperCode)
	{
	case 1: case 9: case 13:	recpaper.unk = 1; break;
	default:			recpaper.unk = 0; break;
	}
    }
    else
    {
	recpaper.w1200 = PageWidth * 1200 / ResX;
	recpaper.h1200 = PageHeight * 1200 / ResY;
	recpaper.unk = 0;						// TODO
    }
    oak_record(ofp, OAK_TYPE_PAPER, &recpaper, sizeof(recpaper));

    // image header (32/33)
    recmono.plane.unk0 = 0;
    recmono.plane.unk1 = 1;
    recmono.plane.w = w;					// TODO
    recmono.plane.h = h;					// TODO
    recmono.plane.resx = ResX;
    recmono.plane.resy = ResY;
    recmono.plane.nbits = 1;
    fill_image_plane_unknown(&recmono.plane);
    oak_record(ofp, OAK_TYPE_IMAGE_MONO, &recmono, sizeof(recmono));

    oak_record(ofp, OAK_TYPE_START_IMAGE, NULL, 0);

    //
    // Mirror the bits in the image
    //
    if (Mirror)
	for (y = 0; y < h; ++y)
	    mirror_bytes(buf + y*bpl, bpl, Mirror1);

    //
    // Output the image stripes
    //
    #define N 256
    for (y = 0; y < h; y += N)
    {
	struct jbg_enc_state	se;
	unsigned char		*bitmaps[1];
	BIE_CHAIN		*chain;
	BIE_CHAIN		*current;
	OAK_IMAGE_DATA		recdata;
	int			chainlen;
	int			padlen;
	static char		pad[] = "PAD_PAD_PAD_PAD_";
	int			rc;

	int	lines = (h-y) > N ? N : (h-y);

	bitmaps[0] = buf + y * ((w+7)/8);
	chain = NULL;

	memset(&recdata.bih, 0, sizeof(recdata.bih));
	recdata.datalen = 0;
	recdata.padlen = 0;
	recdata.unk1C = 0;					// TODO
	recdata.y = y;
	if (Model == MODEL_KM1635)
	    recdata.plane = 0; //K
	else
	    recdata.plane = 3; //K
	recdata.subplane = 0;

	if (lines < N)
	    JbgOptions[2] = lines;
	else
	    JbgOptions[2] = N;
	jbg_enc_init(&se, w, lines, 1, bitmaps, output_jbig, &chain);
	jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
			    JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	jbg_enc_out(&se);
	jbg_enc_free(&se);

	if (chain->len != 20)
	    error(1, "Program error: missing BIH at start of chain\n");
	chainlen = 0;
	for (current = chain->next; current; current = current->next)
	    chainlen += current->len;

	// Copy in the BIH
	memcpy(&recdata.bih, chain->data, sizeof(recdata.bih));

	// Oak is little-endian, but JBIG-kit is big-endian
	iswap32(&recdata.bih.xd);
	iswap32(&recdata.bih.yd);
	iswap32(&recdata.bih.l0);

	recdata.datalen = chainlen;
	recdata.padlen = (recdata.datalen + 15) & ~0x0f;
	oak_record(ofp, OAK_TYPE_IMAGE_DATA, &recdata, sizeof(recdata));
	for (current = chain->next; current; current = current->next)
	{
	    rc = fwrite(current->data, 1, current->len, ofp);
	    if (rc == 0) error(1, "fwrite(7): rc == 0!\n");
	}
	padlen = recdata.padlen - recdata.datalen;  
	if (padlen)
	{
	    rc = fwrite(pad, 1, padlen, ofp);
	    if (rc == 0) error(1, "fwrite(8): rc == 0!\n");
	}
	free_chain(chain);
    }
 
    oak_record(ofp, OAK_TYPE_END_IMAGE, NULL, 0);

    endpage_arg = 0;	// Mono
    oak_record(ofp, OAK_TYPE_END_PAGE, &endpage_arg, sizeof(endpage_arg));

    return 0;
}

int
pgm_page(unsigned char *raw, int w, int h, FILE *ofp)
{
    WORD		endpage_arg;
    DWORD		source_arg;
    OAK_MEDIA		recmedia;
    OAK_COPIES		reccopies;
    OAK_PAPER		recpaper;
    OAK_IMAGE_MONO	recmono;

    int			sub;
    int			y;
    int			bpl = (w+7)/8;
    unsigned char	*subplane[2];

    for (sub = 0; sub < 2 ; ++sub)
    {
	subplane[sub] = malloc(bpl * h);
	if (!subplane[sub]) error(3, "Cannot allocate space for subplane\n");
    }

    pgm_subplanes(subplane, raw, w, h);

    oak_record(ofp, OAK_TYPE_START_PAGE, NULL, 0);

    // TODO: page parms
    source_arg = SourceCode;
    oak_record(ofp, OAK_TYPE_SOURCE, &source_arg, sizeof(source_arg));

    recmedia.media = MediaCode;
    recmedia.unk8[0] = 2;					// TODO
    recmedia.unk8[1] = 0;					// TODO
    recmedia.unk8[2] = 0;					// TODO
    memset(recmedia.string, ' ', sizeof(recmedia.string));
    strcpy(recmedia.string, "");				// TODO
    oak_record(ofp, OAK_TYPE_MEDIA, &recmedia, sizeof(recmedia));

    reccopies.copies = Copies;
    reccopies.duplex = Duplex - 1;
    oak_record(ofp, OAK_TYPE_COPIES, &reccopies, sizeof(reccopies));

    recpaper.paper = PaperCode;
    if (Model == MODEL_KM1635)
    {
	recpaper.w1200 = PageWidth * 600 / ResX;
	recpaper.h1200 = PageHeight * 600 / ResY;
	switch (PaperCode)
	{
	case 1: case 9: case 13:	recpaper.unk = 1; break;
	default:			recpaper.unk = 0; break;
	}
    }
    else
    {
	recpaper.w1200 = PageWidth * 1200 / ResX;
	recpaper.h1200 = PageHeight * 1200 / ResY;
	recpaper.unk = 0;						// TODO
    }
    oak_record(ofp, OAK_TYPE_PAPER, &recpaper, sizeof(recpaper));

    // image header (32/33)
    recmono.plane.unk0 = 0;
    recmono.plane.unk1 = 0;
    recmono.plane.w = w;
    recmono.plane.h = h;
    recmono.plane.resx = ResX;
    recmono.plane.resy = ResY;
    recmono.plane.nbits = 2;
    fill_image_plane_unknown(&recmono.plane);
    oak_record(ofp, OAK_TYPE_IMAGE_MONO, &recmono, sizeof(recmono));

    oak_record(ofp, OAK_TYPE_START_IMAGE, NULL, 0);

    //
    // Mirror the bits in the image
    //
    // TODO: combine this operation with pgm_subplanes
    //
    if (Mirror)
	for (sub = 0; sub < 2; ++sub)
	    for (y = 0; y < h; ++y)
		mirror_bytes(subplane[sub] + y*bpl, bpl, Mirror1);

    //
    // Output the image stripes
    //
    #define N 256
    for (y = 0; y < h; y += N)
    {
	struct jbg_enc_state	se;
	unsigned char		*bitmaps[1];
	BIE_CHAIN		*chain;
	BIE_CHAIN		*current;
	OAK_IMAGE_DATA		recdata;
	int			chainlen;
	int			padlen;
	static char		pad[] = "PAD_PAD_PAD_PAD_";
	int			rc;

	int	lines = (h-y) > N ? N : (h-y);

	for (sub = 0; sub < 2; ++sub)
	{
	    bitmaps[0] = subplane[sub] + y * ((w+7)/8);
	    chain = NULL;

	    memset(&recdata.bih, 0, sizeof(recdata.bih));
	    recdata.datalen = 0;
	    recdata.padlen = 0;
	    recdata.unk1C = 0;					// TODO
	    recdata.y = y;
	    recdata.plane = 3; //K
	    recdata.subplane = sub;

	    if (lines < N)
		JbgOptions[2] = lines;
	    else
		JbgOptions[2] = N;
	    jbg_enc_init(&se, w, lines, 1, bitmaps, output_jbig, &chain);
	    jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
				JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	    jbg_enc_out(&se);
	    jbg_enc_free(&se);

	    if (chain->len != 20)
		error(1, "Program error: missing BIH at start of chain\n");
	    chainlen = 0;
	    for (current = chain->next; current; current = current->next)
		chainlen += current->len;

	    // Copy in the BIH
	    memcpy(&recdata.bih, chain->data, sizeof(recdata.bih));

	    // Oak is little-endian, but JBIG-kit is big-endian
	    iswap32(&recdata.bih.xd);
	    iswap32(&recdata.bih.yd);
	    iswap32(&recdata.bih.l0);

	    recdata.datalen = chainlen;
	    recdata.padlen = (recdata.datalen + 15) & ~0x0f;
	    oak_record(ofp, OAK_TYPE_IMAGE_DATA, &recdata, sizeof(recdata));
	    for (current = chain->next; current; current = current->next)
	    {
		rc = fwrite(current->data, 1, current->len, ofp);
		if (rc == 0) error(1, "fwrite(9): rc == 0!\n");
	    }
	    padlen = recdata.padlen - recdata.datalen;  
	    if (padlen)
	    {
		rc = fwrite(pad, 1, padlen, ofp);
		if (rc == 0) error(1, "fwrite(10): rc == 0!\n");
	    }

	    free_chain(chain);
	}
    }

    for (sub = 0; sub < 2; ++sub)
	free(subplane[sub]);
 
    oak_record(ofp, OAK_TYPE_END_IMAGE, NULL, 0);

    endpage_arg = 0;
    oak_record(ofp, OAK_TYPE_END_PAGE, &endpage_arg, sizeof(endpage_arg));

    return 0;
}

int
cups_page(unsigned char *raw, int w, int h, FILE *ofp)
{
    WORD		endpage_arg;
    DWORD		source_arg;
    OAK_MEDIA		recmedia;
    OAK_COPIES		reccopies;
    OAK_PAPER		recpaper;
    OAK_IMAGE_MONO	recmono;
    OAK_IMAGE_COLOR	reccolor;

    int			p, p0, p3;
    int			sub;
    int			y;
    int			bpl = (w+7)/8;
    unsigned char	*plane[4][2];

    if (Color2Mono)
	p0 = p3 = Color2Mono - 1;
    else
	{ p0 = 0; p3 = 3; }
    for (p = p0; p <= p3 ; ++p)
    {
	for (sub = 0; sub < 2 ; ++sub)
	{
	    plane[p][sub] = malloc(bpl * h);
	    if (!plane[p][sub])
		error(3, "Cannot allocate space for bit plane\n");
	}
    }

    cups_planes(plane, raw, w, h);

    oak_record(ofp, OAK_TYPE_START_PAGE, NULL, 0);

    // TODO: page parms
    source_arg = SourceCode;
    oak_record(ofp, OAK_TYPE_SOURCE, &source_arg, sizeof(source_arg));

    recmedia.media = MediaCode;
    recmedia.unk8[0] = 2;					// TODO
    recmedia.unk8[1] = 0;					// TODO
    recmedia.unk8[2] = 0;					// TODO
    memset(recmedia.string, ' ', sizeof(recmedia.string));
    strcpy(recmedia.string, "");				// TODO
    oak_record(ofp, OAK_TYPE_MEDIA, &recmedia, sizeof(recmedia));

    reccopies.copies = Copies;
    reccopies.duplex = Duplex - 1;
    oak_record(ofp, OAK_TYPE_COPIES, &reccopies, sizeof(reccopies));

    recpaper.paper = PaperCode;
    if (Model == MODEL_KM1635)
    {
	recpaper.w1200 = PageWidth * 600 / ResX;
	recpaper.h1200 = PageHeight * 600 / ResY;
	switch (PaperCode)
	{
	case 1: case 9: case 13:	recpaper.unk = 1; break;
	default:			recpaper.unk = 0; break;
	}
    }
    else
    {
	recpaper.w1200 = PageWidth * 1200 / ResX;
	recpaper.h1200 = PageHeight * 1200 / ResY;
	recpaper.unk = 0;						// TODO
    }
    oak_record(ofp, OAK_TYPE_PAPER, &recpaper, sizeof(recpaper));

    // image header (32/33)
    if (p0 == p3)
    {
	recmono.plane.unk0 = 0;
	recmono.plane.unk1 = 0;
	recmono.plane.w = w;				// TODO
	recmono.plane.h = h;				// TODO
	recmono.plane.resx = ResX;
	recmono.plane.resy = ResY;
	recmono.plane.nbits = 2;
	fill_image_plane_unknown(&recmono.plane);
	oak_record(ofp, OAK_TYPE_IMAGE_MONO, &recmono, sizeof(recmono));
    }
    else
    {
	for (p = p0; p <= p3; ++p)
	{
	    reccolor.plane[p].unk0 = 0;
	    reccolor.plane[p].unk1 = 0;
	    reccolor.plane[p].w = w;			// TODO
	    reccolor.plane[p].h = h;			// TODO
	    reccolor.plane[p].resx = ResX;
	    reccolor.plane[p].resy = ResY;
	    reccolor.plane[p].nbits = 2;
	    fill_image_plane_unknown(&reccolor.plane[p]);
	}
	oak_record(ofp, OAK_TYPE_IMAGE_COLOR, &reccolor, sizeof(reccolor));
    }

    oak_record(ofp, OAK_TYPE_START_IMAGE, NULL, 0);

    //
    // Mirror the bits in the image
    //
    // TODO: combine this operation with cups_planes
    //
    if (Mirror)
	for (p = p0; p <= p3; ++p)
	    for (sub = 0; sub < 2; ++sub)
		for (y = 0; y < h; ++y)
		    mirror_bytes(plane[p][sub] + y*bpl, bpl, Mirror1);

    //
    // Output the image stripes
    //
    #define N 256
    for (y = 0; y < h; y += N)
    {
	struct jbg_enc_state	se;
	unsigned char		*bitmaps[1];
	BIE_CHAIN		*chain;
	BIE_CHAIN		*current;
	OAK_IMAGE_DATA		recdata;
	int			chainlen;
	int			padlen;
	static char		pad[] = "PAD_PAD_PAD_PAD_";
	int			rc;

	int	lines = (h-y) > N ? N : (h-y);

	for (p = p0; p <= p3; ++p)
	{
	    for (sub = 0; sub < 2; ++sub)
	    {
		bitmaps[0] = plane[p][sub] + y * ((w+7)/8);
		chain = NULL;

		memset(&recdata.bih, 0, sizeof(recdata.bih));
		recdata.datalen = 0;
		recdata.padlen = 0;
		recdata.unk1C = 0;				// TODO
		recdata.y = y;
		recdata.plane = p;
		recdata.subplane = sub;

		if (lines < N)
		    JbgOptions[2] = lines;
		else
		    JbgOptions[2] = N;
		jbg_enc_init(&se, w, lines, 1, bitmaps, output_jbig, &chain);
		jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
				JbgOptions[2], JbgOptions[3], JbgOptions[4]);
		jbg_enc_out(&se);
		jbg_enc_free(&se);

		if (chain->len != 20)
		    error(1, "Program error: missing BIH at start of chain\n");
		chainlen = 0;
		for (current = chain->next; current; current = current->next)
		    chainlen += current->len;

		// Copy in the BIH
		memcpy(&recdata.bih, chain->data, sizeof(recdata.bih));

		// Oak is little-endian, but JBIG-kit is big-endian
		iswap32(&recdata.bih.xd);
		iswap32(&recdata.bih.yd);
		iswap32(&recdata.bih.l0);

		recdata.datalen = chainlen;
		recdata.padlen = (recdata.datalen + 15) & ~0x0f;
		oak_record(ofp, OAK_TYPE_IMAGE_DATA, &recdata, sizeof(recdata));
		for (current = chain->next; current; current = current->next)
		{
		    rc = fwrite(current->data, 1, current->len, ofp);
		    if (rc == 0) error(1, "fwrite(11): rc == 0!\n");
		}
		padlen = recdata.padlen - recdata.datalen;  
		rc = fwrite(pad, 1, padlen, ofp);
		if (rc == 0) error(1, "fwrite(12): rc == 0!\n");

		free_chain(chain);
	    }
	}
    }

    for (p = p0; p <= p3; ++p)
	for (sub = 0; sub < 2; ++sub)
	    free(plane[p][sub]);
 
    oak_record(ofp, OAK_TYPE_END_IMAGE, NULL, 0);

    endpage_arg = 0;
    oak_record(ofp, OAK_TYPE_END_PAGE, &endpage_arg, sizeof(endpage_arg));

    return 0;
}

int
read_and_clip_image(unsigned char *buf,
			int rawBpl, int rightBpl, int pixelsPerByte,
			int bpl, int h, FILE *ifp)
{
    unsigned char	*rowbuf, *rowp;
    int			y;
    int			rc;

    debug(1, "read_and_clip_image: rawBpl=%d, rightBpl=%d, pixelsePerByte=%d\n",
		rawBpl, rightBpl, pixelsPerByte);
    debug(1, "read_and_clip_image: bpl=%d, h=%d\n", bpl, h);
    debug(1, "read_and_clip_image: clipleft=%d data=%d clipright=%d\n",
		UpperLeftX/pixelsPerByte, bpl, rightBpl - bpl);

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
    for (y = 0; y < h; ++y, rowp += bpl)
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
	rc = read_and_clip_image(buf, rawBpl, rightBpl, 2, bpl, h, ifp);
	if (rc == EOF)
	    goto done;

	cmyk_page(buf, w, h, ofp);
    }

done:
    free(buf);
    return 0;
}

#if 0
    #include <ctype.h>
#else
    static int
    isdigit(int c)
    {
	return (c >= '0' && c <= '9');
    }
#endif

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

void
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
pbm_pages(FILE *ifp, FILE *ofp)
{
    unsigned char	*buf;
    int			c1, c2;
    int			rawW, rawH, rawBpl;
    int			rightBpl;
    int			w, h, bpl;
    int			rc;
    int			first = 1;

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
	if (first)
	    first = 0;	// P4 already eaten in main
	else
	{
	    c1 = getc(ifp);
	    if (c1 == EOF)
		break;
	    c2 = getc(ifp);
	    if (c1 != 'P' || c2 != '4')
		error(1, "Not a pbmraw data stream\n");
	}

	skip_to_nl(ifp);

	rawW = getint(ifp);
	rawH = getint(ifp);
	skip_to_nl(ifp);

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

	buf = malloc(bpl * h);
	if (!buf)
	    error(1, "Can't allocate page buffer\n");

	rc = read_and_clip_image(buf, rawBpl, rightBpl, 8, bpl, h, ifp);
	if (rc == EOF)
	    error(1, "Premature EOF(pbm) on input stream\n");

	pbm_page(buf, w, h, ofp);

	free(buf);
    }
    return (0);
}

int
pgm_pages(FILE *ifp, FILE *ofp, int first)
{
    unsigned char	*buf;
    int			c1, c2;
    int			rawW, rawH, maxVal;
    int			rawBpl, rightBpl;
    int			w, h, bpl;
    int			rc;

    for (;;)
    {
	if (first)
	    first = 0;	// P5 already eaten in main
	else
	{
	    c1 = getc(ifp);
	    if (c1 == EOF)
		break;
	    c2 = getc(ifp);
	    if (c1 != 'P' || c2 != '5')
		error(1, "Not a pgmraw data stream\n");
	}

	skip_to_nl(ifp);

	rawW = getint(ifp);
	rawH = getint(ifp);
	maxVal = getint(ifp);
	skip_to_nl(ifp);
	if (maxVal != 255)
		error(1, "Don't know how to handle pgm maxVal '%d'\n", maxVal);

	rawBpl = rawW;

	w = rawW - UpperLeftX - LowerRightX;
	h = rawH - UpperLeftY - LowerRightY;
	bpl = w;
	rightBpl = rawW - UpperLeftX;

	buf = malloc(bpl * h);
	if (!buf)
	    error(1, "Can't allocate page buffer\n");

	rc = read_and_clip_image(buf, rawBpl, rightBpl, 1, bpl, h, ifp);
	if (rc == EOF)
	    error(1, "Premature EOF(pgm) on input stream\n");

	pgm_page(buf, w, h, ofp);

	free(buf);
    }
    return (0);
}

int
cups_pages(FILE *ifp, FILE *ofp)
{
    unsigned char	*buf;
    int			bpc;
    int			rawW, rawH, rawBpl;
    int			rightBpl;
    int			w, h, bpl;
    int			rc;
    char		hdr[512];

    //
    // Save the original Upper Right clip values as the logical offset,
    // because we may adjust them slightly below, in the interest of speed.
    //
    if (LogicalClip & LOGICAL_CLIP_X)
	LogicalOffsetX = UpperLeftX;
    if (LogicalClip & LOGICAL_CLIP_Y)
	LogicalOffsetY = UpperLeftY;

    // 00000000: 74 53 61 52 00 00 00 00  00 00 00 00 00 00 00 00 | tSaR
    // 00000170: 00 00 00 00 00 00 00 00  ec 13 00 00 c8 19 00 00
    // 00000180: 00 00 00 00 02 00 00 00  02 00 00 00 ec 13 00 00

    if (fread(hdr, 4, 1, ifp) != 1)
	error(1, "Preamture EOF reading magic number\n");
    if (memcmp(hdr, "tSaR", 4) != 0 && memcmp(hdr, "RaSt", 4) != 0)
	error(1, "Illegal magic number\n");
    if (fread(hdr, 0x178-4, 1, ifp) != 1)
	error(1, "Preamture EOF skipping start of CUPS header\n");

    if (fread(&rawW, 4, 1, ifp) != 1)
	error(1, "Preamture EOF reading width\n");
    if (fread(&rawH, 4, 1, ifp) != 1)
	error(1, "Preamture EOF reading height\n");

    if (fread(&hdr, 4, 1, ifp) != 1)
	error(1, "Preamture EOF skipping mediaType\n");

    if (fread(&bpc, 4, 1, ifp) != 1)
	error(1, "Preamture EOF reading height\n");
    if (bpc != 2)
	error(1, "Illegal number of bits per color (%d)\n", bpc);

    if (fread(&hdr, 4, 1, ifp) != 1)
	error(1, "Preamture EOF skipping bitPerPixel\n");

    if (fread(&rawBpl, 4, 1, ifp) != 1)
	error(1, "Preamture EOF reading height\n");

    if (fread(&hdr, 6*4, 1, ifp) != 1)
	error(1, "Preamture EOF skipping end of CUPS header\n");

    debug(1, "%d x %d, %d\n", rawW, rawH, rawBpl);

    // We only clip multiples of 1 pixels off the leading edge, and
    // add any remainder to the amount we clip from the right edge.
    // Its fast, and good enough for government work.
    LowerRightX += UpperLeftX & 0;
    UpperLeftX &= ~0;

    w = rawW - UpperLeftX - LowerRightX;
    h = rawH - UpperLeftY - LowerRightY;
    bpl = (w + 0) / 1;
    rightBpl = (rawW - UpperLeftX + 0) / 1;

    buf = malloc(bpl * h);
    if (!buf)
	error(1, "Unable to allocate page buffer of %d x %d = %d bytes\n",
		rawW, rawH, rawBpl * rawH);

    for (;;)
    {
	rc = read_and_clip_image(buf, rawBpl, rightBpl, 1, bpl, h, ifp);
	if (rc == EOF)
	    goto done;

	cups_page(buf, w, h, ofp);
    }

done:
    free(buf);
    return 0;
}

void
do_one(FILE *in)
{
    int	mode;

    mode = getc(in);
    if (mode == 't')
    {
	ungetc(mode, in);
	cups_pages(in, stdout);
    }
    else if (mode != 'P' || Mode == MODE_COLOR)
    {
	ungetc(mode, in);
	cmyk_pages(in, stdout);
    }
    else
    {
	mode = getc(in);
	if (mode == '4')
	    pbm_pages(in, stdout);
	else if (mode == '5')
	    pgm_pages(in, stdout, 1);
	else
	    error(1, "Not a bitcmyk, cups, pbm, or pgm file!\n");
    }
}

int
main(int argc, char *argv[])
{
    int	c;

    while ( (c = getopt(argc, argv,
		    "b:cd:g:n:m:p:r:s:u:l:z:L:ABJ:M:S:U:D:V?h")) != EOF)
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
	case 'd':	Duplex = atoi(optarg);
			if (Duplex < 1 || Duplex > 3)
			    error(1, "Illegal value '%s' for -d\n", optarg);
			break;
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
        case 'z':       Model = atoi(optarg);
                        if (Model < 0 || Model > MODEL_LAST)
                            error(1, "Illegal value '%s' for -z\n", optarg);
                        break;
	case 'M':	Mirror = atoi(optarg); break;
	case 'A':	AllIsBlack = !AllIsBlack; break;
	case 'B':	BlackClears = !BlackClears; break;
	case 'J':	if (optarg[0]) Filename = optarg; break;
	case 'U':	if (optarg[0]) Username = optarg; break;
	case 'D':	Debug = atoi(optarg);
			if (Debug == 12345678)
			{
			    // Hack to force time to zero for regression tests
			    ZeroTime = 1;
			    Debug = 0;
			}
			break;
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

    if (Model == MODEL_KM1635)
    {
	JbgOptions[0] = 8;
	JbgOptions[1] = JBG_DELAY_AT | JBG_LRLTWO | JBG_TPBON;
	JbgOptions[3] = 32;
    }

    start_doc(stdout);

    if (argc == 0)
    {
	do_one(stdin);
    }
    else
    {
	int	i;

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
    end_doc(stdout);
    exit(0);
}
