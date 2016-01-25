
#include <inttypes.h>
typedef uint32_t	DWORD;
typedef uint16_t	WORD;
typedef uint8_t		BYTE;

/*
 * JBIG BIH.  But note that Oak uses it little endian.
 */
typedef struct
{
    DWORD	opt1;
    DWORD	xd;	// Oak has this little endian
    DWORD	yd;	// Oak has this little endian
    DWORD	l0;	// Oak has this little endian
    DWORD	opt2;
} OAKBIH;

/*
 * Oak record header.  Every record starts with one of these and
 * the entire record is always padded out to a multiple of 16 bytes.
 */
typedef struct
{
    char	magic[4];	
		#define OAK_HDR_MAGIC	"OAKT"
    DWORD	len;		// Total length of record including this header
    DWORD	type;		// Record type
} OAK_HDR;

/*
 * Note that the upper nibble of the type number encodes the class
 *
 * 0x	- start/end doc
 * 1x	- start/end page
 * 2x	- page parameters
 * 3x	- image data
 */

/************************************************************************/
/*	0x	-	start/end doc					*/
/************************************************************************/
/*
 * First record in file.
 *
 * No idea what the payload means yet.  My guess is username.
 */
#define	OAK_TYPE_OTHER	0x0D
typedef struct
{
    WORD	unk;		// Always 1
    char	string[64];	// "OTHER" padded with 0's
    // WORD	pad;		// "PAD_PAD_" as needed.
} OAK_OTHER;
//typedef OAK_OTHER	HDR_0D;

/*
 * date/time record
 */
#define	OAK_TYPE_TIME	0x0C
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
    // DWORD	pad;
} OAK_TIME;
//typedef OAK_TIME	HDR_0C;

/*
 * Filename record
 */
#define	OAK_TYPE_FILENAME	0x0A
typedef struct
{
    char	string[64];	// "OTHER" padded with 0's
} OAK_FILENAME;

/*
 * Duplex record
 */
#define	OAK_TYPE_DUPLEX		0x0F
typedef struct
{
    DWORD	duplex;		// Duplex
    DWORD	short_edge;
} OAK_DUPLEX;

/*
 * Driver record
 */
#define	OAK_TYPE_DRIVER		0x1F
typedef struct
{
    char	string[36];	// "OTHER" padded with 0's
} OAK_DRIVER;

/*
 * End of document
 */
#define	OAK_TYPE_END_DOC	0x0B

/************************************************************************/
/*	1x	-	start/end page					*/
/************************************************************************/
#define	OAK_TYPE_START_PAGE	0x14	// No arguments
#define	OAK_TYPE_START_IMAGE	0x15	// No arguments
#define	OAK_TYPE_END_IMAGE	0x17	// No arguments
#define	OAK_TYPE_END_PAGE	0x18	// WORD argument (0)
/************************************************************************/
/*	2x	-	page parameters					*/
/************************************************************************/
#define	OAK_TYPE_SOURCE		0x28	// DWORD argument: paper source
				#define OAK_SOURCE_TRAY1	1
				#define OAK_SOURCE_TRAY2	2
				#define OAK_SOURCE_MANUAL	4
				#define OAK_SOURCE_AUTO		7

#define	OAK_TYPE_MEDIA		0x29
typedef struct
{
    BYTE	media;		// Media code
				#define OAK_MEDIA_AUTO		0
				#define OAK_MEDIA_PLAIN		1
				#define OAK_MEDIA_PREPRINTED	2
				#define OAK_MEDIA_LETTERHEAD	3
				#define OAK_MEDIA_GRAYTRANS	4
				#define OAK_MEDIA_PREPUNCHED	5
				#define OAK_MEDIA_LABELS	6
				#define OAK_MEDIA_BOND		7
				#define OAK_MEDIA_RECYCLED	8
				#define OAK_MEDIA_COLOR		9
				#define OAK_MEDIA_CARDSTOCK	10
				#define OAK_MEDIA_HEAVY		11
				#define OAK_MEDIA_ENVELOPE	12
				#define OAK_MEDIA_LIGHT		13
				#define OAK_MEDIA_TOUGH		14
    BYTE	unk8[3];	// Unknown, 2, 0, 0
    char	string[64];	// Unknown string, padd with blanks
} OAK_MEDIA;

#define	OAK_TYPE_COPIES		0x2A
typedef struct
{
    DWORD	copies;		// Number of copies
    DWORD	duplex;		// Duplex
} OAK_COPIES;

#define	OAK_TYPE_PAPER		0x2B
typedef struct
{
    DWORD	paper;		// Paper code
	#define OAK_PAPER_LETTER	1    // 8.5 x 11in
	#define OAK_PAPER_LEGAL		5    // 8.5 x 14in
	#define OAK_PAPER_EXECUTIVE	7    // 7.25 x 10.5in
	#define OAK_PAPER_A4		9    // 210 x 297mm
	#define OAK_PAPER_A5		11   // 148 x 210mm
	#define OAK_PAPER_B5_JIS	13   // 182 x 257mm
	#define OAK_PAPER_ENV_10	20   // 4.125 x 9.5in
	#define OAK_PAPER_ENV_DL	27   // 110 x 220mm
	#define OAK_PAPER_ENV_C5	28   // 162 x 229mm
	#define OAK_PAPER_ENV_B5	34   // 176 x 250mm
	#define OAK_PAPER_ENV_MONARCH	37   // 3.875 x 7.5in
	#define OAK_PAPER_B5_ISO	257  // 176 x 250mm
	#define OAK_PAPER_EXECUTIVE_JIS	258  // 8 x 13in
	#define OAK_PAPER_16K		93   // 7.75 x 10.75in
	#define OAK_PAPER_DOUBLE_POSTCARD 69 // 5.8 x 7.9in
	#define OAK_PAPER_POSTCARD	43   // 4.25 x 6in
	#define OAK_PAPER_CUSTOM	256  // user defined
    DWORD	w1200;		// Paper width at 1200 DPI
    DWORD	h1200;		// Paper height at 1200 DPI
    DWORD	unk;		// unknown, 0
} OAK_PAPER;
/************************************************************************/
/*	3x	-	JBIG image data					*/
/************************************************************************/
#define OAK_TYPE_IMAGE_COLOR	0x32
#define OAK_TYPE_IMAGE_MONO	0x33
typedef struct
{
    DWORD	unk0;		// Likely x offset
    DWORD	unk1;		// Likely y offset
    DWORD	w;		// width of (clipped?) image
    DWORD	h;		// height of (clipped?) image
    DWORD	resx;		// X resolution in DPI
    DWORD	resy;		// Y resolution in DPI
    DWORD	nbits;		// bits per plane, 1 or 2
    char	unk[16];	// always 0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f
} OAK_IMAGE_PLANE;

typedef struct
{
    OAK_IMAGE_PLANE	plane;
} OAK_IMAGE_MONO;

typedef struct
{
    OAK_IMAGE_PLANE	plane[4];
} OAK_IMAGE_COLOR;

#define OAK_TYPE_IMAGE_DATA	0x3C
typedef struct
{
    OAKBIH	bih;		// Little-endian JBIG BIH
    DWORD	datalen;	// Length of actual image data
    DWORD	padlen;		// Padded length of image data
    DWORD	unk1C;		// unknown, 000
    DWORD	y;		// Y offset of this chunk
    DWORD	plane;		// 0=, 1=, 2=, 3=K
    DWORD	subplane;	// 0 or 1
    // DWORD	pad[2];
} OAK_IMAGE_DATA;
