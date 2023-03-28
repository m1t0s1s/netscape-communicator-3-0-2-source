/*************************************************************************************
 *
 * Copyright (c) 1996 Netscape Communications Corp.  All rights reserved.
 *
 * Description:
 *  
 *
 * Authors:
 *  Brian Beck <bcbeck@netscape.com>    Feb 22, 1996     created this file.
 *
 *************************************************************************************/

#include "libi18n.h"
#include "awt_p.h"
#include "java_awt_Font.h"
#include "ustring.h"
#include <sys/time.h>
   
int16         awt_LocaleCharSetID;


/*************************************************************************************
 *
 * Description:
 *  Creates a Motif compound string (XmString) from a Java string.  The Java string's
 *  Unicode is converted piecewise into a series of new encodings.  The new encodings
 *  are chosen to match the non-Unicode fonts that are available on the system.
 *
 *************************************************************************************/

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


/*************************************************************************************
 *
 * Description:
 *  Creates a Motif compound string (XmString) from an array of Unicode characters.
 *  The Unicode is converted piecewise into a series of new encodings.  The new
 *  encodings are chosen to match the non-Unicode fonts that are available on the
 *  system.
 *
 *************************************************************************************/

XmString  makeXmStringFromChars( unicode* uniChars, long length )
{
   INTL_CompoundStr*        compoundString;
   INTL_Encoding_ID         encodingID;
   unsigned char*           stringSegment;
   INTL_CompoundStrIterator iter;
   XmString                 xString;
   char                     tag[48];

   
   /* Many awt routines seem to use the convention that NULL pointers to Java strings
    * should be treated as empty strings.  This routine preserves that convention.
    */   
   if( uniChars == NULL )
   {
      unicode emptyString = 0;
      compoundString = INTL_CompoundStrFromUnicode( &emptyString, 0 );
   }
   else
   {
      compoundString = INTL_CompoundStrFromUnicode( uniChars, length );
   }

   if( compoundString == 0 )
   {
      SignalError(0, JAVAPKG "OutOfMemoryError", 0);
      return (XmString)NULL;
   }      
   

   /* Create an XmString from the INTL_CompoundString */
   iter = INTL_CompoundStrFirstStr( compoundString, &encodingID, &stringSegment );
   if( iter == NULL )
   {
      SignalError(0, JAVAPKG "OutOfMemoryError", 0);
      return (XmString)NULL;
   }      
   INTL_CharSetIDToName( encodingID, tag );
   xString = XmStringCreate( stringSegment, tag );
   
   while((iter = INTL_CompoundStrNextStr( iter, &encodingID, &stringSegment ) ))
   {
      XmString tmpXString1;
      XmString tmpXString2;

      INTL_CharSetIDToName( encodingID, tag );

      tmpXString1 = XmStringCreate( stringSegment, tag );
      tmpXString2 = XmStringConcat( xString, tmpXString1 );
      XmStringFree( xString );
      XmStringFree(tmpXString1);

      xString = tmpXString2;
   }
   
   INTL_CompoundStrDestroy(compoundString);        

   /* return the XmString */
   return xString;
}

/*************************************************************************************
 *
 * Description:
 *  Creates a Motif font list representing all the fonts needed to display the Unicode
 *  character set.
 *
 *************************************************************************************/

char* makeLocalizedCString( Hjava_lang_String* javaStringHandle )
{
   unicode*             uniChars;
   long                 uniCharsLength;
   unicode              emptyString = 0;
   char*                cString;
   long                 approxCStringLength;
   HArrayOfByte*        cStringHandle;
   extern HArrayOfByte* MakeByteString(char *str, long len);

   
   /* Many awt routines seem to use the convention that NULL pointers to Java strings
    * should be treated as empty strings.  This routine preserves that convention.
    */
   if( javaStringHandle == NULL )
   {
      uniChars = &emptyString;
      uniCharsLength = 0;
   }
   else
   {
      Classjava_lang_String* javaStringClass = unhand( javaStringHandle );
      HArrayOfChar* unicodeArrayHandle = (HArrayOfChar *)javaStringClass->value;
      uniChars =  unhand(unicodeArrayHandle)->body + javaStringClass->offset;
      uniCharsLength = javaStringClass->count;
   }

   /* Get an upper bound on the length of the converted string. */
   approxCStringLength = INTL_UnicodeToStrLen( awt_LocaleCharSetID, uniChars,
                                               uniCharsLength );
   
   cStringHandle = MakeByteString( NULL, approxCStringLength );
   if (cStringHandle == 0)
   {
      SignalError(0, JAVAPKG "OutOfMemoryError", 0);
      return NULL;
   }
   cString = unhand(cStringHandle)->body;
   
   INTL_UnicodeToStr( awt_LocaleCharSetID, uniChars, uniCharsLength,
                      cString, approxCStringLength );
   
   return cString;
}



