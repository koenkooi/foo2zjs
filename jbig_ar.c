/*
 *  Arithmetic encoder and decoder of the portable JBIG
 *  compression library
 *
 *  Markus Kuhn -- http://www.cl.cam.ac.uk/~mgk25/jbigkit/
 *
 *  This module implements a portable standard C arithmetic encoder
 *  and decoder used by the JBIG lossless bi-level image compression
 *  algorithm as specified in International Standard ISO 11544:1993
 *  and ITU-T Recommendation T.82.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 *  If you want to use this program under different license conditions,
 *  then contact the author for an arrangement.
 */

#include <assert.h>
#include "jbig_ar.h"

/*
 *  Probability estimation tables for the arithmetic encoder/decoder
 *  given by ITU T.82 Table 24.
 */

static short lsztab[113] = {
  0x5a1d, 0x2586, 0x1114, 0x080b, 0x03d8, 0x01da, 0x00e5, 0x006f,
  0x0036, 0x001a, 0x000d, 0x0006, 0x0003, 0x0001, 0x5a7f, 0x3f25,
  0x2cf2, 0x207c, 0x17b9, 0x1182, 0x0cef, 0x09a1, 0x072f, 0x055c,
  0x0406, 0x0303, 0x0240, 0x01b1, 0x0144, 0x00f5, 0x00b7, 0x008a,
  0x0068, 0x004e, 0x003b, 0x002c, 0x5ae1, 0x484c, 0x3a0d, 0x2ef1,
  0x261f, 0x1f33, 0x19a8, 0x1518, 0x1177, 0x0e74, 0x0bfb, 0x09f8,
  0x0861, 0x0706, 0x05cd, 0x04de, 0x040f, 0x0363, 0x02d4, 0x025c,
  0x01f8, 0x01a4, 0x0160, 0x0125, 0x00f6, 0x00cb, 0x00ab, 0x008f,
  0x5b12, 0x4d04, 0x412c, 0x37d8, 0x2fe8, 0x293c, 0x2379, 0x1edf,
  0x1aa9, 0x174e, 0x1424, 0x119c, 0x0f6b, 0x0d51, 0x0bb6, 0x0a40,
  0x5832, 0x4d1c, 0x438e, 0x3bdd, 0x34ee, 0x2eae, 0x299a, 0x2516,
  0x5570, 0x4ca9, 0x44d9, 0x3e22, 0x3824, 0x32b4, 0x2e17, 0x56a8,
  0x4f46, 0x47e5, 0x41cf, 0x3c3d, 0x375e, 0x5231, 0x4c0f, 0x4639,
  0x415e, 0x5627, 0x50e7, 0x4b85, 0x5597, 0x504f, 0x5a10, 0x5522,
  0x59eb
};

static unsigned char nmpstab[113] = {
    1,   2,   3,   4,   5,   6,   7,   8,
    9,  10,  11,  12,  13,  13,  15,  16,
   17,  18,  19,  20,  21,  22,  23,  24,
   25,  26,  27,  28,  29,  30,  31,  32,
   33,  34,  35,   9,  37,  38,  39,  40,
   41,  42,  43,  44,  45,  46,  47,  48,
   49,  50,  51,  52,  53,  54,  55,  56,
   57,  58,  59,  60,  61,  62,  63,  32,
   65,  66,  67,  68,  69,  70,  71,  72,
   73,  74,  75,  76,  77,  78,  79,  48,
   81,  82,  83,  84,  85,  86,  87,  71,
   89,  90,  91,  92,  93,  94,  86,  96,
   97,  98,  99, 100,  93, 102, 103, 104,
   99, 106, 107, 103, 109, 107, 111, 109,
  111
};

/*
 * least significant 7 bits (mask 0x7f) of nlpstab[] contain NLPS value,
 * most significant bit (mask 0x80) contains SWTCH bit
 */
