/*
 * @(#)util.c	1.27 95/12/08 Chris Warth
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*-
	utility routines needed by both the compiler and the interpreter.
 */

#include "oobj.h"
#include "interpreter.h"
#include "utf.h"
#include "path_md.h"
#include "sys_api.h"
#include "str2id.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "io_md.h"      /* sysWrite() */

unicode *
str2unicode(char *str, unicode *ustr, long len)
{
    unicode *dst = ustr;

/* XXX rjp - revisit */
#if defined(_WINDOWS) && !defined(_WIN32)
    while (*str && --len >= 0)
		*ustr++ = (unicode) *str++;
    while (--len >= 0)
		*ustr++ = (unicode) 0;

#else
    memset((char *) dst, 0, len * sizeof(*dst));
    while (*str && --len >= 0)
	*ustr++ = *str++;
#endif

    return dst;
}


void
unicode2str(unicode *src, char *dst, long len)
{
    while (--len >= 0)
	*dst++ = (char) *src++;
    *dst = '\0';
}

/*
 * Print s null terminated C-style character string.
 */
void
prints(char *s)
{
#ifdef PRINTS_STDIO
    int    c;
    while (c = *s++)
	putchar(c);
#else
    if (s)
	sysWrite(1, s, strlen(s));
#endif
}

/*
 * Print an Java-style unicode string
 */
void
printus(unicode *str, long len)
{
#define UNISTRLEN 100
    char obuf[UNISTRLEN+1];
    register int p;
    if (str == 0 || len <= 0)
	return;
    while (len > 0) {
	unicode c;
	for (p = 0; p < sizeof obuf - 10 && --len >= 0;)
	    if ((c = *str++) >= 256) {
		/*
		 * Make unicode characters readable.  Should not be
		 * possible to overflow, but truncate silently if so.
		 */
		(void) jio_snprintf(obuf + p, UNISTRLEN+1 - p, "\\<%X>", c);
		p += strlen(obuf + p);
	    } else
		obuf[p++] = (char) c;
	if (p)
#ifdef PRINTS_STDIO
	    fwrite(obuf, 1, p, stdout);
#else
	    sysWrite(1, obuf, p);
#endif
    }
}

#define STR2ID_HTOVERFLOWPOINT ((int)(STR2ID_HTSIZE*0.8))

/* Given a string, return a unique 16 bit id number.  If param
 * isn't null, we also set up an array of void * for holding info about each
 * object.  The address of this void * is stored into param
 */
/* Given a string, return a unique 16 bit id number.  If param
 * isn't null, we also set up an array of void * for holding info about each
 * object.  The address of this void * is stored into param
 */
hash_t
Str2ID(StrIDhash **idhroot, register char *s, void ***param, int CopyNeeded)
{
    /*
     * The database is a hash table.  When the hash table overflows, a new
     * hashtable is created chained onto the previous one.  This is done so
     * that we can use the hashtable slot index as the ID, without worrying
     * about having the IDs change when the hashtable grows.
     */
    StrIDhash *h;
    PRArenaPool *stringArena;
    char *s2;
    long i;
    uint32_t raw_hash, hash;
    uint32_t secondary_hash;
    for (raw_hash = 0, s2 = s; (i = *s2) != 0; s2++) 
	raw_hash = raw_hash * 37 + i;
    hash = raw_hash % STR2ID_HTSIZE;
    secondary_hash = (raw_hash & 7) + 1; /* between 1 and 8 */
    if ((h = *idhroot) == 0) {
	h = (StrIDhash *) sysCalloc(1, sizeof(*h));
	if (!h) {
	    /* REMIND: Should throw an exception? */
	    out_of_memory();
	    return 0;
	}
	*idhroot = h;
	PR_InitArenaPool(&(h->stringArena), "string pool", kHashTableArenaIncrementSize, 1);
	h->baseid = 1;  /* guarantee that no ID is 0 */
    }
	stringArena = &(h->stringArena);
    while (1) {
	i = hash;
	while ((s2 = h->hash[i]) != 0) {
	    if (strcmp(h->hash[i], s) == 0) {
		if (param)
		    *param = &h->param[i];
		return h->baseid + i;
	    }
	    if ((i -= secondary_hash) < 0)
		i += STR2ID_HTSIZE;
	}
	if (h->next == 0)
	    break;
	h = h->next;
    }
    /* Couldn't find the string */
    if (h->used >= STR2ID_HTOVERFLOWPOINT) {
	/* have to create a new hashtable */
	h->next = (StrIDhash *) sysCalloc(1, sizeof(*h));
	if (!h->next) {
	    /* REMIND: Should throw an exception? */
	    out_of_memory();
	    return 0;
	}
	h->next->baseid = h->baseid + STR2ID_HTSIZE;
	h = h->next;
	i = hash;
    }
    if (CopyNeeded) {
	PR_ARENA_ALLOCATE(h->hash[i], stringArena, strlen(s) + 1);
	strcpy(h->hash[i], s);
	h->is_malloced[i >> 5] |= (1 << (i & 31));
    } else  {
	h->hash[i] = s;
    }
    h->used++;
    if (param) {
	if (h->param == 0) {
	    h->param = (void **)sysCalloc(STR2ID_HTSIZE, sizeof(*param));
	    if (!h->param) {
		/* REMIND: Should throw an exception? */
		out_of_memory();
		return 0;
	    }
	}
	*param = &h->param[i];
    }
    return (hash_t) (h->baseid + i);
}