/*************************************************************************************
 *
 * Description:
 *
 *************************************************************************************/

Hjava_lang_String*  makeJavaStringFromLocalizedCString( char* cString )
{
   HArrayOfChar* uniCharsHandle;
   unicode*      uniChars;
   long          uniCharsBufferLength;
   long          uniCharsStringLength;

   if( cString == NULL )
      uniCharsBufferLength = 0;
   else
      uniCharsBufferLength = INTL_StrToUnicodeLen( awt_LocaleCharSetID, cString );
   
   uniCharsHandle = (HArrayOfChar*) ArrayAlloc( T_CHAR, uniCharsBufferLength );
   if(uniCharsHandle == 0)
   {
      SignalError(0, JAVAPKG "OutOfMemoryError", 0);
      return 0;
   }
   
   uniChars = unhand( uniCharsHandle )->body;
   
   if( cString )
   {
      uniCharsStringLength = INTL_StrToUnicode( awt_LocaleCharSetID, cString, uniChars,
                                                uniCharsBufferLength );
   }
   
   return (Hjava_lang_String *)
      execute_java_constructor(EE(), 0, classJavaLangString, "([CII)",
                               uniCharsHandle, 0, uniCharsStringLength);
}



/* Some forward declarations needed at this  point. */

static void  getXFontFamilyAndFoundry( char* javaFontName, int16 csid,
                                       char** family, char** foundry );

static XFontSet makeXFontSet( Display* display, char* javaFamily, int size, int style );

XFontStruct*  INTL_GetFont( Display*, int16 , char*, char*, int, int );

int16*  INTL_GetXCharSetIDs(Display *display);

char* INTL_GetXFontName(Display *display, int16 characterSet, char *familyName,
                        char *foundry, int sizeNum, int style);

/*************************************************************************************
 *
 * Description:
 *  Creates a Motif font list representing all the fonts needed to display the Unicode
 *  character set.
 *
 *************************************************************************************/

XmFontList makeXmFontList( struct Hjava_awt_Font* javaFontHandle, XFontStruct *xfont)
{
   Classjava_awt_Font*   javaFont;
   char*                 javaFamily;
   int16*                csidList;
   int16                 csidListLength;
   int16                 i;
   XmFontList            fontList = NULL; 
   XFontSet              fontSet;
   XmFontListEntry       fontEntry;
   Display*              display;

   
   if( javaFontHandle == NULL )
   {
      /* *errmsg = JAVAPKG "NullPointerException"; */
      return (XmFontList) NULL;
   }
   
   javaFont = unhand( javaFontHandle );
   javaFamily = makeCString( javaFont->family );

   display = XDISPLAY;
   
   csidList = INTL_GetUnicodeCSIDList( &csidListLength );
   for( i=0; i<csidListLength; i++ )
   {
      char*            family;
      char*            foundry;
      XFontStruct*     fontStruct;
      char             tag[48];   

      getXFontFamilyAndFoundry( javaFamily, csidList[i], &family, &foundry );
      fontStruct = INTL_GetFont( display, csidList[i], family, foundry,
                                 javaFont->size, javaFont->style );
      if( fontStruct == NULL )
      {
         /* Some error */
         return (XmFontList) NULL;
      }

      /* Add this thing to our fontlist */
      INTL_CharSetIDToName( csidList[i], tag );
      fontEntry = XmFontListEntryCreate( tag, XmFONT_IS_FONT, fontStruct );
      fontList = XmFontListAppendEntry( fontList, fontEntry );

      XmFontListEntryFree(&fontEntry);
   }

   /* Now create an XFontSet for the widgets that can only handle a single locale. */
   fontSet = makeXFontSet( display, javaFamily, javaFont->size, javaFont->style );
   fontEntry = XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONTSET, fontSet);
   fontList = XmFontListAppendEntry( fontList, fontEntry );
   XmFontListEntryFree(&fontEntry);

   return fontList;
}

