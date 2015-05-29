#include <math.h>
#include <stdio.h>
/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

/***************************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 * 
 *  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats. 
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Andrew Tridgell (tridge@samba.org) Oct 1998
 *    fixed handling of %.0f
 *    added test for HAVE_LONG_DOUBLE
 *
 *  Russ Allbery <rra@stanford.edu> 2000-08-26
 *    fixed return value to comply with C99
 *    fixed handling of snprintf(NULL, ...)
 *
 *  Charles "Wog" Reiss <car@cs.brown.edu> 2001-04-13
 *    very minor updates for FB6
 *    modified va_arg stuff to handle default agument promotions
 *
 *  Revar Desmera <revar@belfry.com> 2005-09-06
 *    Added long long support for integers.
 *    Fixed %f where fractional part leads with multiple zeros like 2.0003.
 *    Allow FP to display more precision.
 *      Currently about 31 pre-decimal and 16 post-decimal digits are good.
 *      This needs improvement.  Don't use this for scientific purposes.
 *      The conversion is also kind of slow.
 *    Added %e, %E, %g, and %G support.
 *    Expanded testing to include longlong integers, %e, and %g formats.
 *
 ***************************************************************************/

#include "autoconf.h"

#ifdef TEST_SNPRINTF
# undef HAVE_SNPRINTF
# undef HAVE_VSNPRINTF
#endif

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

/* Define this as a fall through, HAVE_STDARG_H is probably already set */

/* #define HAVE_VARARGS_H -- we do this in configure */

/* varargs declarations: */

#if defined(HAVE_STDARG_H)
# include <stdarg.h>
# define HAVE_STDARGS    /* let's hope that works everywhere (mj) */
# define VA_LOCAL_DECL   va_list ap
# define VA_START(f)     va_start(ap, f)
# define VA_SHIFT(v,t)  ;   /* no-op for ANSI */
# define VA_END          va_end(ap)
#else
# if defined(HAVE_VARARGS_H)
#  include <varargs.h>
#  undef HAVE_STDARGS
#  define VA_LOCAL_DECL   va_list ap
#  define VA_START(f)     va_start(ap)      /* f is ignored! */
#  define VA_SHIFT(v,t) v = va_arg(ap,t)
#  define VA_END        va_end(ap)
# else
/*XX ** NO VARARGS ** XX*/
#  error snprintf.c requires varargs
# endif
#endif

#ifdef HAVE_LONG_LONG
#define LLONG long long
#else
#define LLONG long
#endif

#ifdef HAVE_LONG_DOUBLE
#define LDOUBLE long double
#else
#define LDOUBLE double
#endif

int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);

static int dopr (char *buffer, size_t maxlen, const char *format, 
                 va_list args);
static int fmtstr (char *buffer, size_t *currlen, size_t maxlen,
		   char *value, int flags, int min, int max);
static int fmtint (char *buffer, size_t *currlen, size_t maxlen,
		   LLONG value, int base, int min, int max, int flags);
static int fmtfp (char *buffer, size_t *currlen, size_t maxlen,
		  LDOUBLE fvalue, int min, int max, int flags);
static int dopr_outch (char *buffer, size_t *currlen, size_t maxlen, char c );

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
enum dp_state_t {
    DP_S_DEFAULT = 0,
    DP_S_FLAGS,
    DP_S_MIN,
    DP_S_DOT,
    DP_S_MAX,
    DP_S_MOD,
    DP_S_CONV,
    DP_S_DONE
};


/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)
#define DP_F_MANTISSA 	(1 << 7)
#define DP_F_SHORTFP 	(1 << 8)


/* Conversion Flags */
enum dp_conv_flag_t {
    DP_C_NONE = 0,
    DP_C_SHORT,
    DP_C_LONG,
    DP_C_LDOUBLE,
    DP_C_LONG_LONG
};


#define char_to_int(p) (p - '0')
#define MAX(p,q) ((p >= q) ? p : q)
#define MIN(p,q) ((p <= q) ? p : q)


