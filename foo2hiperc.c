/*

GENERAL
This program converts pbm (B/W) images and 1-bit-per-pixel cmyk images
(both produced by ghostscript) to Zenographics ZJ-stream format. There
is some information about the ZJS format at http://ddk.zeno.com.

With this utility, you can print to some Oki printers, such as these:
    - Oki C310dn, C3100, C3200, C3300n, C3400n, C5100n, C5500n

AUTHORS
This program began life as Robert Szalai's 'pbmtozjs' program.  It
also uses Markus Kuhn's jbig-kit compression library (included, but
also available at http://www.cl.cam.ac.uk/~mgk25/jbigkit/).

The program was overhauled by Rick Richardson to limit data chunk size
to 65536 bytes, add command line options, add color support,
and other miscellaneous features.

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

static char Version[] = "$Id: foo2hiperc.c,v 1.37 2019/05/17 21:55:07 rick Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#ifdef linux
    #include <sys/utsname.h>
#endif
#include "jbig.h"
#include "hiperc.h"

/*
 * Command line options
 */
int	Debug = 0;
int	ResX = 600;
int	ResY = 600;
int	Bpp = 1;
int	PaperCode = 2; //DMPAPER_LETTER;
int	PageWidth = 600 * 8.5;
int	PageHeight = 600 * 11;
int	UpperLeftX = 0;
int	UpperLeftY = 0;
int	LowerRightX = 0;
int	LowerRightY = 0;
int	Copies = 1;
int	Duplex = DMDUPLEX_OFF;
int	SourceCode = DMBIN_AUTO;
int	MediaCode = DMMEDIA_PLAIN;
char	*Username = NULL;
char	*Filename = NULL;
int	Mode = 1;
		#define MODE_MONO	1
		#define MODE_COLOR	2

int	Color2Mono = 0;
int	BlackClears = 0;
int	AllIsBlack = 0;
int	OutputStartPlane = 1;
int	ExtraPad = 16;

int	LogicalOffsetX = 0;
int	LogicalOffsetY = 0;

#define LOGICAL_CLIP_X	2
#define LOGICAL_CLIP_Y	1
int	LogicalClip = LOGICAL_CLIP_X | LOGICAL_CLIP_Y;
int	SaveToner = 0;
int	PageNum = 0;
int	RealWidth;
int	EconoMode = 0;
int	Compressed = 0;

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
    0, //JBG_ILEAVE | JBG_SMID,
    /* Options */
    JBG_DELAY_AT | JBG_LRLTWO | JBG_TPBON,
    /* L0 */
    256,
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
"   foo2hiperc [options] <pbmraw-file >hiperc-file\n"
"\n"
"	Convert Ghostscript pbmraw format to a monochrome HIPERC stream,\n"
"	for driving the Oki C310, C3100 to C5800 color laser printers.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"		-sPAPERSIZE=letter -r600x600 -sDEVICE=pbmraw \\ \n"
"		-sOutputFile=- - < testpage.ps \\ \n"
"	| foo2hiperc -r600x600 -g5100x6600 -p1 >testpage.zm\n"
"\n"
"   foo2hiperc [options] <bitcmyk-file >hiperc-file\n"
"   foo2hiperc [options] <pksmraw-file >hiperc-file\n"
"\n"
"	Convert Ghostscript bitcmyk or pksmraw format to a color HIPERC stream,\n"
"	for driving the Oki C310, C3100 to C5800 color laser printers\n"
"	N.B. Color correction is expected to be performed by ghostscript.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"	    -sPAPERSIZE=letter -g5100x6600 -r600x600 -sDEVICE=bitcmyk \\ \n"
"	    -sOutputFile=- - < testpage.ps \\ \n"
"	| foo2hiperc -r600x600 -g5100x6600 -p1 >testpage.zc\n"
"\n"
"Normal Options:\n"
"-c                Force color mode if autodetect doesn't work\n"
"-d duplex         Duplex code to send to printer [%d]\n"
"                    1=off, 2=longedge, 3=shortedge\n"
"-g <xpix>x<ypix>  Set page dimensions in pixels [%dx%d]\n"
"-m media          Media code to send to printer [%d]\n"
"                    0=plain 1=labels 2=transparency\n"
"-p paper          Paper code to send to printer [%d]\n"
"                    1=A4, 2=letter, 3=legal, 5=A5, 6=B5, 7=A6, 8=envMonarch,\n"
"                    9=envDL, 10=envC5, 11=env#10, 12=executive, 13=env#9,\n"
"                    14=legal135, 15=A3, 16=tabloid/ledger\n"
"-n copies         Number of copies [%d]\n"
"-r <xres>x<yres>  Set device resolution in pixels/inch [%dx%d]\n"
"-s source         Source code to send to printer [%d]\n"
"                    0=auto 1=tray1 2=tray2 3=multi 4=manual\n"
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
"-P                Do not output START_PLANE codes.  May be needed by some\n"
"                  some black and white only printers.\n"
"-X padlen         Add extra zero padding to the end of BID segments [%d]\n"
"-Z compressed     Use uncompressed (0) or compressed (1) data [%d]\n"
"\n"
"Debugging Options:\n"
"-S plane          Output just a single color plane from a color print [all]\n"
"                  1=Cyan, 2=Magenta, 3=Yellow, 4=Black\n"
"-D lvl            Set Debug level [%d]\n"
"-V                Version %s\n"
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
    , ExtraPad
    , Compressed
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