XmFontList makeXmFontListTest( )
{
   char*                 javaFamily;
   int16*                csidList;
   int16                 csidListLength;
   int16                 i;
   XmFontList            fontList = NULL; 
   XFontSet              fontSet;
   XmFontListEntry       fontEntry;
   Display*              display;

   
   javaFamily = "Courier";

   display = XDISPLAY;
   
   csidList = INTL_GetUnicodeCSIDList( &csidListLength );
   for( i=0; i<csidListLength; i++ )
   {
      char*            family;
      char*            foundry;
      XFontStruct*     fontStruct;
      char             tag[48];   

      getXFontFamilyAndFoundry( javaFamily, csidList[i], &family, &foundry );
      fontStruct = INTL_GetFont( display, csidList[i], family, foundry,
                                 12, 0 );
      if( fontStruct == NULL )
      {
         /* Some error */
         return (XmFontList) NULL;
      }

      /* Add this thing to our fontlist */
      INTL_CharSetIDToName( csidList[i], tag );
      fontEntry = XmFontListEntryCreate( tag, XmFONT_IS_FONT, fontStruct );
      fontList = XmFontListAppendEntry( fontList, fontEntry );

      XmFontListEntryFree(&fontEntry);
   }

   /* Now create an XFontSet for the widgets that can only handle a single locale. */
   fontSet = makeXFontSet( display, javaFamily, 12, 0 );
   fontEntry = XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONTSET, fontSet);
   fontList = XmFontListAppendEntry( fontList, fontEntry );
   XmFontListEntryFree(&fontEntry);

   return fontList;
}

static XFontSet
makeXFontSet( Display* display, char* javaFamily, int size, int style )
{
   int16*   csidList;
   int      i;
   char*    fontName;
   char*    fontNameList = NULL;
   char*    newFontNameList;
   char*    family;
   char*    foundry;
   char**   missingCharSets;
   int      missingCharSetCount;
   char*    missingCharString;

   csidList = INTL_GetXCharSetIDs( display );
   if( csidList == NULL )
      return NULL;

   
   for( i=0; csidList[i] != 0; i++ )
   {
      getXFontFamilyAndFoundry( javaFamily, csidList[i], &family, &foundry );
      fontName = INTL_GetXFontName( display, csidList[i], family, foundry, size, style );
      if (!fontName)
      {
	 continue;
      }

      if( fontNameList == NULL )
         newFontNameList = strdup( fontName );
      else
      {
         newFontNameList = PR_smprintf("%s,%s", fontNameList, fontName);
	 if (newFontNameList)
	 {
            free( fontNameList );
	 }
      }
      free( fontName );
      if (!newFontNameList)
      {
         continue;
      }

      fontNameList = newFontNameList;
   }
   free( csidList );
   
   return XCreateFontSet( display, fontNameList, &missingCharSets,
                          &missingCharSetCount, &missingCharString );
}



/* The following tables map Java font names to X font names for various character sets.
 * The tables are indexed by CSID.  Null values indicate "don't care" names.  These  
 * tables are used to implement the function getXFontFamilyAndFoundry and should be
 * considered private to that function.
 */
struct FontInfo
{
   char* foundry;
   char* family;
};

static struct FontInfo  helveticaFonts[INTL_CHAR_SET_MAX ] =
{
   {0,0},
   {"adobe","helvetica"},              /* CS_ASCII */
   {"adobe","helvetica"},              /* CS_LATIN1 */
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},
   {"adobe","helvetica"},              /* CS_LATIN2 */
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,"gothic"},                       /* CS_JISX0208 */
   {0,"gothic"}                        /* CS_JISX0201 */
};

static struct FontInfo  timesRomanFonts[ INTL_CHAR_SET_MAX ] =
{
   {0,0},
   {"adobe","times"},                  /* CS_ASCII */
   {"adobe","times"},                  /* CS_LATIN1 */
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},                                
   {"adobe","times"},                  /* CS_LATIN2 */
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {"daewoo","mincho"},                /* CS_JISX0208 */
   {"daewoo","mincho"}                 /* CS_JISX0201 */
};

static struct FontInfo  courierFonts[ INTL_CHAR_SET_MAX ] =
{
   {0,0},
   {"adobe","courier"},                  /* CS_ASCII */
   {"adobe","courier"},                  /* CS_LATIN1 */
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},                                
   {"adobe","courier"},                  /* CS_LATIN2 */
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {"jis","fixed"},                      /* CS_JISX0208 */
   {"jis","fixed"}                       /* CS_JISX0201 */
};

static struct FontInfo  dialogFonts[ INTL_CHAR_SET_MAX ] =
{
   {0,0},
   {"b&h","lucida"},                   /* CS_ASCII */
   {"b&h","lucida"},                   /* CS_LATIN1 */
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},
   {"b&h","lucida"},                   /* CS_LATIN2 */
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,"gothic"},                       /* CS_JISX0208 */
   {0,"gothic"}                        /* CS_JISX0201 */
};

