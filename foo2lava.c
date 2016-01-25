/*

GENERAL
This program converts pbm (B/W) images and 1-bit-per-pixel cmyk images
(both produced by ghostscript) to Zenographics LAVAFLOW format.

With this utility, you can print to some HP and Minolta/QMS printers,
such as these:
    Model 0:
     - Konica Minolta magicolor 2490 MF		B/W and color
     - Konica Minolta magicolor 2530 DL		B/W and color
     - Xerox Phaser 6115MFP			B/W and color

    Model 1:
     - Konica Minolta magicolor 2480 MF		B/W and color
						NOTE: Copies is unimplented!

    Model 2:
     - Konica Minolta magicolor 1600W		B/W and color
     - Konica Minolta magicolor 1680MF		B/W and color
     - Konica Minolta magicolor 1690MF		B/W and color
     - Konica Minolta magicolor 4690MF		B/W and color
     - Oki C110					B/W and color
     - Xerox Phaser 6121MFP			B/W and color

AUTHORS
It also uses Markus Kuhn's jbig-kit compression library (included, but
also available at http://www.cl.cam.ac.uk/~mgk25/jbigkit/).

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

static char Version[] = "$Id: foo2lava.c,v 1.40 2014/06/27 14:34:46 rick Exp $";

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

typedef enum
{
    DMDUPLEX_OFF        = 1,
    DMDUPLEX_LONGEDGE   = 2,
    DMDUPLEX_SHORTEDGE  = 3,
    DMDUPLEX_MANUALLONG = 4,
    DMDUPLEX_MANUALSHORT= 5
} DMDUPLEX;

/*
 * Command line options
 */
int	Debug = 0;
int	ResX = 1200;
int	ResY = 600;
int	Bpp = 1;
int	PaperCode = 1;
char	*PaperStr = "na_letter_8.5x11in";
int	PageWidth = 1200 * 8.5;
int	PageHeight = 600 * 11;
int	UpperLeftX = 0;
int	UpperLeftY = 0;
int	LowerRightX = 0;
int	LowerRightY = 0;
int	Copies = 1;
int	Duplex = 1;
int	SourceCode = 255;
int	MediaCode = 0;
char	*MediaStr = "plain";
char	*Username = NULL;
char	*Filename = NULL;
int	Mode = 0;
		#define MODE_MONO	1
		#define MODE_COLOR	2
int	Model = 0;
		#define MODEL_2530DL	0
		#define MODEL_2480MF	1
		#define MODEL_1600W	2
		#define MODEL_LAST	2

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
int	BihW;
int	BihH;
int	Dots[4];
int	TotalDots;

int	IsCUPS = 0;

FILE	*EvenPages = NULL;
typedef struct
{
    off_t	b, e;
} SEEKREC;
SEEKREC	SeekRec[2000];
int	SeekIndex = 0;
off_t	SeekMedia;

long JbgOptions[5] =
{
    /* Order */
    JBG_ILEAVE | JBG_SMID,
    /* Options */
    JBG_DELAY_AT | JBG_LRLTWO | JBG_TPDON | JBG_TPBON | JBG_DPON,
    /* L0 */
    128,
    /* MX */
    0,
    /* MY */
    0
};