void
Str2IDFree(StrIDhash **hash_ptr)
{
    StrIDhash *hash, *next;
    hash = *hash_ptr;
	PR_FreeArenaPool(&(hash->stringArena));
    while (hash) {
	next = hash->next;
	if (hash->param)
	    sysFree(hash->param);
	sysFree(hash);
	hash = next;
    }
    *hash_ptr = 0;
}

void
Str2IDApply(StrIDhash *hash_ptr,
	    void (*f)(StrIDhash *h, void *arg, void *param),
	    void *arg) {
    StrIDhash *hash, *next;
    int i;
    hash = hash_ptr;
    while (hash) {
	next = hash->next;
	for (i = 0; i < STR2ID_HTSIZE; i++) {
	    if (hash->param[i]) {
		(*f)(hash_ptr, arg, hash->param[i]);
	    }
	}
	hash = next;
    }
}

char *
ID2Str(StrIDhash *h, unsigned short ID, void ***param) {
    while ((long)(ID - h->baseid) >= STR2ID_HTSIZE)
	h = h->next;
    if (param) {
	if (h->param == 0) {
	    h->param = (void **)sysCalloc(STR2ID_HTSIZE, sizeof(*param));
	    if (!h->param) {
		/* REMIND: Should throw an exception? */
		out_of_memory();
		return 0;
	    }
	}
	*param = &h->param[ID - h->baseid];
    }
    return h->hash[ID - h->baseid];
}

char       *
addstr(char *s, char *buf,
       char *limit, char term)
{
    char        c;
    while ((c = *s) && c != term && buf < limit) {
	*buf++ = c;
	s++;
    }
    return buf;
}

char       *
unicode2rd(unicode *s, long len)
{       /* unicode string to readable C string */
#define CSTRLEN 40
    static char buf[CSTRLEN+1];
    char *dp = buf;
    int c = 0;
    if (s == 0)
	return "NULL";
    *dp++ = '"';
    while (--len>=0 && (c = *s++) != 0 && dp < buf + sizeof buf - 10)
	if (040 <= c && c < 0177)
	    *dp++ = c;
	else
	    switch (c) {
	    case '\n':
		*dp++ = '\\';
		*dp++ = 'n';
		break;
	    case '\t':
		*dp++ = '\\';
		*dp++ = 't';
		break;
	    case '\r':
		*dp++ = '\\';
		*dp++ = 'r';
		break;
	    case '\b':
		*dp++ = '\\';
		*dp++ = 'b';
		break;
	    case '\f':
		*dp++ = '\\';
		*dp++ = 'f';
		break;
	    default:
		/* Should not be possible to overflow, truncate if so */
		(void) jio_snprintf(dp, CSTRLEN+1 - (dp - buf), "\\%X", c);
		dp += strlen(dp);
		break;
	    }
    *dp++ = '"';
    if (len >= 0 && c != 0)
	*dp++ = '.', *dp++ = '.', *dp++ = '.';
    *dp++ = 0;
    return buf;
}

char *
int642CString(int64_t number, char *buf, int buflen)
{
    ll2str(number, buf, buf + buflen);
    return buf;
}

void
out_of_memory()
{
    sysWrite(2, "**Out of memory, exiting**\n", 27);
    sysAbort();
}

#if 0
JAVA_PUBLIC_API(void) 
panic(const char *format, ...)
{
    va_list ap;
    char buf[256];

    va_start(ap, format);
    printf("\n*** panic: ");
    /* If buffer overflow, quietly truncate */
    (void) jio_vsnprintf(buf, sizeof(buf), format, ap);
    prints(buf);
    sysAbort();
}
#endif

char *
classname2string(char *src, char *dst, int size) {
    char *buf = dst;
    for (; (--size > 0) && (*src != '\0') ; src++, dst++) {
	if (*src == '/') {
	    *dst = '.';
	}  else {
	    *dst = *src;
	}
    }
    *dst = '\0';
    return buf;
}