static unsigned char nlpstab[113] = {
  129,  14,  16,  18,  20,  23,  25,  28,
   30,  33,  35,   9,  10,  12, 143,  36,
   38,  39,  40,  42,  43,  45,  46,  48,
   49,  51,  52,  54,  56,  57,  59,  60,
   62,  63,  32,  33, 165,  64,  65,  67,
   68,  69,  70,  72,  73,  74,  75,  77,
   78,  79,  48,  50,  50,  51,  52,  53,
   54,  55,  56,  57,  58,  59,  61,  61,
  193,  80,  81,  82,  83,  84,  86,  87,
   87,  72,  72,  74,  74,  75,  77,  77,
  208,  88,  89,  90,  91,  92,  93,  86,
  216,  95,  96,  97,  99,  99,  93, 223,
  101, 102, 103, 104,  99, 105, 106, 107,
  103, 233, 108, 109, 110, 111, 238, 112,
  240
};

/*
 * The next functions implement the arithmedic encoder and decoder
 * required for JBIG. The same algorithm is also used in the arithmetic
 * variant of JPEG.
 */

/* marker codes */
#define MARKER_STUFF    0x00
#define MARKER_ESC      0xff

void arith_encode_init(struct jbg_arenc_state *s, int reuse_st)
{
  int i;
  
  if (!reuse_st)
    for (i = 0; i < 4096; s->st[i++] = 0) ;
  s->c = 0;
  s->a = 0x10000L;
  s->sc = 0;
  s->ct = 11;
  s->buffer = -1;    /* empty */
  
  return;
}


void arith_encode_flush(struct jbg_arenc_state *s)
{
  unsigned long temp;

  /* find the s->c in the coding interval with the largest
   * number of trailing zero bits */
  if ((temp = (s->a - 1 + s->c) & 0xffff0000L) < s->c)
    s->c = temp + 0x8000;
  else
    s->c = temp;
  /* send remaining bytes to output */
  s->c <<= s->ct;
  if (s->c & 0xf8000000L) {
    /* one final overflow has to be handled */
    if (s->buffer >= 0) {
      s->byte_out(s->buffer + 1, s->file);
      if (s->buffer + 1 == MARKER_ESC)
	s->byte_out(MARKER_STUFF, s->file);
    }
    /* output 0x00 bytes only when more non-0x00 will follow */
    if (s->c & 0x7fff800L)
      for (; s->sc; --s->sc)
	s->byte_out(0x00, s->file);
  } else {
    if (s->buffer >= 0)
      s->byte_out(s->buffer, s->file); 
    /* T.82 figure 30 says buffer+1 for the above line! Typo? */
    for (; s->sc; --s->sc) {
      s->byte_out(0xff, s->file);
      s->byte_out(MARKER_STUFF, s->file);
    }
  }
  /* output final bytes only if they are not 0x00 */
  if (s->c & 0x7fff800L) {
    s->byte_out((s->c >> 19) & 0xff, s->file);
    if (((s->c >> 19) & 0xff) == MARKER_ESC)
      s->byte_out(MARKER_STUFF, s->file);
    if (s->c & 0x7f800L) {
      s->byte_out((s->c >> 11) & 0xff, s->file);
      if (((s->c >> 11) & 0xff) == MARKER_ESC)
	s->byte_out(MARKER_STUFF, s->file);
    }
  }

  return;
}


