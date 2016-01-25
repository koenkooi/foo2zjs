/*
 * Manifest constants for the ZjStream protocol
 *
 * I'm told that a lot of this stuff came from a file called "zjrca.h".
 * But a Google search does not turn up that file. I added my own
 * improvements and missing pieces. -Rick
 */

#include <inttypes.h>
typedef uint32_t	DWORD;
typedef uint16_t	WORD;
typedef uint8_t		BYTE;

typedef enum {
    ZJT_START_DOC	= 0,
    ZJT_END_DOC		= 1,
    ZJT_START_PAGE	= 2,
    ZJT_END_PAGE	= 3,
    ZJT_JBIG_BIH	= 4,	// Bi-level Image Header
    ZJT_JBIG_BID	= 5,	// Bi-level Image Data blocks
    ZJT_END_JBIG	= 6,
    ZJT_SIGNATURE	= 7,
    ZJT_RAW_IMAGE	= 8,	// full uncompressed plane follows
    ZJT_START_PLANE	= 9,
    ZJT_END_PLANE	=10,

    ZJT_2600N_PAUSE	=11,
    ZJT_2600N		=12,	// hplj2600n 

    ZJT_ZX_0x0d		=0x0d,
    ZJT_ZX_0x0e		=0x0e,
    ZJT_ZX_0x0f		=0x0f
} ZJ_TYPE;

typedef struct _ZJ_HEADER {
    DWORD size;           /*  total record size, includes sizeof(ZJ_HEADER) */
    DWORD type;           /*  ZJ_TYPE */
    DWORD items;          /*  use varies by Type, e.g. item count */
    WORD reserved;        /*  later to be sumcheck or CRC */
    WORD signature;       /*  'ZZ' */
} ZJ_HEADER;

typedef enum {
/*
 * 0x0000-0x7FFF : Zenographics reserved
 */
    /* for START_DOC */
    ZJI_PAGECOUNT	= 0,	// number of ZJT_START_PAGE / ZJT_END_PAGE 
				// pairs, if known
    ZJI_DMCOLLATE	= 1,	// from DEVMODE
    ZJI_DMDUPLEX	= 2,	// from DEVMODE
  
    /* for START_PAGE */
    ZJI_DMPAPER		= 3,	// from DEVMODE
    ZJI_DMCOPIES	= 4,	// from DEVMODE
    ZJI_DMDEFAULTSOURCE	= 5,	// from DEVMODE (DMBIN?)
    ZJI_DMMEDIATYPE	= 6,	// from DEVMODE
    ZJI_NBIE		= 7,	// number of Bi-level Image Entities,
				//  e.g. 1 for monochrome, 4 for color
    ZJI_RESOLUTION_X	= 8,	// dots per inch
    ZJI_RESOLUTION_Y	= 9,
    ZJI_OFFSET_X	=10,	// upper left corner
    ZJI_OFFSET_Y	=11,
    ZJI_RASTER_X	=12,	// raster dimensions
    ZJI_RASTER_Y	=13,
  
    ZJI_COLLATE		=14,	// asks for collated copies
    ZJI_QUANTITY	=15,	// copy count

    ZJI_VIDEO_BPP	=16,	// video bits per pixel
    ZJI_VIDEO_X		=17,	// video dimensions (if different than raster)
    ZJI_VIDEO_Y		=18,
    ZJI_INTERLACE	=19,	// 0 or 1
    ZJI_PLANE		=20,	// Plane number, 1=C, 2=M, 3=Y, 4=K
    ZJI_PALETTE		=21,	// translation table (dimensions in item type)
    ZJI_RET		=22,	// HP's Resolution Enhancement Technology
    ZJI_ECONOMODE	=23,	// HP's "EconoMode", 0=OFF, 1=ON

    ZJI_BITMAP_TYPE	=0x65,	// hp2600: ?
    ZJI_JBIG_BIH	=0x66,	// Bi-level Image Header
    ZJI_BITMAP_PIXELS	=0x68,	// hp2600: ?
    ZJI_INCRY		=0x69,	// Incremental Y
    ZJI_BITMAP_BPP	=0x6a,	// hp2600: ?
    ZJI_BITMAP_STRIDE	=0x6b,	// hp2600: ?

    ZJI_ZX_0x6c		=0x6c,	// ZX: color order ???
				// cmyk=04030201, rgb=0b0a09, mono=0c
    ZJI_ZX_COLOR_OPT	=0x6e,	// ZX: Color Options. 1=mono, 3=rgb, 4=cmyk
    ZJI_ZX_0x6f		=0x6f,	// ZX: ???

    ZJI_PAD		=99,	// bogus item type for padding stream

/*
 * 0x8000-0x80FF : Item tags for QMS specific features.
 */ 
    ZJI_QMS_FINEMODE	= 0x8000,	// for 668, 671
    ZJI_QMS_OUTBIN	= 0x8001,	// for 671 output bin

/*
 * 0x8100-0x81FF : Item tags for Minolta specific features.
 */
    /* for START_DOC */
    ZJI_MINOLTA_USERNAME	= 0x810e,	// C string
    ZJI_MINOLTA_FILENAME	= 0x8115,	// C string
    // Also 8100-810F; meanings unknown

    /* for START_PAGE */
    ZJI_MINOLTA_PAGE_NUMBER	= 0x8110,	// Number of this page
    // Also 8111, 8116-811D; meanings unknown
    ZJI_MINOLTA_CUSTOM_X	= 0x8113,	// Custom Page Width
    ZJI_MINOLTA_CUSTOM_Y	= 0x8114,	// Custom Page Height

    /* for END_PAGE */
    // Also 8101, 8110; meanings unknown

/*
 * 0x8200-0x82FF : Item tags for the next OEM specific features.
 */
    ZJI_HP_CDOTS		= 0x8200,	// Cyan Dots?
    ZJI_HP_MDOTS		= 0x8201,	// Magenta Dots?
    ZJI_HP_YDOTS		= 0x8202,	// Yellow Dots?
    ZJI_HP_KDOTS		= 0x8203,	// Black Dots?
    ZJI_HP_CWHITE		= 0x8204,	// Cyan White Dots?
    ZJI_HP_MWHITE		= 0x8205,	// Magenta White Dots?
    ZJI_HP_YWHITE		= 0x8206,	// Yellow White Dots?
    ZJI_HP_KWHITE		= 0x8207,	// Black White Dots?

    ZJI_LAST
} ZJ_ITEM;