/*
 * Warning: the buffer overflow checking here is pretty bogus although
 * should prevent overruns.
 */
void 
mangleMethodName(struct methodblock *mb, char *buffer, int buflen, char *suffix) 
{ 
    ClassClass *cb = fieldclass(&mb->fb);
    
    static char prefix[] = "Java_";

    char *bufptr = buffer;
    char *bufend = buffer + buflen; 

    /* Copy over Java_ */
    (void) jio_snprintf(bufptr, bufend - bufptr, prefix);
    bufptr += strlen(bufptr);

    /* Mangle the class name */
    bufptr += mangleUTFString(classname(cb), bufptr, bufend - bufptr, 
			      MangleUTF_Class);
    if (bufend - bufptr > 1) 
	*bufptr++ = '_';
    bufptr += mangleUTFString(mb->fb.name, bufptr, bufend - bufptr, 
			      MangleUTF_Field);
    (void) jio_snprintf(bufptr, bufend - bufptr, "%s", suffix);
}
    
int
mangleUTFString(char *name, char *buffer, int buflen, int type) { 
    char *ptr = name;
    char *bufptr = buffer;
    char *bufend = buffer + buflen - 1;	/* save space for NULL */
    int ch;
    while (((ch = next_utf2unicode(&ptr)) != '\0') && (bufend - bufptr > 0)) { 
	if (ch <= 0x7f && isalnum(ch)) {
	    *bufptr++ = ch;
	} else if (ch == '/' && type == MangleUTF_Class) { 
	    *bufptr++ = '_';
	} else if (ch == '_' && type == MangleUTF_FieldStub) { 
	    *bufptr++ = '_';
	} else {
	    /* DANGER.  
	     *   DO NOT USE jio_snprintf directly with format "%.5x"
	     *   It doesn't understand it, and you will lose horribly.
	     * REGNAD
	     */
	    char temp[10];
	    sprintf(temp, "_%.5x", ch);	/* six characters always plus null */
	    (void) jio_snprintf(bufptr, bufend - bufptr, "%s", temp);
	    bufptr += strlen(bufptr);
	}
    }
    *bufptr = '\0';
    return bufptr - buffer;
}


/*
 * jio_snprintf(): sprintf with buffer limit.
 */

#include <stdarg.h>
#include "oobj.h"
#include "interpreter.h"

typedef struct InstanceData {
    char *buffer;
    char *end;
} InstanceData;

#undef  ERROR
#define ERROR -1
#undef  SUCCESS
#define SUCCESS 0
#undef  CheckRet
#define CheckRet(x) { if ((x) == ERROR) return ERROR; }

static int put_char(InstanceData *this, int c)
{
    if (iscntrl(c) && c != '\n' && c != '\t') {
        c = '@' + (c & 0x1F);
        if (this->buffer >= this->end) {
            return ERROR;
        }
        *this->buffer++ = '^';
    }
    if (this->buffer >= this->end) {
        return ERROR;
    }
    *this->buffer++ = c;
    return SUCCESS;
}

static int
format_string(InstanceData *this, char *str, int left_justify, int min_width,
              int precision)
{
    int pad_length;
    char *p;

    if (str == 0) {
	return ERROR;
    }

    if ((int)strlen(str) < precision) {
        pad_length = min_width - strlen(str);
    } else {
        pad_length = min_width - precision;
    }
    if (pad_length < 0)
        pad_length = 0;
    if (left_justify) {
        while (pad_length > 0) {
            CheckRet(put_char(this, ' '));
            --pad_length;
        }
    }

    for (p = str; *p != '\0' && --precision >= 0; p++) {
        CheckRet(put_char(this, *p));
    }

    if (!left_justify) {
        while (pad_length > 0) {
            CheckRet(put_char(this, ' '));
            --pad_length;
        }
    }
    return SUCCESS;
}

#define MAX_DIGITS 32