void arith_encode(struct jbg_arenc_state *s, int cx, int pix) 
{
  register unsigned lsz, ss;
  register unsigned char *st;
  long temp;

  assert(cx >= 0 && cx < 4096);
  st = s->st + cx;
  ss = *st & 0x7f;
  assert(ss < 113);
  lsz = lsztab[ss];

#if 0
  fprintf(stderr, "pix = %d, cx = %d, mps = %d, st = %3d, lsz = 0x%04x, "
	  "a = 0x%05lx, c = 0x%08lx, ct = %2d, buf = 0x%02x\n",
	  pix, cx, !!(s->st[cx] & 0x80), ss, lsz, s->a, s->c, s->ct,
	  s->buffer);
#endif

  if (((pix << 7) ^ s->st[cx]) & 0x80) {
    /* encode the less probable symbol */
    if ((s->a -= lsz) >= lsz) {
      /* If the interval size (lsz) for the less probable symbol (LPS)
       * is larger than the interval size for the MPS, then exchange
       * the two symbols for coding efficiency, otherwise code the LPS
       * as usual: */
      s->c += s->a;
      s->a = lsz;
    }
    /* Check whether MPS/LPS exchange is necessary
     * and chose next probability estimator status */
    *st &= 0x80;
    *st ^= nlpstab[ss];
  } else {
    /* encode the more probable symbol */
    if ((s->a -= lsz) & 0xffff8000L)
      return;   /* A >= 0x8000 -> ready, no renormalization required */
    if (s->a < lsz) {
      /* If the interval size (lsz) for the less probable symbol (LPS)
       * is larger than the interval size for the MPS, then exchange
       * the two symbols for coding efficiency: */
      s->c += s->a;
      s->a = lsz;
    }
    /* chose next probability estimator status */
    *st &= 0x80;
    *st |= nmpstab[ss];
  }

  /* renormalization of coding interval */
  do {
    s->a <<= 1;
    s->c <<= 1;
    --s->ct;
    if (s->ct == 0) {
      /* another byte is ready for output */
      temp = s->c >> 19;
      if (temp & 0xffffff00L) {
	/* handle overflow over all buffered 0xff bytes */
	if (s->buffer >= 0) {
	  ++s->buffer;
	  s->byte_out(s->buffer, s->file);
	  if (s->buffer == MARKER_ESC)
	    s->byte_out(MARKER_STUFF, s->file);
	}
	for (; s->sc; --s->sc)
	  s->byte_out(0x00, s->file);
	s->buffer = temp & 0xff;  /* new output byte, might overflow later */
	assert(s->buffer != 0xff);
	/* can s->buffer really never become 0xff here? */
      } else if (temp == 0xff) {
	/* buffer 0xff byte (which might overflow later) */
	++s->sc;
      } else {
	/* output all buffered 0xff bytes, they will not overflow any more */
	if (s->buffer >= 0)
	  s->byte_out(s->buffer, s->file);
	for (; s->sc; --s->sc) {
	  s->byte_out(0xff, s->file);
	  s->byte_out(MARKER_STUFF, s->file);
	}
	s->buffer = temp;   /* buffer new output byte (can still overflow) */
      }
      s->c &= 0x7ffffL;
      s->ct = 8;
    }
  } while (s->a < 0x8000);
 
  return;
}


void arith_decode_init(struct jbg_ardec_state *s, int reuse_st)
{
  int i;
  
  if (!reuse_st)
    for (i = 0; i < 4096; s->st[i++] = 0) ;
  s->c = 0;
  s->a = 1;
  s->ct = 0;
  s->startup = 1;
  s->nopadding = 0;
  return;
}

/*
 * Decode and return one symbol from the provided PSCD byte stream
 * that starts in s->pscd_ptr and ends in the byte before s->pscd_end.
 * The context cx is a 12-bit integer in the range 0..4095. This
 * function will advance s->pscd_ptr each time it has consumed all
 * information from that PSCD byte.
 *
 * If a symbol has been decoded successfully, the return value will be
 * 0 or 1 (depending on the symbol).
 *
 * If the decoder was not able to decode a symbol from the provided
 * PSCD, then the return value will be -1, and two cases can be
 * distinguished:
 *
 * s->pscd_ptr == s->pscd_end:
 *
 *   The decoder has used up all information in the provided PSCD
 *   bytes. Further PSCD bytes have to be provided (via new values of
 *   s->pscd_ptr and/or s->pscd_end) before another symbol can be
 *   decoded.
 *
 * s->pscd_ptr == s->pscd_end - 1:
 * 
 *   The decoder has used up all provided PSCD bytes except for the
 *   very last byte, because that has the value 0xff. The decoder can
 *   at this point not yet tell whether this 0xff belongs to a
 *   MARKER_STUFF sequence or marks the end of the PSCD. Further PSCD
 *   bytes have to be provided (via new values of s->pscd_ptr and/or
 *   s->pscd_end), including the not yet processed 0xff byte, before
 *   another symbol can be decoded successfully.
 *
 * If s->nopadding != 0, the decoder will return -2 when it reaches
 * the first two bytes of the marker segment that follows (and
 * terminates) the PSCD, but before decoding the first symbol that
 * depends on a bit in the input data that could have been the result
 * of zero padding, and might, therefore, never have been encoded.
 * This gives the caller the opportunity to lookahead early enough
 * beyond a terminating SDNORM/SDRST for a trailing NEWLEN (as
 * required by T.85) before decoding remaining symbols. Call the
 * decoder again afterwards as often as necessary (leaving s->pscd_ptr
 * pointing to the start of the marker segment) to retrieve any
 * required remaining symbols that might depend on padding.
 *
 * [Note that each PSCD can be decoded into an infinitely long
 * sequence of symbols, because the encoder might have truncated away
 * an arbitrarily long sequence of trailing 0x00 bytes, which the
 * decoder will append automatically as needed when it reaches the end
 * of the PSCD. Therefore, the decoder cannot report any end of the
 * symbol sequence and other means (external to the PSCD and
 * arithmetic decoding process) are needed to determine that.]
 */