static int
dopr(char *buffer, size_t maxlen, const char *format, va_list args)
{
  char ch;
  LLONG value;
  LDOUBLE fvalue;
  char *strvalue;
  int min;
  int max;
  enum dp_state_t state;
  enum dp_conv_flag_t cflags;
  int flags;
  int total;
  size_t currlen;
  
  state = DP_S_DEFAULT;
  cflags = DP_C_NONE;
  currlen = flags = min = 0;
  max = -1;
  ch = *format++;
  total = 0;

  while (state != DP_S_DONE)
  {
    if (ch == '\0')
      state = DP_S_DONE;

    switch(state) 
    {
    case DP_S_DEFAULT:
      if (ch == '%') 
	state = DP_S_FLAGS;
      else 
	total += dopr_outch (buffer, &currlen, maxlen, ch);
      ch = *format++;
      break;
    case DP_S_FLAGS:
      switch (ch) 
      {
      case '-':
	flags |= DP_F_MINUS;
        ch = *format++;
	break;
      case '+':
	flags |= DP_F_PLUS;
        ch = *format++;
	break;
      case ' ':
	flags |= DP_F_SPACE;
        ch = *format++;
	break;
      case '#':
	flags |= DP_F_NUM;
        ch = *format++;
	break;
      case '0':
	flags |= DP_F_ZERO;
        ch = *format++;
	break;
      default:
	state = DP_S_MIN;
	break;
      }
      break;
    case DP_S_MIN:
      if (isdigit(ch)) 
      {
	min = 10*min + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	min = va_arg (args, int);
	ch = *format++;
	state = DP_S_DOT;
      } 
      else 
	state = DP_S_DOT;
      break;
    case DP_S_DOT:
      if (ch == '.') 
      {
	state = DP_S_MAX;
	ch = *format++;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MAX:
      if (isdigit(ch)) 
      {
	if (max < 0)
	  max = 0;
	max = 10*max + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	max = va_arg (args, int);
	ch = *format++;
	state = DP_S_MOD;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MOD:
      switch (ch) 
      {
      case 'h':
	cflags = DP_C_SHORT;
	ch = *format++;
	break;
      case 'l':
	cflags = DP_C_LONG;
	ch = *format++;
	if (ch == 'l') {
	    cflags = DP_C_LONG_LONG;
	    ch = *format++;
	}
	break;
      case 'L':
	cflags = DP_C_LDOUBLE;
	ch = *format++;
	break;
      default:
	break;
      }
      state = DP_S_CONV;
      break;
    case DP_S_CONV:
      switch (ch) 
      {
      case 'd':
      case 'i':
	if (cflags == DP_C_SHORT) 
#ifdef IGNORE_ARGUMENT_PROMOTION
	  value = va_arg (args, short int);
#else
	  value = va_arg (args, int);
#endif
	else if (cflags == DP_C_LONG_LONG)
	  value = va_arg (args, LLONG int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, long int);
	else
	  value = va_arg (args, int);
	total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'o':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
#ifdef IGNORE_ARGUMENT_PROMOTION
	  value = va_arg (args, unsigned short int);
#else
	  value = va_arg (args, unsigned int);
#endif
	else if (cflags == DP_C_LONG_LONG)
	  value = va_arg (args, unsigned LLONG int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 8, min, max, flags);
	break;
      case 'u':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
#ifdef IGNORE_ARGUMENT_PROMOTION	
	  value = va_arg (args, unsigned short int);
#else
	  value = va_arg (args, unsigned int);
#endif
	else if (cflags == DP_C_LONG_LONG)
	  value = va_arg (args, unsigned LLONG int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'X':
	flags |= DP_F_UP;
      case 'x':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
#ifdef IGNORE_ARGUMENT_PROMOTION
	  value = va_arg (args, unsigned short int);
#else
	  value = va_arg (args, unsigned int);
#endif
	else if (cflags == DP_C_LONG_LONG)
	  value = va_arg (args, unsigned LLONG int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
	break;
      case 'f':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	total += fmtfp(buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'E':
	flags |= DP_F_UP;
      case 'e':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	flags |= DP_F_MANTISSA;
	total += fmtfp(buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'G':
	flags |= DP_F_UP;
      case 'g':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	flags |= DP_F_SHORTFP;
	total += fmtfp(buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'c':
	total += dopr_outch (buffer, &currlen, maxlen, va_arg (args, int));
	break;
      case 's':
	strvalue = va_arg (args, char *);
	total += fmtstr (buffer, &currlen, maxlen, strvalue, flags, min, max);
	break;
      case 'p':
	strvalue = va_arg (args, void *);
	value = (long)strvalue;
	total += fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
	break;
      case 'n':
	if (cflags == DP_C_SHORT) 
	{
	  short int *num;
	  num = va_arg (args, short int *);
	  *num = currlen;
        } 
	else if (cflags == DP_C_LONG_LONG) 
	{
	  LLONG int *num;
	  num = va_arg (args, LLONG int *);
	  *num = currlen;
        } 
	else if (cflags == DP_C_LONG) 
	{
	  long int *num;
	  num = va_arg (args, long int *);
	  *num = currlen;
        } 
	else 
	{
	  int *num;
	  num = va_arg (args, int *);
	  *num = currlen;
        }
	break;
      case '%':
	total += dopr_outch (buffer, &currlen, maxlen, ch);
	break;
      case 'w':
	/* not supported yet, treat as next char */
	ch = *format++;
	break;
      default:
	/* Unknown, skip */
	break;
      }
      ch = *format++;
      state = DP_S_DEFAULT;
      cflags = DP_C_NONE;
      flags = min = 0;
      max = -1;
      break;
    case DP_S_DONE:
      break;
    default:
      /* hmm? */
      break; /* some picky compilers need this */
    }
  }
  if (buffer != NULL)
  {
    if (currlen < maxlen - 1) 
      buffer[currlen] = '\0';
    else 
      buffer[maxlen - 1] = '\0';
  }
  return total;
}


static int
fmtstr(char *buffer, size_t *currlen, size_t maxlen,
       char *value, int flags, int min, int max)
{
  int padlen, strln;     /* amount to pad */
  int cnt = 0;
  int total = 0;
  
  if (value == 0)
  {
    value = "<NULL>";
  }

  for (strln = 0; value[strln]; ++strln); /* strlen */
  if (max >= 0 && max < strln)
    strln = max;
  padlen = min - strln;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justify */

  while (padlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
  }
  while (*value && ((max < 0) || (cnt < max)))
  {
    total += dopr_outch (buffer, currlen, maxlen, *value++);
    ++cnt;
  }
  while (padlen < 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
  }
  return total;
}


/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static int
fmtint(char *buffer, size_t *currlen, size_t maxlen,
       LLONG value, int base, int min, int max, int flags)
{
  int signvalue = 0;
  unsigned LLONG uvalue;
  char convert[40];
  int place = 0;
  int spadlen = 0; /* amount to space pad */
  int zpadlen = 0; /* amount to zero pad */
  int caps = 0;
  int total = 0;
  
  if (max < 0)
    max = 0;

  uvalue = value;

  if(!(flags & DP_F_UNSIGNED))
  {
    if( value < 0 ) {
      signvalue = '-';
      uvalue = -value;
    }
    else
      if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
	signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';
  }
  
  if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

  do {
    convert[place++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")
      [uvalue % (unsigned)base  ];
    uvalue = (uvalue / (unsigned)base );
  } while(uvalue && (place < sizeof(convert)));
  if (place == sizeof(convert)) place--;
  convert[place] = 0;

  zpadlen = max - place;
  spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
  if (zpadlen < 0) zpadlen = 0;
  if (spadlen < 0) spadlen = 0;
  if (flags & DP_F_ZERO)
  {
    zpadlen = MAX(zpadlen, spadlen);
    spadlen = 0;
  }
  if (flags & DP_F_MINUS) 
    spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
      zpadlen, spadlen, min, max, place));
#endif

  /* Spaces */
  while (spadlen > 0) 
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --spadlen;
  }

  /* Sign */
  if (signvalue) 
    total += dopr_outch (buffer, currlen, maxlen, signvalue);

  /* Zeros */
  if (zpadlen > 0) 
  {
    while (zpadlen > 0)
    {
      total += dopr_outch (buffer, currlen, maxlen, '0');
      --zpadlen;
    }
  }

  /* Digits */
  while (place > 0) 
    total += dopr_outch (buffer, currlen, maxlen, convert[--place]);
  
  /* Left Justified spaces */
  while (spadlen < 0) {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++spadlen;
  }

  return total;
}


static LDOUBLE
abs_val(LDOUBLE value)
{
  LDOUBLE result = value;

  if (value < 0)
    result = -value;

  return result;
}


static int
int_log10(LDOUBLE val)
{
  int result = 0;
  val = abs_val(val);
  if (val == 0.0) {
      return 0;
  } else if (val < 1.0) {
      while (val < 1.0) {
          val *= 10.0;
	  result--;
      }
  } else {
      while (val >= 10.0) {
          val /= 10.0;
	  result++;
      }
  }

  return result;
}


static LDOUBLE
ldouble_pow10(int exp)
{
  LDOUBLE result = 1;

  if (exp > 0) {
    while (exp)
    {
      result *= 10;
      exp--;
    }
  } else {
    while (exp)
    {
      result /= 10.0;
      exp++;
    }
  }
  
  return result;
}


static LDOUBLE
ldouble_round(LDOUBLE value)
{
  LDOUBLE intpart;

  intpart = floor(value);
  value = value - intpart;
  if (value >= 0.5)
    intpart += 1.0;

  return intpart;
}


static int
fmtfp(char *buffer, size_t *currlen, size_t maxlen,
      LDOUBLE fvalue, int min, int max, int flags)
{
  int signvalue = 0;
  int signmant = 0;
  LDOUBLE ufvalue;
  char iconvert[256];
  char fconvert[256];
  char mconvert[40];
  int iplace = 0;
  int fplace = 0;
  int mplace = 0;
  int idigits = 0;
  int padlen = 0; /* amount to pad */
  int zpadlen = 0; 
  int nzfound = 0; 
  int caps = 0;
  int total = 0;
  char * ptr;
  LDOUBLE intpart;
  LDOUBLE fracpart;
  long mantpart = 0;
  LDOUBLE maxpow, origmaxpow; 
  
  /* 
   * AIX manpage says the default is 0, but Solaris says the default
   * is 6, and sprintf on AIX defaults to 6
   */
  if (max < 0)
    max = 6;

  ufvalue = abs_val(fvalue);

  if (fvalue < 0)
    signvalue = '-';
  else
    if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
      signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';

  if (flags & DP_F_SHORTFP) {
    if (max < 1) {
      max = 1;
    }
    mantpart = int_log10(ufvalue);
    if (mantpart < -4 || mantpart >= max) {
      flags |= DP_F_MANTISSA;
    }
  }
  if (flags & DP_F_MANTISSA) {
      mantpart = int_log10(ufvalue);
      ufvalue /= ldouble_pow10(mantpart);
  }

  if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

  intpart = floor(ufvalue);
  if (flags & DP_F_SHORTFP) {
    if (intpart >= 1.0) {
      max -= int_log10(intpart)+1;
      if (max < 0) {
	max = 0;
      }
    }
  }

  /* We "cheat" by converting the fractional part to integer by
   * multiplying by a factor of 10
   */
  origmaxpow = maxpow = ldouble_pow10(max);
  fracpart = maxpow * (ufvalue - intpart);
  if (flags & DP_F_SHORTFP && intpart < 1.0) {
      while (fracpart*10.0 < origmaxpow && fracpart > 0.0) {
          fracpart *= 10.0;
	  max++;
	  maxpow *= 10.0;
      }
  }
  fracpart = ldouble_round(fracpart);

  if (fracpart >= maxpow) {
    intpart += 1.0;
    fracpart -= maxpow;
  }

  if (flags & DP_F_MANTISSA) {
    if (intpart >= 10) {
      long part = fmod(intpart,10.0);
      intpart /= 10;
      fracpart += maxpow * part;
      fracpart /= 10;
      mantpart++;
    }
    if (mantpart < 0) {
      signmant = '-';
      mantpart = -mantpart;
    } else {
      signmant = '+';
    }
  }


#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "fmtfp: %f =? %.0f.%.0f\n", fvalue, intpart, fracpart));
#endif

  /* Convert integer part */
  /* This is slow, due to lots of division, but more accurate. */
  if (ufvalue > 1.0) {
    idigits = int_log10(ufvalue);
  } else {
    idigits = 0;
  }
  maxpow = ldouble_pow10(idigits);
  while (idigits-- >= 0 && iplace < sizeof(iconvert)) {
    int digit = (int)(intpart/maxpow);
    iconvert[iplace++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")[digit];
    intpart -= digit * maxpow;
    maxpow /= 10.0;
  }
  if (iplace == sizeof(iconvert)) iplace--;
  iconvert[iplace] = 0;

  /* Convert fractional part */
  if (!(flags & DP_F_SHORTFP)) {
    nzfound = 1;
  }
  do {
    int digit = (int)fmod(fracpart,10.0);
    if (digit != 0)
        nzfound = 1;
    if (nzfound) {
      fconvert[fplace++] =
	(caps? "0123456789ABCDEF":"0123456789abcdef")[digit];
    } else {
      max--;
      if (max < 0) {
        max = 0;
      }
    }
    fracpart = floor(fracpart / 10.0);
  } while((fplace < max || fracpart > 0.0) && (fplace < sizeof(fconvert)));
  if (fplace == sizeof(fconvert)) fplace--;
  fconvert[fplace] = 0;

  if (flags & DP_F_MANTISSA) {
    /* Convert mantissa part */
    do {
      mconvert[mplace++] =
	(caps? "0123456789ABCDEF":"0123456789abcdef")[mantpart % 10];
      mantpart = (mantpart / 10);
    } while(mantpart && (mplace < sizeof(mconvert)));
    if (mplace == 1) {
	mconvert[mplace++] = '0';
    }
    if (mplace == sizeof(mconvert)) mplace--;
    mconvert[mplace] = 0;
  }

  /* -1 for decimal point, another -1 if we are printing a sign */
  padlen = min - iplace - max - ((signvalue) ? 1 : 0); 
  zpadlen = max - fplace;
  if (max > 0) {
    padlen--;
  }
  if (flags & DP_F_MANTISSA) {
    padlen -= mplace + 2;
    zpadlen -= mplace;
  }
  if (zpadlen < 0)
    zpadlen = 0;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_SHORTFP) {
    padlen += zpadlen;
    zpadlen = 0;
  }
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justifty */

  if ((flags & DP_F_ZERO) && (padlen > 0)) 
  {
    if (signvalue) 
    {
      total += dopr_outch (buffer, currlen, maxlen, signvalue);
      --padlen;
      signvalue = 0;
    }
    while (padlen > 0)
    {
      total += dopr_outch (buffer, currlen, maxlen, '0');
      --padlen;
    }
  }
  while (padlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
  }
  if (signvalue) 
    total += dopr_outch (buffer, currlen, maxlen, signvalue);

  ptr = iconvert;
  while (iplace-->0) 
    total += dopr_outch (buffer, currlen, maxlen, *ptr++);

  /*
   * Decimal point.  This should probably use locale to find the correct
   * char to print out.
   */
  if (max > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, '.');

    while (fplace > 0) 
      total += dopr_outch (buffer, currlen, maxlen, fconvert[--fplace]);
  }

  if (flags & DP_F_MANTISSA) {
    total += dopr_outch (buffer, currlen, maxlen, caps? 'E' : 'e');
    total += dopr_outch (buffer, currlen, maxlen, signmant);
    while (mplace > 0) 
      total += dopr_outch (buffer, currlen, maxlen, mconvert[--mplace]);
  }

  while (zpadlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, '0');
    --zpadlen;
  }

  while (padlen < 0) 
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
  }

  return total;
}


static int
dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c)
{
  if (*currlen + 1 < maxlen)
    buffer[(*currlen)++] = c;
  return 1;
}


#ifndef HAVE_VSNPRINTF
int
vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
  if (str != NULL)
    str[0] = 0;
  return dopr(str, count, fmt, args);
}
#endif /* !HAVE_VSNPRINTF */