/*
 * This creates a linked list of compressed data.  The first item
 * in the list is the BIH and is always 20 bytes in size.  Each following
 * item is 65536 bytes in length.  The last item length is whatever remains.
 */
void
output_jbig(unsigned char *start, size_t len, void *cbarg)
{
    BIE_CHAIN	*current, **root = (BIE_CHAIN **) cbarg;
    int		size = 0x80000;	// Printer does strange things otherwise.

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

void
start_page_compressed(int nbie, int w, int h, int plane, unsigned char *bih,
			FILE *ofp)
{
    int		h256 = ((h + 255) / 256) * 256;
    DWORD	rec[13];
    int		rc;

    rec[0] = be32(52);			//reclen=52
    rec[1] = be32(0);			//rectype=0

    rec[2] = be32(16);				//block0: len=16
    //rec[3] = be32( (1<<24) + (plane<<16) + (128<<8) + 17);	//block0: data
    if (ResX == 300)
	rec[3] = be32( (nbie<<24) + (plane<<16) + (128<<8) + 0);
    else if (ResY == 1200)
	rec[3] = be32( (nbie<<24) + (plane<<16) + (128<<8) + 33);
    else
	rec[3] = be32( (nbie<<24) + (plane<<16) + (128<<8) + 17);
    rec[4] = be32(w);				//block0: width
    rec[5] = be32(0);				//block0: data
    rec[6] = be32(0x10000000);			//block0: data

    rec[7] = be32(20);			//block1: len=20
    rc = fwrite(rec, 32, 1, ofp);
    if (rc == 0) error(1, "fwrite(1): rc == 0!\n");

    ((DWORD *) bih)[2] = be32(h256);
    rc = fwrite(bih, 20, 1, ofp);
    if (rc == 0) error(1, "fwrite(2): rc == 0!\n");
}

void
write_plane_compressed(int nbie, unsigned char *buf, int w, int h, int plane,
			FILE *ofp)
{
    int		y;
    int		ns = 256;

    for (y = 0; y < h; y += ns)
    {
	struct jbg_enc_state	se;
	BIE_CHAIN		*chain;
	BIE_CHAIN		*current;
	unsigned char		*bitmaps[1];
	int			lines = (h-y) > ns ? ns : (h-y);
	int			chainlen;
	DWORD			rec[5];
	int			rc;

	bitmaps[0] = buf + y * ((w+7)/8);
	chain = NULL;
	JbgOptions[2] = (lines < ns) ? lines : ns;

	jbg_enc_init(&se, w, lines, 1, bitmaps, output_jbig, &chain);
	jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
			JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	jbg_enc_out(&se);
	jbg_enc_free(&se);

	if (chain->len != 20)
	    error(1,"Program error: missing BIH at start of chain\n"); 
	if (y == 0)
	    start_page_compressed(nbie, w, h, plane, chain->data, ofp);

	chainlen = 0;
	for (current = chain->next; current; current = current->next)
	    chainlen += current->len;

	rec[0] = be32(chainlen + 20);	//reclen
	rec[1] = be32(1);		//rectype=1

	rec[2] = be32(4);		//block0: len=4
	rec[3] = be32(plane << 24);	//block0: black

	rec[4] = be32(chainlen);	//block1: len
	rc = fwrite(rec, 20, 1, ofp);
	if (rc == 0) error(1, "fwrite(3): rc == 0!\n");
	for (current = chain->next; current; current = current->next)
	{
	    rc = fwrite(current->data, 1, current->len, ofp);
	    if (rc == 0) error(1, "fwrite(4): rc == 0!\n");
	}
    }
}

int
write_plane(int pn, BIE_CHAIN **root, FILE *ofp)
{
    BIE_CHAIN	*current = *root;
    // BIE_CHAIN	*next;
    // int		len;
    int		w, h;
    int		stripe = 0;

    debug(3, "Write Plane %d\n", pn); 

    /* error handling */
    if (!current) 
	error(1,"There is no JBIG!\n"); 
    if (!current->next)
	error(1,"There is no or wrong JBIG header!\n"); 
    if (current->len != 20)
	error(1,"wrong BIH length\n"); 

    w = (((long) current->data[ 4] << 24)
	    | ((long) current->data[ 5] << 16)
	    | ((long) current->data[ 6] <<  8)
	    | (long) current->data[ 7]);
    h = (((long) current->data[ 8] << 24)
	    | ((long) current->data[ 9] << 16)
	    | ((long) current->data[10] <<  8)
	    | (long) current->data[11]);

    start_page_compressed(1, w, h, pn-1, current->data, ofp);

    for (current = (*root)->next; current && current->len;
						    current = current->next)
    {
	DWORD rec[5];
	int blklen = current->len;
	unsigned char *buf = current->data;
	int plane = pn - 1;
	int y = 0;
	int rc;

	// len = current->len;
	// next = current->next;
	++stripe;

	rec[0] = be32(blklen + 20);	//reclen
	rec[1] = be32(1);		//rectype=1

	rec[2] = be32(4);		//block0: len=4
	rec[3] = be32(plane << 24);	//block0: black

	rec[4] = be32(blklen);	//block1: len
	rc = fwrite(rec, 20, 1, ofp);
	if (rc == 0) error(1, "fwrite(5): rc == 0!\n");
	rc = fwrite(buf + y*(w/8), 1, blklen, ofp);
	if (rc == 0) error(1, "fwrite(6): rc == 0!\n");
    }
    free_chain(*root);

    return 0;
}

void
end_page(FILE *ofp)
{
    DWORD	rec[2];
    static int	pageno = 0;
    int		rc;

    rec[0] = be32(8);	//reclen=8
    rec[1] = be32(255);	//rectype=255
    rc = fwrite(rec, 8, 1, ofp);
    if (rc == 0) error(1, "fwrite(7): rc == 0!\n");

    ++pageno;
    if (IsCUPS)
	fprintf(stderr, "PAGE: %d %d\n", pageno, Copies);
}

void
start_page_uncompressed(int nbie, int w, int h, int plane, FILE *ofp)
{
    DWORD	rec[13];
    int		h256 = ((h + 255) / 256) * 256;
    int		rc;

    rec[0] = be32(52);			//reclen=52
    rec[1] = be32(0);			//rectype=0

    rec[2] = be32(16);				//block0: len=16
    if (ResX == 300)
	rec[3] = be32( (nbie<<24) + (plane<<16) + 0);	//block0: data
    else if (ResY == 1200)
	rec[3] = be32( (nbie<<24) + (plane<<16) + 33);	//block0: data
    else
	rec[3] = be32( (nbie<<24) + (plane<<16) + 17);	//block0: data
    rec[4] = be32(w);				//block0: width
    rec[5] = be32(0);				//block0: data
    if (Duplex == DMDUPLEX_OFF)
	rec[6] = be32(0);			//block0: data
    else
	rec[6] = be32( (PageNum & 1) ? 0x100 : 0x200);	//block0: data

    rec[7] = be32(20);			//block1: len=20
    rec[8] = be32(0x30303130);		//block1: "0010"
    rec[9] = be32(w);			//block1: width
    rec[10] = be32(h256);			//block1: height
    rec[11] = be32(256);		//block1: rows
    rec[12] = be32(0);			//block1: data
    rc = fwrite(rec, 52, 1, ofp);
    if (rc == 0) error(1, "fwrite(8): rc == 0!\n");
}

void
write_plane_uncompressed(unsigned char *buf, int w, int h, int plane, FILE *ofp)
{
    int		h256 = ((h + 255) / 256) * 256;
    int		w256 = 256 * w/8;
    int		x, y;
    int		blklen;
    DWORD	rec[5];
    int		rc;

    for (y = 0; y < h; y += 256)
    {
	if ((y+256) <= h)
	    blklen = 256 * w/8;
	else
	    blklen = (h - y) * w/8;
	rec[0] = be32(w256 + 20);	//reclen
	rec[1] = be32(1);		//rectype=1

	rec[2] = be32(4);		//block0: len=4
	rec[3] = be32(plane << 24);	//block0: black

	rec[4] = be32(w256);		//block1: len
	rc = fwrite(rec, 20, 1, ofp);
	if (rc == 0) error(1, "fwrite(9): rc == 0!\n");
	rc = fwrite(buf + y*(w/8), 1, blklen, ofp);
	if (rc == 0) error(1, "fwrite(10): rc == 0!\n");
    }

    // Pad to 256 lines...
    for (y = h; y < h256; ++y)
	for (x = 0; x < w/8; ++x)
	    fputc(0, ofp);
}

int
write_page(BIE_CHAIN **root, BIE_CHAIN **root2,
	BIE_CHAIN **root3, BIE_CHAIN **root4, FILE *ofp)
{
    int	nbie = root2 ? 4 : 1;

//    start_page(root, nbie, ofp);

    if (root)
    {
	if (OutputStartPlane)
	    write_plane(nbie == 1 ? 4 : 1, root, ofp);
	else
	    write_plane(nbie == 1 ? 0 : 1, root, ofp);
    }
    if (root2)
	write_plane(2, root2, ofp);
    if (root3)
	write_plane(3, root3, ofp);
    if (root4)
	write_plane(4, root4, ofp);

    end_page(ofp);
    return 0;
}

void
start_doc(FILE *ofp)
{
    time_t	now;
    struct tm	*tmp;
    char	datetime[256+1];
    char	*device_uri;
    char	*strmedia[] =
		{
		    "PLAIN", "LABELS", "TRANSPARENCY"
		};
    char	*strpaper[] =
		{
		    /*0*/	"CUSTOM", "A4", "LETTER", "LEGAL", "LEGAL13",
		    /*5*/	"A5", "B5", "A6", "MONARCH", "DL",
		    /*10*/	"C5", "COM10", "EXECUTIVE", "COM9", "LEGAL135",
		    /*15*/	"A3", "TABLOID"
		};
    #define STRARY(X, A) \
            ((X) >= 0 && (X) < sizeof(A)/sizeof(A[0])) \
            ? A[X] : "NORMAL"

    fprintf(ofp, "\033%%-12345X@PJL\r\n");
    fprintf(ofp, "@PJL RDYMSG DISPLAY = \"%s\"\r\n",
		    Username ? Username : "Unknown");
    fprintf(ofp, "@PJL SET OKIJOBACCOUNTJOB USERID=\"%s\" JOBNAME=\"%s\"\r\n",
		    Username ? Username : "Unknown",
		    Filename ? Filename : "Unknown"
		    );
    fprintf(ofp, "@PJL SET OKIAUXJOBINFO DATA=\"DocumentName=%s\"\r\n",
		    Filename ? Filename : "Unknown"
		    );

    #ifdef linux
    {
	struct utsname u;

	uname(&u);
	fprintf(ofp, "@PJL SET OKIAUXJOBINFO DATA=\"ComputerName=%s\"\r\n",
	    u.nodename);
    }
    #endif

    now = time(NULL);
    tmp = localtime(&now);
    strftime(datetime, sizeof(datetime), "00:00:00 %Y/%m/%d", tmp);
    fprintf(ofp, "@PJL SET OKIAUXJOBINFO DATA=\"ReceptionTime=%s\"\r\n",
	datetime);

    device_uri = getenv("DEVICE_URI");
    if (device_uri)
	fprintf(ofp, "@PJL SET OKIAUXJOBINFO DATA=\"PortName=%s\"\r\n",
	    device_uri);

    if (SourceCode == DMBIN_AUTO)
	fprintf(ofp, "@PJL SET OKIAUTOTRAYSWITCH=ON\r\n");
    else
	fprintf(ofp, "@PJL SET OKIAUTOTRAYSWITCH=OFF\r\n");

    fprintf(ofp, "@PJL SET OKIPAPERSIZECHECK=ENABLE\r\n");
    switch (ResX)
    {
    case 300:	fprintf(ofp, "@PJL SET RESOLUTION=300\r\n"); break;
    case 600:	fprintf(ofp, "@PJL SET RESOLUTION=600\r\n"); break;
    default:	error(1, "Illegal X resolution\n"); break;
    }
    switch (ResY)
    {
    case 300:
    case 600: break;
    case 1200:	fprintf(ofp, "@PJL SET RESOLUTION=V1200\r\n"); break;
    }

    fprintf(ofp, "@PJL SET PAPER=%s\r\n", STRARY(PaperCode, strpaper));
    fprintf(ofp, "@PJL SET OKITRAYSEQUENCE=PAPERFEEDTRAY\r\n");

    switch (SourceCode)
    {
    case DMBIN_AUTO:
    case DMBIN_TRAY1:
	    fprintf(ofp, "@PJL SET OKIPAPERFEED=TRAY1\r\n");
	    break;
    case DMBIN_TRAY2:
	    fprintf(ofp, "@PJL SET OKIPAPERFEED=TRAY2\r\n");
	    break;
    case DMBIN_MULTI:
	    fprintf(ofp, "@PJL SET OKIPAPERFEED=FRONTTRAY\r\n");
	    break;
    case DMBIN_MANUAL:
	    fprintf(ofp, "@PJL SET OKIPAPERFEED=FRONTTRAY\r\n");
	    fprintf(ofp, "@PJL SET MANUALFEED=ON\r\n");
	    break;
    }

    fprintf(ofp, "@PJL SET OKIMEDIATYPE = %s\r\n", STRARY(MediaCode, strmedia));

    fprintf(ofp, "@PJL SET LPARM:PCL OKIPRINTMARGIN=INCH1D6\r\n");

    fprintf(ofp, "@PJL SET COPIES=%d\r\n", Copies);
    fprintf(ofp, "@PJL SET QTY=1\r\n");
    fprintf(ofp, "@PJL SET HIPERCEFFECTIVEBLOCKSIZE=%d\r\n",
	PageWidth * PageHeight / 8);

    switch (Duplex)
    {
    case DMDUPLEX_LONGEDGE:
    case DMDUPLEX_MANUALLONG:
	fprintf(ofp, "@PJL SET DUPLEX=ON\r\n");
	//fprintf(ofp, "@PJL SET BINDING=LONGEDGE\r\n");
	break;
    case DMDUPLEX_SHORTEDGE:
    case DMDUPLEX_MANUALSHORT:
	fprintf(ofp, "@PJL SET DUPLEX=ON\r\n");
	//fprintf(ofp, "@PJL SET BINDING=SHORTEDGE\r\n");
	break;
    }
    fprintf(ofp, "@PJL ENTER LANGUAGE=HIPERC\n");
}

void
end_doc(FILE *ofp)
{
    //fprintf(ofp, "%c", 9);
    fprintf(ofp, "\033%%-12345X@PJL\r\n");
    fprintf(ofp, "@PJL EOJ NAME = \"End \"\n");
    fprintf(ofp, "\033%%-12345X");
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
    debug(1, "w=%d, bpl=%d, rawbpl=%d\n", w, bpl, rawbpl);
    AnyColor = 0;
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
    BIE_CHAIN *chain[4];
    int i;
    int	bpl, bpl16;
    unsigned char *plane[4], *bitmaps[4][1];
    struct jbg_enc_state se[4]; 

    RealWidth = w;
    w = (w + 127) & ~127;
    bpl = (w + 7) / 8;
    bpl16 = (bpl + 15) & ~15;
    debug(1, "w = %d, bpl = %d, bpl16 = %d\n", w, bpl, bpl16);

    for (i = 0; i < 4; ++i)
    {
	plane[i] = malloc(bpl16 * h);
	if (!plane[i]) error(3, "Cannot allocate space for bit plane\n");
	chain[i] = NULL;
    }

    cmyk_planes(plane, raw, RealWidth, h);

    if (Compressed)
    {
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

	    *bitmaps[i] = plane[i];

	    jbg_enc_init(&se[i], w, h, 1, bitmaps[i], output_jbig, &chain[i]);
	    jbg_enc_options(&se[i], JbgOptions[0], JbgOptions[1],
			    JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	    jbg_enc_out(&se[i]);
	    jbg_enc_free(&se[i]);
	}

	if (Color2Mono)
	    write_page(&chain[Color2Mono-1], NULL, NULL, NULL, ofp);
	else if (AnyColor)
	    write_page(&chain[0], &chain[1], &chain[2], &chain[3], ofp);
	else
	    write_page(&chain[3], NULL, NULL, NULL, ofp);
    }
    else
    {
	if (Color2Mono)
	{
	    i = Color2Mono - 1;
	    start_page_uncompressed(1, w, h, i, ofp);
	    write_plane_uncompressed(plane[i], w, h, i, ofp);
	}
	else if (AnyColor)
	{
	    for (i = 0; i < 4; ++i)
		start_page_uncompressed(4, w, h, i, ofp);
	    for (i = 0; i < 4; ++i)
		write_plane_uncompressed(plane[i], w, h, i, ofp);
	}
	else
	{
	    start_page_uncompressed(1, w, h, 3, ofp);
	    write_plane_uncompressed(plane[3], w, h, 3, ofp);
	}
	end_page(ofp);
    }

    for (i = 0; i < 4; ++i)
	free(plane[i]);
    return 0;
}

int
pksm_page(unsigned char *plane[4], int w, int h, FILE *ofp)
{
    BIE_CHAIN *chain[4];
    int i;
    unsigned char *bitmaps[4][1];
    struct jbg_enc_state se[4]; 

    RealWidth = w;
    w = (w + 127) & ~127;
    debug(1, "w = %d\n", w);

    if (Compressed)
    {
	for (i = 0; i < 4; ++i)
	    chain[i] = NULL;

	for (i = 0; i < 4; ++i)
	{
	    *bitmaps[i] = plane[i];

	    jbg_enc_init(&se[i], w, h, 1, bitmaps[i], output_jbig, &chain[i]);
	    jbg_enc_options(&se[i], JbgOptions[0], JbgOptions[1],
			    JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	    jbg_enc_out(&se[i]);
	    jbg_enc_free(&se[i]);
	}

	if (Color2Mono)
	    write_page(&chain[Color2Mono-1], NULL, NULL, NULL, ofp);
	else if (AnyColor)
	    write_page(&chain[0], &chain[1], &chain[2], &chain[3], ofp);
	else
	    write_page(&chain[3], NULL, NULL, NULL, ofp);
    }
    else
    {
	if (Color2Mono)
	{
	    i = Color2Mono - 1;
	    start_page_uncompressed(1, w, h, i, ofp);
	    write_plane_uncompressed(plane[i], w, h, i, ofp);
	}
	else if (AnyColor)
	{
	    for (i = 0; i < 4; ++i)
		start_page_uncompressed(4, w, h, i, ofp);
	    for (i = 0; i < 4; ++i)
		write_plane_uncompressed(plane[i], w, h, i, ofp);
	}
	else
	{
	    start_page_uncompressed(1, w, h, 3, ofp);
	    write_plane_uncompressed(plane[3], w, h, 3, ofp);
	}
	end_page(ofp);
    }

    return 0;
}

int
pbm_page(unsigned char *buf, int w, int h, FILE *ofp)
{
    // BIE_CHAIN		*chain = NULL;
    unsigned char	*bitmaps[1];
    // struct jbg_enc_state se; 

    RealWidth = w;
    w = (w + 127) & ~127;

    if (SaveToner)
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

    *bitmaps = buf;

    if (Compressed)
    {
	// start_page_compressed(1, w, h, 3, ofp);

	write_plane_compressed(1, buf, w, h, 3, ofp);

	end_page(ofp);
#if 0
	jbg_enc_init(&se, w, h, 1, bitmaps, output_jbig, &chain);
	jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
			    JbgOptions[2], JbgOptions[3], JbgOptions[4]);
	jbg_enc_out(&se);
	jbg_enc_free(&se);

	write_page(&chain, NULL, NULL, NULL, ofp);
#endif
    }
    else
    {
	start_page_uncompressed(1, w, h, 3, ofp);

	write_plane_uncompressed(buf, w, h, 3, ofp);

	end_page(ofp);
    }

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
	if (rc != 1) error(1, "fscanf: rc == 0!");
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
	    debug(1, "bpl=%d bpl16=%d\n", bpl, bpl16);

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
    int			bpl16 = 0;
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
	debug(1, "bpl=%d bpl16=%d\n", bpl, bpl16);

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

void
blank_page(FILE *ofp)
{
    int			w, h, bpl, bpl16 = 0;
    unsigned char	*plane;
    
    w = PageWidth - UpperLeftX - LowerRightX;
    h = PageHeight - UpperLeftY - LowerRightY;
    bpl = (w + 7) / 8;
    bpl16 = (bpl + 15) & ~15;

    plane = malloc(bpl16 * h);
    if (!plane)
    error(1, "Unable to allocate blank plane (%d bytes)\n", bpl16*h);
    memset(plane, 0, bpl16*h);

    ++PageNum;
    pbm_page(plane, w, h, ofp);
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

    if (Mode == MODE_COLOR)
    {
	mode = getc(in);
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
	mode = getc(in);
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
    int i, j;

    while ( (c = getopt(argc, argv,
		    "cd:g:n:m:p:r:s:tu:l:L:ABPJ:S:U:X:Z:D:V?h")) != EOF)
	switch (c)
	{
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
	case 'P':	OutputStartPlane = !OutputStartPlane; break;
	case 'J':	if (optarg[0]) Filename = optarg; break;
	case 'U':	if (optarg[0]) Username = optarg; break;
	case 'X':	ExtraPad = atoi(optarg); break;
	case 'Z':	Compressed = atoi(optarg); break;
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

    if (getenv("ccc"))
	Compressed = 1;

    Bpp = ResX / 600;
    //ResX = 600;
    if (0 && SaveToner)
    {
	SaveToner = 0;
	EconoMode = 1;
    }

    switch (Duplex)
    {
    case DMDUPLEX_MANUALLONG:
    case DMDUPLEX_MANUALSHORT:
	EvenPages = tmpfile();
	break;
    }

    start_doc(stdout);

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