static struct FontInfo  dialogInputFonts[ INTL_CHAR_SET_MAX ] =
{
   {0,0},
   {"b&h","lucidatypewriter"},         /* CS_ASCII */
   {"b&h","lucidatypewriter"},         /* CS_LATIN1 */
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},                                
   {"b&h","lucidatypewriter"},         /* CS_LATIN2 */
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,"gothic"},                       /* CS_JISX0208 */
   {0,"gothic"}                        /* CS_JISX0201 */
};

static struct FontInfo  zapfDingbatsFonts[ INTL_CHAR_SET_MAX ] =
{
   {0,0},
   {"itc","zapfdingbats"},             /* CS_ASCII */
   {"itc","zapfdingbats"},             /* CS_LATIN1 */
   {0,0},{0,0},{0,0},{0,0},{0,0},                          
   {0,0},{0,0},                                
   {"itc","zapfdingbats"},             /* CS_LATIN2 */
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},                              /* CS_JISX0208 */
   {0,0}                               /* CS_JISX0201 */
};


/*************************************************************************************
 *
 * Description:
 *  This function maps a Java font name to an X font name and foundry for a given
 *  character set.  The X font name and foundry are returned in the fill-in parameters
 *  "family" and "foundry".
 *  
 ************************************************************************************/

static void
getXFontFamilyAndFoundry( char* javaFontName, int16 csid,
                          char** family, char** foundry )
{
   int16 maskedCsid = csid & 0x00ff;
   
   if(strcmp(javaFontName,"Helvetica")==0)
   {
      *family =  helveticaFonts[maskedCsid].family;
      *foundry =  helveticaFonts[maskedCsid].foundry;
   }
   else if(strcmp(javaFontName,"TimesRoman") == 0)
   {
      *family =  timesRomanFonts[maskedCsid].family;
      *foundry =  timesRomanFonts[maskedCsid].foundry;
   }
   else if(strcmp(javaFontName,"Courier") == 0)
   {
      *family =  courierFonts[maskedCsid].family;
      *foundry =  courierFonts[maskedCsid].foundry;
   }
   else if(strcmp(javaFontName,"Dialog") == 0)
   {
      *family =  dialogFonts[maskedCsid].family;
      *foundry =  dialogFonts[maskedCsid].foundry;
   }
   else if(strcmp(javaFontName,"DialogInput") == 0)
   {
      *family =  dialogInputFonts[maskedCsid].family;
      *foundry =  dialogInputFonts[maskedCsid].foundry;
   }
   else if(strcmp(javaFontName,"ZapfDingbats") == 0)
   {
      *family =  zapfDingbatsFonts[maskedCsid].family;
      *foundry =  zapfDingbatsFonts[maskedCsid].foundry;
   }
   else
   {
      *family  = NULL;
      *foundry = NULL;
   }
}


void printFontInfo( Widget widget )
{
   XmFontList      fontList;
   XmFontContext   context;
   XmFontListEntry fontListEntry;
   
   XtVaGetValues(widget,  XmNfontList, &fontList,  NULL);

   XmFontListInitFontContext( &context, fontList );
   while ((fontListEntry = XmFontListNextEntry(context) ))
   {
      char*       tag;
      XtPointer   font;
      XmFontType  type;

      tag = XmFontListEntryGetTag( fontListEntry );
      fprintf(stderr,"Font Tag: %s ", tag);
      font = XmFontListEntryGetFont( fontListEntry, &type );
      if( type == XmFONT_IS_FONTSET )
      {
         XFontStruct** fontStructList;
         char**        fontNameList;
         int           length;
         int           i;

         length = XFontsOfFontSet( font, &fontStructList, &fontNameList );
         
         fprintf(stderr,"is a fontset.\n");
         for( i=0; i<length; i++ )
            fprintf(stderr,"\t%s\n",fontNameList[i]);
      }
      else if( type == XmFONT_IS_FONT )
      {
         fprintf(stderr,"is a font.\n");
      }
      else
      {
         fprintf(stderr,"is ???.\n");
      }
      
 
      
   }
   
   XmFontListFreeFontContext( context );
   
}

void printAncestry( Widget widget )
{
   Widget parent;
   int    ancestor=1;

   fprintf(stderr, "Target Widget: %s\n", widget);
   
   parent = XtParent(widget);
   while( ( parent!=NULL ) && (parent != widget) )
   {
      fprintf(stderr,"Ancestor %d: %s\n", ancestor++, XtName(parent) );
      widget = parent;
      parent = XtParent(widget);
   }
}