void
usage(void)
{
    fprintf(stderr,
"Usage:\n"
"   foo2lava [options] <pbmraw-file >lava-file\n"
"\n"
"	Convert Ghostscript pbmraw format to a monochrome LAVAFLOW stream,\n"
"	for driving the Konica Minolta magicolor 2530 DL network color laser\n"
"	printer and other LAVAFLOW-based black and white printers.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"		-sPAPERSIZE=letter -r1200x600 -sDEVICE=pbmraw \\ \n"
"		-sOutputFile=- - < testpage.ps \\ \n"
"	| foo2lava -r1200x600 -g10200x6600 -p1 >testpage.zm\n"
"\n"
"   foo2lava [options] <bitcmyk-file >lava-file\n"
"   foo2lava [options] <pksmraw-file >lava-file\n"
"\n"
"	Convert Ghostscript bitcmyk or pksmraw format to a color LAVAFLOW\n"
"	stream, for driving the Konica Minolta magicolor 2530 DL network\n"
"	color laser printer.\n"
"	N.B. Color correction is expected to be performed by ghostscript.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"	    -sPAPERSIZE=letter -g10200x6600 -r1200x600 -sDEVICE=bitcmyk \\ \n"
"	    -sOutputFile=- - < testpage.ps \\ \n"
"	| foo2lava -r1200x600 -g10200x6600 -p1 >testpage.zc\n"
"\n"
"Normal Options:\n"
"-c                Force color mode if autodetect doesn't work\n"
"-d duplex         Duplex code to send to printer [%d]\n"
"                    1=off, 2=longedge, 3=shortedge\n"
"                    4=manual longedge, 5=manual shortedge\n"
"-g <xpix>x<ypix>  Set page dimensions in pixels [%dx%d]\n"
"-m media          Media code to send to printer [%d]\n"
"                    1=standard 4=transparency 20=thick stock 22=envelope\n"
"                    23=letterhead 25=postcard 26=labels 27=recycled\n"
"                    28=glossy\n"
"-p paper          Paper code to send to printer [%d]\n"
"                    1=executive 2=letter 3=legal 25=A5 26=A4 45=B5jis\n"
"                    65=B5iso 80=envMonarch 81=env#10 90=envDL 91=envC5\n"
"                    92=envC6 835=4x6\" 837=10x15cm\n"
"-n copies         Number of copies [%d]\n"
"-r <xres>x<yres>  Set device resolution in pixels/inch [%dx%d]\n"
"-s source         Source code to send to printer [%d]\n"
"                    1=tray 1 (multipurpose) 4=tray 2 255=auto\n"
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
"-z model          Model: 0=2530DL, 2=1600W (LAVAFLOW), 1=2480MF (OPL) [%d]\n"
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
    , Model
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

int BlackOnes[256] =
{
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
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

int
write_plane(int planeNum, BIE_CHAIN **root, FILE *fp)
{
    BIE_CHAIN	*current = *root;
    BIE_CHAIN	*next;
    int		len;

    debug(3, "Write Plane %d\n", planeNum); 

    /* error handling */
    if (!current) 
	error(1,"There is no JBIG!\n"); 
    if (!current->next)
	error(1,"There is no or wrong JBIG header!\n"); 
    if (current->len != 20)
	error(1,"wrong BIH length\n"); 

    if (Model == MODEL_2480MF)
    {
	int w, h, x = 0;

	switch (planeNum)
	{
	case 1: x = 0x00FFFF; break;
	case 2: x = 0xFF00FF; break;
	case 3: x = 0xFFFF00; break;
	}
	fprintf(fp, "RasterObject.Planes=%06X,0,0,0,0,0,0;", x);

	/* startpage, jbig_bih, jbig_bid, jbig_end, endpage */
	w = (((long) current->data[ 4] << 24)
	    | ((long) current->data[ 5] << 16)
	    | ((long) current->data[ 6] <<  8)
	    | (long) current->data[ 7]);
	h = (((long) current->data[ 8] << 24)
	    | ((long) current->data[ 9] << 16)
	    | ((long) current->data[10] <<  8)
	    | (long) current->data[11]);
	fprintf(fp, "RasterObject.Width=%d;", w);
	fprintf(fp, "RasterObject.Height=%d;", h);
    }

    for (current = *root; current && current->len; current = current->next)
    {
	if (current == *root)
	{
	    switch (Model)
	    {
	    case MODEL_1600W:
	    case MODEL_2530DL:
		fprintf(fp, "\033*b20V");
		fwrite(current->data, 1, current->len, fp);
		break;
	    case MODEL_2480MF:
		fprintf(fp, "RasterObject.Data#%d=", (int) current->len);
		fwrite(current->data, 1, current->len, fp);
		fprintf(fp, ";");
		break;
	    }
	}
	else
	{
	    int i;
	    int pad;
	    #define PAD 32

	    len = current->len;
	    next = current->next;
	    if (!next || !next->len)
                pad = PAD * ((len+PAD-1)/PAD) - len;
            else
                pad = 0;
	    switch (Model)
	    {
	    case MODEL_1600W:
	    case MODEL_2530DL:
		fprintf(fp, "\033*b%d%s", len + pad, next ? "V" : "W");
		fwrite(current->data, 1, len, fp);
		for (i = 0; i < pad; i++)
		    putc(0, fp);
		break;
	    case MODEL_2480MF:
		fprintf(fp, "RasterObject.Data#%d=", len + pad);
		fwrite(current->data, 1, len, fp);
		for (i = 0; i < pad; i++)
		    putc(0, fp);
		fprintf(fp, ";");
		break;
	    }
	}
    }

    free_chain(*root);

    if (Model == MODEL_2530DL || Model == MODEL_1600W)
    {
	switch (planeNum)
	{
	case 0: case 4:
	    fprintf(fp, "\033*x%dK", Dots[3]);
	    fprintf(fp, "\033*x%dW", TotalDots - Dots[3]);
	    break;
	case 1:
	    fprintf(fp, "\033*x%dC", Dots[0]);
	    fprintf(fp, "\033*x%dZ", TotalDots - Dots[0]);
	    break;
	case 2:
	    fprintf(fp, "\033*x%dM", Dots[1]);
	    fprintf(fp, "\033*x%dV", TotalDots - Dots[1]);
	    break;
	case 3:
	    fprintf(fp, "\033*x%dY", Dots[2]);
	    fprintf(fp, "\033*x%dU", TotalDots - Dots[2]);
	    break;
	}
    }

    return 0;
}

void
start_page(BIE_CHAIN **root, int nbie, FILE *ofp)
{
    BIE_CHAIN		*current = *root;
    unsigned long	w, h;
    int			i, np;
    static int		pageno = 0;

    /* error handling */
    if (!current)
	error(1, "There is no JBIG!\n"); 
    if (!current->next)
	error(1, "There is no or wrong JBIG header!\n"); 
    if (current->len != 20 )
	error(1,"wrong BIH length\n"); 

  /* startpage, jbig_bih, jbig_bid, jbig_end, endpage */
    w = (((long) current->data[ 4] << 24)
	    | ((long) current->data[ 5] << 16)
	    | ((long) current->data[ 6] <<  8)
	    | (long) current->data[ 7]);
    h = (((long) current->data[ 8] << 24)
	    | ((long) current->data[ 9] << 16)
	    | ((long) current->data[10] <<  8)
	    | (long) current->data[11]);
    TotalDots = w*h;

    switch (Model)
    {
    case MODEL_1600W:
    case MODEL_2530DL:
	fprintf(ofp, "\033&l%dO", 0);
	fprintf(ofp, "\033*r%dU",
	    (Mode == MODE_COLOR && nbie == 4) ? -1004 : 1);
	fprintf(ofp, "\033*g%dW",
	    (Mode == MODE_COLOR && nbie == 4) ? 26 : 8);
	putc(2, ofp);
	putc(nbie, ofp);
	np = (Mode == MODE_COLOR && nbie == 4) ? 4 : 1;
	for (i = 0; i < np; ++i)
	{
	    putc(ResX>>8, ofp);
	    putc(ResX, ofp);
	    putc(ResY>>8, ofp);
	    putc(ResY, ofp);
	    putc(0, ofp);
	    //putc(Mode == MODE_COLOR ? 2 : 0, ofp);	// Number of bpc
	    putc(2, ofp);
	}
	fprintf(ofp, "\033*b%dM", 1234);
	fprintf(ofp, "\033&l%dA", PaperCode);
	if (PaperCode == 101)
	{
	    fprintf(ofp, "\033&f%dG", PageHeight * 720 / ResY);	// Custom Y
	    fprintf(ofp, "\033&f%dF", PageWidth * 720 / ResX);	// Custom X
	}
	fprintf(ofp, "\033&l%dH", SourceCode);
	fprintf(ofp, "\033&l%dM", MediaCode);
	fprintf(ofp, "\033&l%dE", 0);
	fprintf(ofp, "\033*r%dS", (int) w);
	fprintf(ofp, "\033*r%dT", (int) h);
	fprintf(ofp, "\033&l%dU", 0);
	fprintf(ofp, "\033&l%dZ", 0);
	fprintf(ofp, "\033*p%dX", ResX / 6);
	fprintf(ofp, "\033*p%dY", ResX / 6);

	fprintf(ofp, "\033*r1A");
	break;
    case MODEL_2480MF:
	fprintf(ofp, "MediaSize=%s;", PaperStr);
	fprintf(ofp, "MediaType=%s;", MediaStr);
	fprintf(ofp, "MediaInputTrayCheck=top;");
	fprintf(ofp, "RasterObject.BitsPerPixel=1;");
	break;
    }

    if ((PageNum & 1) == 0 && EvenPages)
	SeekMedia = ftell(EvenPages) - 4;

    ++pageno;
    if (IsCUPS)
	fprintf(stderr, "PAGE: %d %d\n", pageno, Copies);
}

void
end_page(FILE *ofp)
{
    switch (Model)
    {
    case MODEL_1600W:
    case MODEL_2530DL:
	fprintf(ofp, "\033*rC");
	if (0)
	    fprintf(ofp, "\f");		// Eject page
	else
	    fprintf(ofp, "\033&l0H");	// Eject page
	break;
    case MODEL_2480MF:
	fprintf(ofp, "Event=EndOfPage;");
	break;
    }
}

int
write_page(BIE_CHAIN **root, BIE_CHAIN **root2,
	BIE_CHAIN **root3, BIE_CHAIN **root4, FILE *ofp)
{
    int	nbie = root2 ? 4 : 1;

    start_page(root, nbie, ofp);

    switch (Model)
    {
    case MODEL_1600W:
    case MODEL_2530DL:
	if (root3) write_plane(3, root3, ofp);
	if (root2) write_plane(2, root2, ofp);
	if (root)
	{
	    if (OutputStartPlane)
		write_plane(nbie == 1 ? 4 : 1, root, ofp);
	    else
		write_plane(nbie == 1 ? 0 : 1, root, ofp);
	}
	if (root4) write_plane(4, root4, ofp);
	break;
    case MODEL_2480MF:
	if (root)
	{
	    if (OutputStartPlane)
		write_plane(nbie == 1 ? 4 : 1, root, ofp);
	    else
		write_plane(nbie == 1 ? 0 : 1, root, ofp);
	}
	if (root2) write_plane(2, root2, ofp);
	if (root3) write_plane(3, root3, ofp);
	if (root4) write_plane(4, root4, ofp);
	break;
    }

    end_page(ofp);
    return 0;
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

    if (Model == MODEL_2480MF)
	size = 32768;

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
start_doc(FILE *fp)
{
    char	buf[64];
    time_t	now;
    struct tm	*tmp;

    switch (Model)
    {
    case MODEL_1600W:
    case MODEL_2530DL:
	now = time(NULL);
	tmp = localtime(&now);
	strftime(buf, sizeof(buf), "%m/%d/%Y", tmp);

	fprintf(fp, "\033%%-12345X@PJL JOB NAME=\"%s\"\n",
	    Filename ? Filename : "stdin");
	fprintf(fp, "\033%%-12345X@PJL JOB USERNAME=\"%s\"\n",
	    Username ? Username : "");
	fprintf(fp, "\033%%-12345X@PJL JOB TIMESTAMP=\"%s\"\n", buf);
	#ifdef linux
	{
	    struct utsname u;

	    u.release[0] = 0;
	    uname(&u);

	    fprintf(fp, "\033%%-12345X@PJL JOB OSINFO=\"Linux/%s\"\n",
		    u.release);
	}
	#else
	    fprintf(fp, "\033%%-12345X@PJL JOB OSINFO=\"%s\"\n", "Unknown");
	#endif
	fprintf(fp, "\033%%-12345X@PJL ENTER LANGUAGE=LAVAFLOW\n");
	fprintf(fp, "\033E");
	fprintf(fp, "\033&l%dS", Duplex-1);
	fprintf(fp, "\033&l%dG", 0);
	fprintf(fp, "\033&u%dD", ResX);
	fprintf(fp, "\033&l%dX", Copies);
	fprintf(fp, "\033&x%dX", 1);
	break;
    case MODEL_2480MF:
	fprintf(fp, "Event=StartOfJob;");
	fprintf(fp, "OSVersion=Linux;");
	fprintf(fp, "DrvVersion=2.0.1410.0;");
	fprintf(fp, "Resolution=%dx%d;", ResX, ResY);
	fprintf(fp, "RasterObject.Compression=JBIG;");
	fprintf(fp, "Sides=%sSided;", (Duplex-1) ? "Two" : "One");
	break;
    }
}

void
end_doc(FILE *fp)
{
    switch (Model)
    {
    case MODEL_1600W:
    case MODEL_2530DL:
	fprintf(fp, "\033E");
	fprintf(fp, "\033%%-12345X");
	break;
    case MODEL_2480MF:
	fprintf(fp, "Event=EndOfJob;");
	break;
    }
}

void
load_tray2(FILE *fp)
{
    //int			nitems;

    //nitems = 0;
    //chunk_write(ZJT_2600N_PAUSE, nitems, nitems * sizeof(ZJ_ITEM_UINT32), fp);
}

int
compute_image_dots(int w, int h, unsigned char *bitmap)
{
    int	dots = 0;
    int x, y, bpl;

    bpl = (w + 7) / 8;
    for (y = 0; y < h; ++y)
	for (x = 0; x < bpl; ++x)
	    dots += BlackOnes[ bitmap[y*bpl + x] ];
    return dots;
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
    int	bpl = (w + 7) / 8;
    unsigned char *plane[4], *bitmaps[4][1];
    struct jbg_enc_state se[4]; 

    RealWidth = w;
    for (i = 0; i < 4; ++i)
    {
	plane[i] = malloc(bpl * h);
	if (!plane[i]) error(3, "Cannot allocate space for bit plane\n");
	chain[i] = NULL;
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

	Dots[i] = compute_image_dots(w, h, plane[i]);

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
    for (i = 0; i < 4; ++i)
	chain[i] = NULL;

    for (i = 0; i < 4; ++i)
    {
	Dots[i] = compute_image_dots(w, h, plane[i]);

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

    return 0;
}

int
pbm_page(unsigned char *buf, int w, int h, FILE *ofp)
{
    BIE_CHAIN		*chain = NULL;
    unsigned char	*bitmaps[1];
    struct jbg_enc_state se; 
    int	x, y;
    int	bpl, bpl16;

    RealWidth = w;
    //if (Model == MODEL_HP1020)
	//w = (w + 127) & ~127;

    Dots[3] = 0;
    bpl = (w + 7) / 8;
    //if (Model == MODEL_2300DL)
	bpl16 = bpl;
    //else
	//bpl16 = (bpl + 15) & ~15;

    if (SaveToner)
    {
	for (y = 0; y < h; y += 2)
	    for (x = 0; x < bpl16; ++x)
		buf[y*bpl16 + x] &= 0x55;
	for (y = 1; y < h; y += 2)
	    for (x = 0; x < bpl16; ++x)
		buf[y*bpl16 + x] &= 0xaa;
    }

    Dots[3] = compute_image_dots(w, h, buf);

    *bitmaps = buf;

    jbg_enc_init(&se, w, h, 1, bitmaps, output_jbig, &chain);
    jbg_enc_options(&se, JbgOptions[0], JbgOptions[1],
			JbgOptions[2], JbgOptions[3], JbgOptions[4]);
    jbg_enc_out(&se);
    jbg_enc_free(&se);

    write_page(&chain, NULL, NULL, NULL, ofp);

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

	    plane[i] = malloc(bpl * h);
	    if (!plane[i])
		error(1, "Can't allocate plane buffer\n");

	    rc = read_and_clip_image(plane[i],
					rawBpl, rightBpl, 8, bpl, h, bpl, ifp);
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

		for (p = plane[i], e = p + bpl*h; p < e; ++p)
		    if (*p)
		    {
			AnyColor |= 1<<i;
			break;
		    }
	    }

	    if (Duplex == DMDUPLEX_LONGEDGE && (PageNum & 1) == 0)
		rotate_bytes_180(plane[i], plane[i] + bpl * h - 1, Mirror1);
	    if (Duplex == DMDUPLEX_MANUALLONG && (PageNum & 1) == 0)
		rotate_bytes_180(plane[i], plane[i] + bpl * h - 1, Mirror1);
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

	//switch (Model)
	//{
	//case MODEL_2300DL:	bpl16 = bpl; break;
	//case MODEL_HP1020:	bpl16 = (bpl + 15) & ~15; break;
	//default:		error(1, "Bad model %d\n", Model); break;
	//}
	bpl16 = bpl;

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
    bpl16 = bpl;
    //switch (Model)
    //{
    //case MODEL_2300DL:	bpl16 = bpl; break;
    //case MODEL_HP1020:	bpl16 = (bpl + 15) & ~15; break;
    //default:		error(1, "Bad model %d\n", Model); break;
    //}

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
		    "cd:g:n:m:p:r:s:tu:l:z:L:ABPJ:S:U:X:D:V?h")) != EOF)
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
	case 'm':	if (isalpha(optarg[0]))
			    MediaStr = optarg;
			else
			    MediaCode = atoi(optarg);
			break;
	case 'n':	Copies = atoi(optarg); break;
	case 'p':	if (strstr(optarg,"in") || strstr(optarg, "mm"))
			    PaperStr = optarg;
			else
			    PaperCode = atoi(optarg);
			break;
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
	case 'z':	Model = atoi(optarg);
			if (Model < 0 || Model > MODEL_LAST)
			    error(1, "Illegal value '%s' for -z\n", optarg);
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

    if (0)//Model == MODEL_HP1020)
    {
	Bpp = ResX / 600;
	ResX = 600;
	if (SaveToner)
	{
	    SaveToner = 0;
	    EconoMode = 1;
	}
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
	int	media;
	int	rc;

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

	/*
	 *  Manual Pause
	 */
	if (0)//Model == MODEL_HP1020)
	    load_tray2(stdout);

	fseek(EvenPages, SeekMedia, 0L);
	//media = be32(DMMEDIA_LETTERHEAD);
	rc = fwrite(&media, 1, sizeof(4), EvenPages);
	if (rc == 0) error(1, "fwrite(1): rc == 0!\n");

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