int arith_decode(struct jbg_ardec_state *s, int cx)
{
  register unsigned lsz, ss;
  register unsigned char *st;
  int pix;

  /* renormalization */
  while (s->a < 0x8000 || s->startup) {
    while (s->ct <= 8 && s->ct >= 0) {
      /* first we can move a new byte into s->c */
      if (s->pscd_ptr >= s->pscd_end) {
	return -1;  /* more bytes needed */
      }
      if (*s->pscd_ptr == 0xff) 
	if (s->pscd_ptr + 1 >= s->pscd_end) {
	  return -1; /* final 0xff byte not processed */
	} else {
	  if (*(s->pscd_ptr + 1) == MARKER_STUFF) {
	    s->c |= 0xffL << (8 - s->ct);
	    s->ct += 8;
	    s->pscd_ptr += 2;
	  } else {
	    s->ct = -1; /* start padding with zero bytes */
	    if (s->nopadding) {
	      s->nopadding = 0;
	      return -2; /* subsequent symbols might depend on zero padding */
	    }
	  }
	}
      else {
	s->c |= (long)*(s->pscd_ptr++) << (8 - s->ct);
	s->ct += 8;
      }
    }
    s->c <<= 1;
    s->a <<= 1;
    if (s->ct >= 0) s->ct--;
    if (s->a == 0x10000L)
      s->startup = 0;
  }

  st = s->st + cx;
  ss = *st & 0x7f;
  assert(ss < 113);
  lsz = lsztab[ss];

#if 0
  fprintf(stderr, "cx = %d, mps = %d, st = %3d, lsz = 0x%04x, a = 0x%05lx, "
	  "c = 0x%08lx, ct = %2d\n",
	  cx, !!(s->st[cx] & 0x80), ss, lsz, s->a, s->c, s->ct);
#endif

  if ((s->c >> 16) < (s->a -= lsz))
    if (s->a & 0xffff8000L)
      return *st >> 7;
    else {
      /* MPS_EXCHANGE */
      if (s->a < lsz) {
	pix = 1 - (*st >> 7);
	/* Check whether MPS/LPS exchange is necessary
	 * and chose next probability estimator status */
	*st &= 0x80;
	*st ^= nlpstab[ss];
      } else {
	pix = *st >> 7;
	*st &= 0x80;
	*st |= nmpstab[ss];
      }
    }
  else {
    /* LPS_EXCHANGE */
    if (s->a < lsz) {
      s->c -= s->a << 16;
      s->a = lsz;
      pix = *st >> 7;
      *st &= 0x80;
      *st |= nmpstab[ss];
    } else {
      s->c -= s->a << 16;
      s->a = lsz;
      pix = 1 - (*st >> 7);
      /* Check whether MPS/LPS exchange is necessary
       * and chose next probability estimator status */
      *st &= 0x80;
      *st ^= nlpstab[ss];
    }
  }

  return pix;
}