typedef enum
{
    DMDUPLEX_OFF	= 1,
    DMDUPLEX_LONGEDGE	= 2,
    DMDUPLEX_SHORTEDGE	= 3,
    DMDUPLEX_MANUALLONG	= 4,
    DMDUPLEX_MANUALSHORT= 5
} DMDUPLEX;

typedef enum {
    DMBIN_UPPER		= 1,
    DMBIN_ONLYONE	= 1,
    DMBIN_LOWER		= 2,
    DMBIN_MIDDLE	= 3,
    DMBIN_MANUAL	= 4,
    DMBIN_ENVELOPE	= 5,
    DMBIN_ENVMANUAL	= 6,
    DMBIN_AUTO		= 7,
    DMBIN_TRACTOR	= 8,
    DMBIN_SMALLFMT	= 9,
    DMBIN_LARGEFMT	=10,
    DMBIN_LARGECAPACITY	=11,
    DMBIN_CASSETTE	=14,
    DMBIN_FORMSOURCE	=15
} DM_BIN;

typedef enum {
    DMMEDIA_STANDARD	= 1,	// Standard paper
    DMMEDIA_TRANSPARENCY= 2,	// Transparency
    DMMEDIA_GLOSSY	= 3,	// Glossy paper
    DMMEDIA_USER	= 4,	// Device-specific media start here

    DMMEDIA_ENVELOPE	= 0x101, // Envelope
    DMMEDIA_LETTERHEAD	= 0x103, // Letterhead
    DMMEDIA_THICK_STOCK	= 0x105, // Thick Stock
    DMMEDIA_POSTCARD	= 0x106, // Postcard
    DMMEDIA_LABELS	= 0x107, // Labels
} DMMEDIA;

typedef enum {
    DMCOLOR_MONOCHROME	= 1,
    DMCOLOR_COLOR	= 2,
} DMCOLOR;

typedef enum {
    DMORIENT_PORTRAIT	= 1,
    DMORIENT_LANDSCAPE	= 2,
} DMORIENT;

