#include "awt_p.h"
#include "java_awt_Font.h"
#include "ustring.h"

/*
 */
XmString  makeXmString( Hjava_lang_String* javaStringHandle )
{
   long        length;
   unicode*    uniChars;
   unicode     emptyString = 0;

   /* Many awt routines seem to use the convention that NULL pointers to Java strings
    * should be treated as empty strings.  This routine preserves that convention.
    */
   if( javaStringHandle == NULL )
   {
      uniChars = &emptyString;
      length = 0;
   }
   else
   {
      Classjava_lang_String* javaStringClass = unhand( javaStringHandle );
      HArrayOfChar* unicodeArrayHandle = (HArrayOfChar *)javaStringClass->value;
      uniChars =  unhand(unicodeArrayHandle)->body + javaStringClass->offset;
      length = javaStringClass->count;
   }

   return makeXmStringFromChars( uniChars, length );
}

XmString  makeXmStringFromChars( unicode* uniChars, long length )
{
    XmString xs = NULL;
    char *foo = (char*) malloc(length+1);
    if (foo) {
	long l;
	for (l = 0; l < length; l++) {
	    foo[l] = uniChars[l];
	}
	foo[l] = 0;
	xs = XmStringCreateLocalized(foo);
	/* XXX does Xm keep foo or does it copy it? only purify knows */
    }
    return xs;
}

Hjava_lang_String*  makeJavaStringFromLocalizedCString( char* cString )
{
    return makeJavaString(cString, strlen(cString));
}

char* makeLocalizedCString( Hjava_lang_String* javaStringHandle )
{
    if (javaStringHandle) {
	return makeCString(javaStringHandle);
    }
    return strdup("");
}

XmFontList makeXmFontList(struct Hjava_awt_Font* jfont, XFontStruct *xfont)
{
    XmFontList fontList;
    XmFontListEntry fontEntry;

    fontEntry = XmFontListEntryCreate("iso-8859-1", XmFONT_IS_FONT, xfont);
    fontList = XmFontListAppendEntry(NULL, fontEntry);
    return fontList;
}