static int
format_number(InstanceData *this, long value, int format_type, int left_justify,
              int min_width, int precision, bool_t zero_pad)
{
    int sign_value = 0;
    unsigned long uvalue;
    char convert[MAX_DIGITS+1];
    int place = 0;
    int pad_length = 0;
    static char digits[] = "0123456789abcdef";
    int base = 0;
    bool_t caps = FALSE;
    bool_t add_sign = FALSE;

    switch (format_type) {
      case 'o': case 'O':
          base = 8;
          break;
      case 'd': case 'D':
          add_sign = TRUE; /* fall through */
      case 'u': case 'U':
          base = 10;
          break;
      case 'X':
          caps = TRUE; /* fall through */
      case 'x':
          base = 16;
          break;
    }
    sysAssert(base > 0 && base <= 16);

    uvalue = value;
    if (add_sign) {
        if (value < 0) {
            sign_value = '-';
            uvalue = -value;
        }
    }

    do {
        convert[place] = digits[uvalue % (unsigned)base];
        if (caps) {
            convert[place] = toupper(convert[place]);
        }
        place++;
        uvalue = (uvalue / (unsigned)base);
        if (place > MAX_DIGITS) {
            return ERROR;
        }
    } while(uvalue);
    convert[place] = 0;

    pad_length = min_width - place;
    if (pad_length < 0) {
        pad_length = 0;
    }
    if (left_justify) {
        if (zero_pad && pad_length > 0) {
            if (sign_value) {
                CheckRet(put_char(this, sign_value));
                --pad_length;
                sign_value = 0;
            }
            while (pad_length > 0) {
                CheckRet(put_char(this, '0'));
                --pad_length;
            }
        } else {
            while (pad_length > 0) {
                CheckRet(put_char(this, ' '));
                --pad_length;
            }
        }
    }
    if (sign_value) {
        CheckRet(put_char(this, sign_value));
    }

    while (place > 0 && --precision >= 0) {
        CheckRet(put_char(this, convert[--place]));
    }

    if (!left_justify) {
        while (pad_length > 0) {
            CheckRet(put_char(this, ' '));
            --pad_length;
        }
    }
    return SUCCESS;
}

int
jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
    char *strvalue;
    long value;
    InstanceData this;
    bool_t left_justify, zero_pad, long_flag, add_sign, fPrecision;
    int min_width, precision, ch;

    if (str == NULL) {
	return ERROR;
    }
    str[0] = '\0';

    this.buffer = str;
    this.end = str + count - 1;
    *this.end = '\0';		/* ensure null-termination in case of failure */

    while ((ch = *fmt++) != 0) {
        if (ch == '%') {
            zero_pad = long_flag = add_sign = fPrecision = FALSE;
            left_justify = TRUE;
            min_width = 0;
            precision = this.end - this.buffer;
        next_char:
            ch = *fmt++;
            switch (ch) {
              case 0:
                  return ERROR;
              case '-':
                  left_justify = FALSE;
                  goto next_char;
              case '0': /* set zero padding if min_width not set */
                  if (min_width == 0)
                      zero_pad = TRUE;
              case '1': case '2': case '3':
              case '4': case '5': case '6':
              case '7': case '8': case '9':
                  if (fPrecision == TRUE) {
                      precision = precision * 10 + (ch - '0');
                  } else {
                      min_width = min_width * 10 + (ch - '0');
                  }
                  goto next_char;
              case '.':
                  fPrecision = TRUE;
                  precision = 0;
                  goto next_char;
              case 'l':
                  long_flag = TRUE;
                  goto next_char;
              case 's':
                  strvalue = va_arg(args, char *);
                  CheckRet(format_string(&this, strvalue, left_justify,
                                         min_width, precision));
                  break;
              case 'c':
                  ch = va_arg(args, int);
                  CheckRet(put_char(&this, ch));
                  break;
              case '%':
                  CheckRet(put_char(&this, '%'));
                  break;
              case 'd': case 'D':
              case 'u': case 'U':
              case 'o': case 'O':
              case 'x': case 'X':
                  value = long_flag ? va_arg(args, long) : va_arg(args, int);
                  CheckRet(format_number(&this, value, ch, left_justify,
                                         min_width, precision, zero_pad));
                  break;
              default:
                  return ERROR;
            }
        } else {
            CheckRet(put_char(&this, ch));
        }
    }
    *this.buffer = '\0';
    return strlen(str);
}

int jio_snprintf(char *str, size_t count, const char *fmt, ...)
{
    va_list args;
    int len;

    va_start(args, fmt);
    len = jio_vsnprintf(str, count, fmt, args);
    va_end(args);
    return len;
}

/* Return true if the two classes are in the same class package */
bool_t
IsSameClassPackage(ClassClass *class1, ClassClass *class2) 
{
    if (cbLoader(class1) != cbLoader(class2)) 
	return FALSE;
    else {
	char *name1 = classname(class1);
	char *name2 = classname(class2);
	char *last_slash1 = strrchr(name1, DIR_SEPARATOR);
	char *last_slash2 = strrchr(name2, DIR_SEPARATOR);
	if ((last_slash1 == NULL) || (last_slash2 == NULL)) {
	    return (last_slash1 == last_slash2); 
	} else {
	    int length1 = last_slash1 - name1;
	    int length2 = last_slash2 - name2;
	    return ((length1 == length2) 
		    && (strncmp(name1, name2, length1) == 0));
	}
    }
}