typedef enum {
    DMPAPER_LETTER	= 1,	// Letter, 8 1/2- by 11-inches
    DMPAPER_LETTERSMALL	= 2,	// Letter Small, 8 1/2- by 11-inches
    DMPAPER_TABLOID	= 3,	// Tabloid, 11- by 17-inches
    DMPAPER_LEDGER	= 4,	// Ledger, 17- by 11-inches
    DMPAPER_LEGAL	= 5,	// Legal, 8 1/2- by 14-inches
    DMPAPER_STATEMENT	= 6,	// Statement, 5 1/2- by 8 1/2-inches
    DMPAPER_EXECUTIVE	= 7,	// Executive, 7 1/4- by 10 1/2-inches
    DMPAPER_A3		= 8,	// A3 sheet, 297- by 420-millimeters
    DMPAPER_A4		= 9,	// A4 Sheet, 210- by 297-millimeters
    DMPAPER_A4SMALL	=10,	// A4 small sheet, 210- by 297-millimeters
    DMPAPER_A5		=11,	// A5 sheet, 148- by 210-millimeters
    DMPAPER_B4		=12,	// B4 sheet, 250- by 354-millimeters
    DMPAPER_B5		=13,	// B5 sheet, 182- by 257-millimeter paper
    DMPAPER_FOLIO	=14,	// Folio, 8 1/2- by 13-inch paper
    DMPAPER_QUARTO	=15,	// Quarto, 215- by 275-millimeter paper
    DMPAPER_10X14	=16,	// 10- by 14-inch sheet
    DMPAPER_11X17	=17,	// 11- by 17-inch sheet
    DMPAPER_NOTE	=18,	// Note, 8 1/2- by 11-inches
    DMPAPER_ENV_9	=19,	// #9 Envelope, 3 7/8- by 8 7/8-inches
    DMPAPER_ENV_10	=20,	// #10 Envelope, 4 1/8- by 9 1/2-inches
    DMPAPER_ENV_11	=21,	// #11 Envelope, 4 1/2- by 10 3/8-inches
    DMPAPER_ENV_12	=22,	// #12 Envelope, 4 3/4- by 11-inches
    DMPAPER_ENV_14	=23,	// #14 Envelope, 5- by 11 1/2-inches
    DMPAPER_CSHEET	=24,	// C Sheet, 17- by 22-inches
    DMPAPER_DSHEET	=25,	// D Sheet, 22- by 34-inches
    DMPAPER_ESHEET	=26,	// E Sheet, 34- by 44-inches
    DMPAPER_ENV_DL	=27,	// DL Envelope, 110- by 220-millimeters
    DMPAPER_ENV_C5	=28,	// C5 Envelope, 162- by 229-millimeters
    DMPAPER_ENV_C3	=29,	// C3 Envelope, 324- by 458-millimeters
    DMPAPER_ENV_C4	=30,	// C4 Envelope, 229- by 324-millimeters
    DMPAPER_ENV_C6	=31,	// C6 Envelope, 114- by 162-millimeters
    DMPAPER_ENV_C65	=32,	// C65 Envelope, 114- by 229-millimeters
    DMPAPER_ENV_B4	=33,	// B4 Envelope, 250- by 353-millimeters
    DMPAPER_ENV_B5	=34,	// B5 Envelope, 176- by 250-millimeters
    DMPAPER_ENV_B6	=35,	// B6 Envelope, 176- by 125-millimeters
    DMPAPER_ENV_ITALY	=36,	// Italy Envelope, 110- by 230-millimeters
    DMPAPER_ENV_MONARCH	=37,	// Monarch Envelope, 3 7/8- by 7 1/2-inches
    DMPAPER_ENV_PERSONAL=38,	// 6 3/4 Envelope, 3 5/8- by 6 1/2-inches
    DMPAPER_FANFOLD_US	=39,	// US Std Fanfold, 14 7/8- by 11-inches
    DMPAPER_FANFOLD_STD_GERMAN	=40,	// German Std Fanfold, 8 1/2 x 12 in
    DMPAPER_FANFOLD_LGL_GERMAN	=41,	// German Legal Fanfold, 8 1/2 x 13 in

    DMPAPER_ISO_B4		   =42,	// B4 (ISO) 250 x 353 mm
    DMPAPER_JAPANESE_POSTCARD	   =43,	// Japanese Postcard 100 x 148 mm
    DMPAPER_9X11		   =44,	// 9 x 11 in
    DMPAPER_10X11		   =45,	// 10 x 11 in
    DMPAPER_15X11		   =46,	// 15 x 11 in
    DMPAPER_ENV_INVITE		   =47,	// Envelope Invite 220 x 220 mm
    DMPAPER_RESERVED_48		   =48,	// RESERVED--DO NOT USE
    DMPAPER_RESERVED_49		   =49,	// RESERVED--DO NOT USE
    DMPAPER_LETTER_EXTRA	   =50,	// Letter Extra 9 \275 x 12 in
    DMPAPER_LEGAL_EXTRA		   =51,	// Legal Extra 9 \275 x 15 in
    DMPAPER_TABLOID_EXTRA	   =52,	// Tabloid Extra 11.69 x 18 in
    DMPAPER_A4_EXTRA		   =53,	// A4 Extra 9.27 x 12.69 in
    DMPAPER_LETTER_TRANSVERSE	   =54,	// Letter Transverse 8 \275 x 11 in
    DMPAPER_A4_TRANSVERSE	   =55,	// A4 Transverse 210 x 297 mm
    DMPAPER_LETTER_EXTRA_TRANSVERSE=56,	// Letter Extra Transverse 9\275 x 12 in
    DMPAPER_A_PLUS		   =57,	// SuperA/SuperA/A4 227 x 356 mm
    DMPAPER_B_PLUS		   =58,	// SuperB/SuperB/A3 305 x 487 mm
    DMPAPER_LETTER_PLUS		   =59,	// Letter Plus 8.5 x 12.69 in
    DMPAPER_A4_PLUS		   =60,	// A4 Plus 210 x 330 mm
    DMPAPER_A5_TRANSVERSE	   =61,	// A5 Transverse 148 x 210 mm
    DMPAPER_B5_TRANSVERSE	   =62,	// B5 (JIS) Transverse 182 x 257 mm
    DMPAPER_A3_EXTRA		   =63,	// A3 Extra 322 x 445 mm
    DMPAPER_A5_EXTRA		   =64,	// A5 Extra 174 x 235 mm
    DMPAPER_B5_EXTRA		   =65,	// B5 (ISO) Extra 201 x 276 mm
    DMPAPER_A2			   =66,	// A2 420 x 594 mm
    DMPAPER_A3_TRANSVERSE	   =67,	// A3 Transverse 297 x 420 mm
    DMPAPER_A3_EXTRA_TRANSVERSE	   =68,	// A3 Extra Transverse 322 x 445 mm
} DMPAPER;

