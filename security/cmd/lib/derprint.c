#include "secutil.h"

#ifdef __sun
extern int fprintf(FILE *strm, const char *format, .../* args */);
extern int fflush(FILE *stream);
#endif
extern int SEC_ERROR_BAD_DER;
extern int SEC_ERROR_IO;

#define RIGHT_MARGIN	24
/*#define RAW_BYTES 1 */

static int prettyColumn = 0;

static int
getInteger256(unsigned char *data, unsigned int nb)
{
    int val;

    switch (nb) {
      case 1:
	val = data[0];
	break;
      case 2:
	val = (data[0] << 8) | data[1];
	break;
      case 3:
	val = (data[0] << 16) | (data[1] << 8) | data[2];
	break;
      case 4:
	val = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
	break;
      default:
	PORT_SetError(SEC_ERROR_BAD_DER);
	return -1;
    }

    return val;
}

static int
prettyNewline(FILE *out)
{
    int rv;

    if (prettyColumn != -1) {
	rv = fprintf(out, "\n");
	prettyColumn = -1;
	if (rv < 0) {
	    PORT_SetError(SEC_ERROR_IO);
	    return rv;
	}
    }
    return 0;
}

static int
prettyIndent(FILE *out, unsigned level)
{
    unsigned int i;
    int rv;

    if (prettyColumn == -1) {
	prettyColumn = level;
	for (i = 0; i < level; i++) {
	    rv = fprintf(out, "   ");
	    if (rv < 0) {
		PORT_SetError(SEC_ERROR_IO);
		return rv;
	    }
	}
    }

    return 0;
}

static int
prettyPrintByte(FILE *out, unsigned char item, unsigned int level)
{
    int rv;

    rv = prettyIndent(out, level);
    if (rv < 0)
	return rv;

    rv = fprintf(out, "%02x ", item);
    if (rv < 0) {
	PORT_SetError(SEC_ERROR_IO);
	return rv;
    }

    prettyColumn++;
    if (prettyColumn >= RIGHT_MARGIN) {
	return prettyNewline(out);
    }

    return 0;
}

static int
prettyPrintLeaf(FILE *out, unsigned char *data,
		unsigned int len, unsigned int lv)
{
    unsigned int i;
    int rv;

    for (i = 0; i < len; i++) {
	rv = prettyPrintByte(out, *data++, lv);
	if (rv < 0)
	    return rv;
    }
    return prettyNewline(out);
}

static int
prettyPrintString(FILE *out, unsigned char *str,
		  unsigned int len, unsigned int level)
{
#define BUF_SIZE 100
    unsigned char buf[BUF_SIZE];
    int rv;

    if (len >= BUF_SIZE)
	len = BUF_SIZE - 1;

    rv = prettyNewline(out);
    if (rv < 0)
	return rv;

    rv = prettyIndent(out, level);
    if (rv < 0)
	return rv;

    memcpy(buf, str, len);
    buf[len] = '\000';

    rv = fprintf(out, "\"%s\"", buf);
    if (rv < 0) {
	PORT_SetError(SEC_ERROR_IO);
	return rv;
    }

    return prettyNewline(out);

#undef BUF_SIZE
}

static int
prettyPrintObjectID(FILE *out, unsigned char *data,
		    unsigned int len, unsigned int level)
{
    SECOidData *oiddata;
    SECItem oiditem;
    unsigned int i;
    unsigned long val;
    int rv;


    oiditem.data = data;
    oiditem.len = len;

    rv = prettyIndent(out, level);
    if (rv < 0)
	return rv;

    val = data[0];
    i   = val % 40;
    val = val / 40;
    rv = fprintf(out, "%lu %u ", val, i);
    if (rv < 0) {
	PORT_SetError(SEC_ERROR_IO);
	return rv;
    }

    val = 0;
    for (i = 1; i < len; ++i) {
        unsigned long j;

	j = data[i];
	val = (val << 7) | (j & 0x7f);
	if (j & 0x80) 
	    continue;
	rv = fprintf(out, "%lu ", val);
	if (rv < 0) {
	    PORT_SetError(SEC_ERROR_IO);
	    return rv;
	}
	val = 0;
    }
    rv = prettyNewline(out);
    if (rv < 0) {
	PORT_SetError(SEC_ERROR_IO);
	return rv;
    }

    for (i = 0; i < len; i++) {
	rv = prettyPrintByte(out, *data++, level);
	if (rv < 0)
	    return rv;
    }

    oiddata = SECOID_FindOID(&oiditem);

    if (oiddata != NULL) {
	len = PORT_Strlen(oiddata->desc);
	if ((prettyColumn + 1 + (len / 3)) > RIGHT_MARGIN) {
	    rv = prettyNewline(out);
	    if (rv < 0)
		return rv;
	}

	rv = prettyIndent(out, level);
	if (rv < 0)
	    return rv;

	rv = fprintf(out, "(%s)", oiddata->desc);
	if (rv < 0) {
	    PORT_SetError(SEC_ERROR_IO);
	    return rv;
	}
    }

    return prettyNewline(out);
}

static char *prettyTagType [32] = {
  "End of Contents", "Boolean", "Integer", "Bit String", "Octet String",
  "NULL", "Object Identifier", "0x07", "0x08", "0x09", "0x0A",
  "0x0B", "0x0C", "0x0D", "0x0E", "0x0F", "Sequence", "Set",
  "0x12", "Printable String", "T61 String", "0x15", "IA5 String",
  "UTCTime",
  "0x18", "0x19", "0x1A", "0x1B", "0x1C", "0x1D", "0x1E",
  "High-Tag-Number"
};