#ifndef HAVE_SNPRINTF
/* VARARGS3 */
#ifdef HAVE_STDARGS
int
snprintf(char *str,size_t count,const char *fmt,...)
#else
int
snprintf(va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
  char *str;
  size_t count;
  char *fmt;
#endif
  VA_LOCAL_DECL;
  int total;
    
  VA_START (fmt);
  VA_SHIFT (str, char *);
  VA_SHIFT (count, size_t );
  VA_SHIFT (fmt, char *);
  total = vsnprintf(str, count, fmt, ap);
  VA_END;
  return total;
}
#endif /* !HAVE_SNPRINTF */

#ifdef TEST_SNPRINTF
#ifndef LONG_STRING
#define LONG_STRING 1024
#endif
#include <stdio.h>
int
main (void)
{
  char buf1[LONG_STRING];
  char buf2[LONG_STRING];
  char *fp_fmt[] = {
    "%-1.5f",
    "%1.5f",
    "%123.9f",
    "%10.5f",
    "% 10.5f",
    "%22.15f",
    "%+22.9f",
    "%+4.9f",
    "%01.3f",
    "%4f",
    "%3.1f",
    "%3.2f",
    "%.0f",
    "%.1f",
    "%-1.5e",
    "%1.5e",
    "%123.9e",
    "%10.5e",
    "% 10.5e",
    "%22.15e",
    "%+22.9e",
    "%+4.9e",
    "%01.3e",
    "%4e",
    "%3.1e",
    "%3.2e",
    "%.0e",
    "%.1e",
    "%-1.5g",
    "%1.5g",
    "%123.9g",
    "%10.5g",
    "% 10.5g",
    "%22.15g",
    "%+22.9g",
    "%+4.9g",
    "%01.3g",
    "%4g",
    "%3.1g",
    "%3.2g",
    "%.0g",
    "%.1g",
    NULL
  };
  double fp_nums[] = { -1.5, 134.21, 91340.2, 398.00043, 341.1234, 0203.9,
    0.96, 0.996, 9.648433465e+31, 1.0/1073.0, 0.9996, 1.996, 4.136, 0};
  char *int_fmt[] = {
    "%-1.5d",
    "%1.5d",
    "%123.9d",
    "%5.5d",
    "%10.5d",
    "% 10.5d",
    "%+22.33d",
    "%01.3d",
    "%4d",
    "%-1.5X",
    "%1.5X",
    "%123.9X",
    "%5.5X",
    "%10.5X",
    "% 10.5X",
    "%+22.33X",
    "%01.3X",
    "%4X",
    "%-1.5x",
    "%1.5x",
    "%123.9x",
    "%5.5x",
    "%10.5x",
    "% 10.5x",
    "%+22.33x",
    "%01.3x",
    "%4x",
    "%-1.5o",
    "%1.5o",
    "%123.9o",
    "%5.5o",
    "%10.5o",
    "% 10.5o",
    "%+22.33o",
    "%01.3o",
    "%4o",
    "%-1.5u",
    "%1.5u",
    "%123.9u",
    "%5.5u",
    "%10.5u",
    "% 10.5u",
    "%+22.33u",
    "%01.3u",
    "%4u",
    NULL
  };
  long int_nums[] = { -1, 134, 91340, 341, 0203, 0};
  char *llint_fmt[] = {
    "%-1.5lld",
    "%1.5lld",
    "%123.9lld",
    "%5.5lld",
    "%10.5lld",
    "% 10.5lld",
    "%+22.33lld",
    "%01.3lld",
    "%4lld",
    "%-1.5llx",
    "%1.5llx",
    "%123.9llx",
    "%5.5llx",
    "%10.5llx",
    "% 10.5llx",
    "%+22.33llx",
    "%01.3llx",
    "%4llx",
    "%-1.5llX",
    "%1.5llX",
    "%123.9llX",
    "%5.5llX",
    "%10.5llX",
    "% 10.5llX",
    "%+22.33llX",
    "%01.3llX",
    "%4llX",
    "%-1.5llo",
    "%1.5llo",
    "%123.9llo",
    "%5.5llo",
    "%10.5llo",
    "% 10.5llo",
    "%+22.33llo",
    "%01.3llo",
    "%4llo",
    "%-1.5llu",
    "%1.5llu",
    "%123.9llu",
    "%5.5llu",
    "%10.5llu",
    "% 10.5llu",
    "%+22.33llu",
    "%01.3llu",
    "%4llu",
    NULL
  };
  LLONG llint_nums[] = { -1, 134, 91340, 87359837092359824ll, 341, 0203, 0};
  int x, y;
  int fail = 0;
  int num = 0;

  printf ("Testing snprintf format codes against system sprintf...\n");

  for (x = 0; fp_fmt[x] != NULL ; x++)
    for (y = 0; fp_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), fp_fmt[x], fp_nums[y]);
      sprintf (buf2, fp_fmt[x], fp_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s with value %8g\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    fp_fmt[x], fp_nums[y], buf1, buf2);
	fail++;
      }
      num++;
    }

  for (x = 0; int_fmt[x] != NULL ; x++)
    for (y = 0; int_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), int_fmt[x], int_nums[y]);
      sprintf (buf2, int_fmt[x], int_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    int_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }

  for (x = 0; llint_fmt[x] != NULL ; x++)
    for (y = 0; llint_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), llint_fmt[x], llint_nums[y]);
      sprintf (buf2, llint_fmt[x], llint_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    llint_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }
  printf ("%d tests failed out of %d.\n", fail, num);
}
#endif /* SNPRINTF_TEST */

#endif /* !HAVE_SNPRINTF */