typedef enum {
    ZJIT_UINT32	= 1,	// unsigned integer
    ZJIT_INT32	= 2,    // signed integer
    ZJIT_STRING	= 3,    // byte string, NUL-terminated, DWORD-aligned
    ZJIT_BYTELUT= 4     // DWORD count followed by that many byte entries
} ZJ_ITEM_TYPE;

typedef struct _ZJ_ITEM_HEADER {
    DWORD size;		// total record size, includes sizeof(ZJ_ITEM_HEADER)
    WORD  item;		//  ZJ_ITEM
    BYTE  type;		//  ZJ_ITEM_TYPE
    BYTE  param;	//  general use
} ZJ_ITEM_HEADER;

typedef struct _ZJ_ITEM_UINT32 {
    ZJ_ITEM_HEADER	header;
    DWORD		value;
} ZJ_ITEM_UINT32;

typedef struct _ZJ_ITEM_INT32 {
    ZJ_ITEM_HEADER	header;
    int32_t		value;
} ZJ_ITEM_INT32;

typedef union _SWAP_32{
    char		byte[sizeof(uint32_t)];
    uint32_t		dword;
} SWAP_32;

typedef union _SWAP_16{
    char		byte[sizeof(uint16_t)];
    uint16_t		word;
} SWAP_16;

static inline uint32_t
be32(uint32_t dword)
{
    SWAP_32	swap;
    uint32_t	probe = 1;

    if (((char *)&probe)[0] == 1)
    {
	swap.byte[3] = (( SWAP_32 )dword).byte[0]; 
	swap.byte[2] = (( SWAP_32 )dword).byte[1];
	swap.byte[1] = (( SWAP_32 )dword).byte[2]; 
	swap.byte[0] = (( SWAP_32 )dword).byte[3];
	return swap.dword;
    }
    else
	return dword;
}

static inline uint16_t
be16(uint16_t word)
{
    SWAP_16	swap;
    uint16_t	probe = 1;

    if (((char *)&probe)[0] == 1)
    {
	swap.byte[1] = (( SWAP_16 )word).byte[0]; 
	swap.byte[0] = (( SWAP_16 )word).byte[1];
	return swap.word;
    }
    else
	return word;
}