static int
prettyPrintTag(FILE *out, unsigned char *src, unsigned char *end,
	       unsigned char *codep, unsigned int level)
{
    int rv;
    unsigned char code, tagnum;

    if (src >= end) {
	PORT_SetError(SEC_ERROR_BAD_DER);
	return -1;
    }

    code = *src;
    tagnum = code & DER_TAGNUM_MASK;

    /*
     * XXX This code does not (yet) handle the high-tag-number form!
     */
    if (tagnum == DER_HIGH_TAG_NUMBER) {
        PORT_SetError(SEC_ERROR_BAD_DER);
	return -1;
    }

#ifdef RAW_BYTES
    rv = prettyPrintByte(out, code, level);
#else
    rv = prettyIndent(out, level);
#endif
    if (rv < 0)
	return rv;

    if (code & DER_CONSTRUCTED) {
        rv = fprintf(out, "C-");
	if (rv < 0) {
	    PORT_SetError(SEC_ERROR_IO);
	    return rv;
	}
    }

    switch (code & DER_CLASS_MASK) {
    case DER_UNIVERSAL:
        rv = fprintf(out, "%s ", prettyTagType[tagnum]);
	break;
    case DER_APPLICATION:
        rv = fprintf(out, "Application: %d ", tagnum);
	break;
    case DER_CONTEXT_SPECIFIC:
        rv = fprintf(out, "[%d] ", tagnum);
	break;
    case DER_PRIVATE:
        rv = fprintf(out, "Private: %d ", tagnum);
	break;
    }

    if (rv < 0) {
        PORT_SetError(SEC_ERROR_IO);
	return rv;
    }

    *codep = code;

    return 1;
}

static int
prettyPrintLength(FILE *out, unsigned char *data, unsigned char *end,
		  int *lenp, PRBool *indefinitep, unsigned int lv)
{
    unsigned char lbyte;
    int lenLen;
    int rv;

    if (data >= end) {
	PORT_SetError(SEC_ERROR_BAD_DER);
	return -1;
    }

    *indefinitep = PR_FALSE;

    lbyte = *data++;
    if (lbyte >= 0x80) {
	/* Multibyte length */
	unsigned nb = (unsigned) (lbyte & 0x7f);
	if (nb > 4) {
	    PORT_SetError(SEC_ERROR_BAD_DER);
	    return -1;
	}
	if (nb > 0) {
	    int il;

	    if ((data + nb) > end) {
		PORT_SetError(SEC_ERROR_BAD_DER);
		return -1;
	    }
	    il = getInteger256(data, nb);
	    if (il < 0) return -1;
	    *lenp = (unsigned) il;
	} else {
	    *lenp = 0;
	    *indefinitep = PR_TRUE;
	}
	lenLen = nb + 1;
#ifdef RAW_BYTES
	rv = prettyPrintByte(out, lbyte, lv);
	if (rv < 0)
	    return rv;
	for (i = 0; i < nb; i++) {
	    rv = prettyPrintByte(out, data[i], lv);
	    if (rv < 0)
		return rv;
	}
#endif
    } else {
	*lenp = lbyte;
	lenLen = 1;
#ifdef RAW_BYTES
	rv = prettyPrintByte(out, lbyte, lv);
	if (rv < 0)
	    return rv;
#endif
    }
    if (*indefinitep)
	rv = fprintf(out, " (indefinite)\n");
    else
	rv = fprintf(out, " (%d)\n", *lenp);
    if (rv < 0) {
        PORT_SetError(SEC_ERROR_IO);
	return rv;
    }

    prettyColumn = -1;
    return lenLen;
}

static int
prettyPrintItem(FILE *out, unsigned char *data, unsigned char *end,
		unsigned int lv)
{
    int slen;
    int lenLen;
    unsigned char *orig = data;
    int rv;

    while (data < end) {
        unsigned char code;
	PRBool indefinite;

	slen = prettyPrintTag(out, data, end, &code, lv);
	if (slen < 0)
	    return slen;
	data += slen;

	lenLen = prettyPrintLength(out, data, end, &slen, &indefinite, lv);
	if (lenLen < 0)
	    return lenLen;
	data += lenLen;

	/*
	 * Just quit now if slen more bytes puts us off the end.
	 */
	if ((data + slen) > end) {
	    PORT_SetError(SEC_ERROR_BAD_DER);
	    return -1;
	}

        if (code & DER_CONSTRUCTED) {
	    if (slen > 0 || indefinite) {
		slen = prettyPrintItem(out, data,
				       slen == 0 ? end : data + slen, lv+1);
		if (slen < 0)
		    return slen;
		data += slen;
	    }
	} else if (code == 0) {
	    if (slen != 0 || lenLen != 1) {
		PORT_SetError(SEC_ERROR_BAD_DER);
		return -1;
	    }
	    break;
	} else {
	    if (code == DER_PRINTABLE_STRING || code == DER_UTC_TIME) {
	        rv = prettyPrintString(out, data, slen, lv+1);
		if (rv < 0)
		    return rv;
#ifdef RAW_BYTES
	        rv = prettyPrintLeaf(out, data, slen, lv+1);
		if (rv < 0)
		    return rv;
#endif
	    } else if (code == DER_OBJECT_ID) {
	        rv = prettyPrintObjectID(out, data, slen, lv+1);
		if (rv < 0)
		    return rv;
	    } else {
	        rv = prettyPrintLeaf(out, data, slen, lv+1);
		if (rv < 0)
		    return rv;
	    }
	    data += slen;
	}
    }

    rv = prettyNewline(out);
    if (rv < 0)
	return rv;

    return data - orig;
}

SECStatus
DER_PrettyPrint(FILE *out, SECItem *it)
{
    int rv;

    prettyColumn = -1;

    rv = prettyPrintItem(out, it->data, it->data + it->len, 0);
    if (rv < 0)
	return SECFailure;
    return SECSuccess;
}
