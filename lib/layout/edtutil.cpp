/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */

//
// Public interface and shared subsystem data.
//

#ifdef EDITOR

#include "editor.h"

// Timer


void CEditTimerCallback (void * closure){
	if (closure) {
        CEditTimer* pTimer = (CEditTimer*) closure;
        pTimer->Callback();
    }
}

CEditTimer::CEditTimer(){
    m_timeoutEnabled = FALSE;
    m_timerID = NULL;
}

CEditTimer::~CEditTimer(){
    Cancel();
}

void CEditTimer::Callback() {
    m_timeoutEnabled = FALSE;
    OnCallback();
}

void CEditTimer::Cancel(){
    if ( m_timeoutEnabled ) {
        FE_ClearTimeout(m_timerID);
        m_timeoutEnabled = FALSE;
        m_timerID = NULL;
    }
}

void CEditTimer::OnCallback() {}

void CEditTimer::SetTimeout(uint32 msecs){
    Cancel();
	m_timerID = FE_SetTimeout(&CEditTimerCallback, this, msecs);
    m_timeoutEnabled = TRUE;
}

// Color with a defined/undefined boolean

ED_Color::ED_Color() {
	m_bDefined = TRUE;
    m_color.red = 0;
    m_color.green = 0;
    m_color.blue = 0;
}
ED_Color::ED_Color(LO_Color& pLoColor) {
    m_bDefined = TRUE;
    m_color = pLoColor;
}

static int ED_Color_Clip(int c) {
    if ( c < 0 ) {
        XP_ASSERT(FALSE);
        c = 0;
    }
    if ( c > 255 ) {
        XP_ASSERT(FALSE);
        c = 255;
    }
    return c;
}

ED_Color::ED_Color(int r, int g, int b) {
    m_bDefined = TRUE;
    m_color.red = ED_Color_Clip(r);
    m_color.green = ED_Color_Clip(g);
    m_color.blue = ED_Color_Clip(b);
}

ED_Color::ED_Color(int32 rgb) {
    if ( rgb == -1 ) {
        m_bDefined = FALSE;
        m_color.red = 0;
        m_color.green = 0;
        m_color.blue = 0;
    }
    else {
        m_bDefined = TRUE;
        m_color.red = (rgb >> 16) & 255;
        m_color.green = (rgb >> 8) & 255;
        m_color.blue = rgb & 255;
    }
}

ED_Color::ED_Color(LO_Color* pLoColor) {
    if ( pLoColor ) {
        m_color.red = ED_Color_Clip(pLoColor->red);
        m_color.green = ED_Color_Clip(pLoColor->green);
        m_color.blue = ED_Color_Clip(pLoColor->blue);
        m_bDefined = TRUE;
    }
    else {
        m_bDefined = FALSE;
    }
}

XP_Bool ED_Color::operator==(const ED_Color& other) const {
    return m_bDefined == other.m_bDefined &&
        (!m_bDefined || m_color.red == other.m_color.red &&
        m_color.green == other.m_color.green &&
        m_color.blue == other.m_color.blue );
}

XP_Bool ED_Color::operator!=(const ED_Color& other) const {
    return ! operator==(other);
}

XP_Bool ED_Color::IsDefined() { return m_bDefined; }
int ED_Color::Red() { XP_ASSERT(m_bDefined); return m_color.red; }
int ED_Color::Green() { XP_ASSERT(m_bDefined); return m_color.green; }
int ED_Color::Blue() { XP_ASSERT(m_bDefined); return m_color.blue; }
LO_Color ED_Color::GetLOColor() {XP_ASSERT(m_bDefined); return m_color; }

long ED_Color::GetAsLong() {
    if ( ! m_bDefined ) return -1;
    return (((long)m_color.red << 16) | ((long)m_color.green << 8) | (long)m_color.blue);
}

void
ED_Color::SetUndefined() {
	m_bDefined = FALSE;
    m_color.red = 0;
    m_color.green = 0;
    m_color.blue = 0;
}

ED_Color 
ED_Color::GetUndefined()
{
	ED_Color undef;
	undef.SetUndefined();
	return undef;
}

//-----------------------------------------------------------------------------
// Helper functions.
//-----------------------------------------------------------------------------

//
// Convert a tagtype to a string
//

static char* TagString(int32 tagType)
{
    switch(tagType)
    {
        case P_UNKNOWN:
            return "";
        case P_TEXT:
            return "TEXT";
        case P_TITLE:
            return PT_TITLE;
        case P_INDEX:
            return PT_INDEX;
        case P_BASE:
            return PT_BASE;
        case P_LINK:
            return PT_LINK;
        case P_HEADER_1:
            return PT_HEADER_1;
        case P_HEADER_2:
            return PT_HEADER_2;
        case P_HEADER_3:
            return PT_HEADER_3;
        case P_HEADER_4:
            return PT_HEADER_4;
        case P_HEADER_5:
            return PT_HEADER_5;
        case P_HEADER_6:
            return PT_HEADER_6;
        case P_ANCHOR:
            return PT_ANCHOR;
        case P_PARAGRAPH:
            return PT_PARAGRAPH;
        case P_ADDRESS:
            return PT_ADDRESS;
        case P_IMAGE:
            return PT_IMAGE;
        case P_NEW_IMAGE:
            return PT_NEW_IMAGE;
        case P_PLAIN_TEXT:
            return PT_PLAIN_TEXT;
        case P_PLAIN_PIECE:
            return PT_PLAIN_PIECE;
        case P_PREFORMAT:
            return PT_PREFORMAT;
        case P_LISTING_TEXT:
            return PT_LISTING_TEXT;
        case P_UNUM_LIST:
            return PT_UNUM_LIST;
        case P_NUM_LIST:
            return PT_NUM_LIST;
        case P_MENU:
            return PT_MENU;
        case P_DIRECTORY:
            return PT_DIRECTORY;
        case P_LIST_ITEM:
            return PT_LIST_ITEM;
        case P_DESC_LIST:
            return PT_DESC_LIST;
        case P_DESC_TITLE:
            return PT_DESC_TITLE;
        case P_DESC_TEXT:
            return PT_DESC_TEXT;
        case P_STRIKEOUT:
            return PT_STRIKEOUT;
        case P_UNDERLINE:
            return PT_UNDERLINE;
        case P_FIXED:
            return PT_FIXED;
        case P_CENTER:
            return PT_CENTER;
        case P_EMBED:
            return PT_EMBED;
        case P_SUBDOC:
            return PT_SUBDOC;
        case P_CELL:
            return PT_CELL;
        case P_TABLE:
            return PT_TABLE;
        case P_CAPTION:
            return PT_CAPTION;
        case P_TABLE_ROW:
            return PT_TABLE_ROW;
        case P_TABLE_HEADER:
            return PT_TABLE_HEADER;
        case P_TABLE_DATA:
            return PT_TABLE_DATA;
        case P_BOLD:
            return PT_BOLD;
        case P_ITALIC:
            return PT_ITALIC;
        case P_EMPHASIZED:
            return PT_EMPHASIZED;
        case P_STRONG:
            return PT_STRONG;
        case P_CODE:
            return PT_CODE;
        case P_SAMPLE:
            return PT_SAMPLE;
        case P_KEYBOARD:
            return PT_KEYBOARD;
        case P_VARIABLE:
            return PT_VARIABLE;
        case P_CITATION:
            return PT_CITATION;
        case P_BLOCKQUOTE:
            return PT_BLOCKQUOTE;
        case P_FORM:
            return PT_FORM;
        case P_INPUT:
            return PT_INPUT;
        case P_SELECT:
            return PT_SELECT;
        case P_OPTION:
            return PT_OPTION;
        case P_TEXTAREA:
            return PT_TEXTAREA;
        case P_HRULE:
            return PT_HRULE;
        case P_BODY:
            return PT_BODY;
        case P_COLORMAP:
            return PT_COLORMAP;
        case P_HYPE:
            return PT_HYPE;
        case P_META:
            return PT_META;
        case P_LINEBREAK:
            return PT_LINEBREAK;
        case P_WORDBREAK:
            return PT_WORDBREAK;
        case P_NOBREAK:
            return PT_NOBREAK;
        case P_BASEFONT:
            return PT_BASEFONT;
        case P_FONT:
            return PT_FONT;
        case P_BLINK:
            return PT_BLINK;
        case P_BIG:
            return PT_BIG;
        case P_SMALL:
            return PT_SMALL;
        case P_SUPER:
            return PT_SUPER;
        case P_SUB:
            return PT_SUB;
        case P_GRID:
            return PT_GRID;
        case P_GRID_CELL:
            return PT_GRID_CELL;
        case P_NOGRIDS:
            return PT_NOGRIDS;
        case P_JAVA_APPLET:
            return PT_JAVA_APPLET;
        case P_MAP:
            return PT_MAP;
        case P_AREA:
            return PT_AREA;
        case P_DIVISION:
            return PT_DIVISION;
        case P_KEYGEN:
            return PT_KEYGEN;
        case P_MAX:
            return "HTML";
        case P_SCRIPT:
            return PT_SCRIPT;
        case P_NOEMBED:
            return PT_NOEMBED;
        case P_SERVER:
            return PT_SERVER;
        case P_PARAM:
            return PT_PARAM;
        case P_MULTICOLUMN:
            return PT_MULTICOLUMN;
        case P_SPACER:
            return PT_SPACER;
        case P_NOSCRIPT:
            return PT_NOSCRIPT;
        case P_CERTIFICATE:
            return PT_CERTIFICATE;
        case P_PAYORDER:
            return PT_PAYORDER;
        default:
            XP_ASSERT(FALSE);
            return "UNKNOWN";
    }
}

const int kTagBufferSize = 40;
static char tagBuffer[kTagBufferSize];

char* EDT_UpperCase(char* tagString)
{
    int i = 0;
    if ( tagString ) {
        for ( i = 0; i < kTagBufferSize-1 && tagString[i] != '\0'; i++ ) {
            tagBuffer[i] = XP_TO_UPPER(tagString[i]);
        }
    }
    tagBuffer[i] = '\0';
    return tagBuffer;
}

char *EDT_TagString(int32 tagType){
    return EDT_UpperCase(TagString(tagType));
}

char* EDT_AlignString(ED_Alignment align){
    if ( align < ED_ALIGN_CENTER ) {
        XP_ASSERT(FALSE);
        align = ED_ALIGN_CENTER;
    }
    if ( align > ED_ALIGN_ABSTOP ) {
        XP_ASSERT(FALSE);
        align = ED_ALIGN_ABSTOP;
    }
    return EDT_UpperCase(lo_alignStrings[align]);
}

ED_ETextFormat edt_TagType2TextFormat( TagType t ){
    switch(t){
    case P_BOLD:
    case P_STRONG:
        return TF_BOLD;

    case P_ITALIC:
    case P_EMPHASIZED:
    case P_VARIABLE:
    case P_CITATION:
        return TF_ITALIC;

    case P_FIXED:
    case P_CODE:
    case P_KEYBOARD:
    case P_SAMPLE:
        return TF_FIXED;

    case P_SUPER:
        return TF_SUPER;

    case P_SUB:
        return TF_SUB;

    case P_STRIKEOUT:
        return TF_STRIKEOUT;

    case P_UNDERLINE:
        return TF_UNDERLINE;

    case P_BLINK:
        return TF_BLINK;

    case P_SERVER:
        return TF_SERVER;

    case P_SCRIPT:
        return TF_SCRIPT;
    }
    return TF_NONE;
}

static int workBufSize = 0;
static char* workBuf = 0;

char *edt_WorkBuf( int iSize ){
    if( iSize > workBufSize ){
        if( workBuf != 0 ){
            delete workBuf;
        }
        workBuf = new char[ iSize ];
    }
    return workBuf;
}

char *edt_QuoteString( char* pString ){
    int iLen = 0;
    char *p = pString;
    while( p && *p  ){
        if( *p == '"' ){
            iLen += 6;    // &quot;
        }
        else {
            iLen++;
        }
        p++;
    }
    iLen++;
    
    char *pRet = edt_WorkBuf( iLen+1 );
    p = pRet;
    while( pString && *pString ){
        if( *pString == '"' ){
            strcpy( p, "&quot;" );
            p += 6;
        }
        else {
            *p++ = *pString;
        }
        pString++;
    }
    *p = 0;
    return pRet;
}


char *edt_MakeParamString( char* pString ){
    int iLen = 0;
    char *p = pString;
    while( p && *p  ){
        if( *p == '"' ){
            iLen += 6;    // &quot;
        }
        else {
            iLen++;
        }
        p++;
    }
    iLen += 3;      // the beginning and ending quote and the trailing 0.
    
    char *pRet = edt_WorkBuf( iLen+1 );
    p = pRet;
    if( pString && *pString == '`' ){
        do {
            *p++ = *pString++;
        } while( *pString && *pString != '`' );
        *p++ = '`';
        *p = 0;
    }
    else {
        *p = '"';
        p++;
        while( pString && *pString ){
            if( *pString == '"' ){
                strcpy( p, "&quot;" );
                p += 6;
            }
            else {
                *p++ = *pString;
            }
            pString++;
        }
        *p++ = '"';
        *p = 0;
    }
    return pRet;
}

char *edt_FetchParamString( PA_Tag *pTag, char* param ){
    PA_Block buff;
    char *str;
    char *retVal = 0;

    buff = PA_FetchParamValue(pTag, param);
    if (buff != NULL)
    {
        PA_LOCK(str, char *, buff);
        lo_StripTextWhitespace(str, XP_STRLEN(str));
        if( str ) {
            retVal = XP_STRDUP( str );
        }
        PA_UNLOCK(buff);
        PA_FREE(buff);
    }
    return retVal;
}

XP_Bool edt_FetchParamBoolExist( PA_Tag *pTag, char* param ){
    PA_Block buff;

    buff = PA_FetchParamValue(pTag, param);
    if (buff != NULL){
        return TRUE;
    }
    else {
        return FALSE;
    }
}

XP_Bool edt_FetchDimension( PA_Tag *pTag, char* param, 
                         int32 *pValue, XP_Bool *pPercent,
                         int32 nDefaultValue, XP_Bool bDefaultPercent ){
    PA_Block buff;
    char *str;
    char *retVal = 0;
    int32   value;

    // Fill in defaults
    *pValue = nDefaultValue;
    *pPercent = bDefaultPercent;
    
    buff = PA_FetchParamValue(pTag, param);
    XP_Bool result = buff != NULL;
    if( buff != NULL )
    {
        PA_LOCK(str, char *, buff);
        if( str != 0 ){
            value = XP_ATOI(str);
            *pValue = value;
            *pPercent = (NULL != XP_STRCHR(str, '%'));
        }
        PA_UNLOCK(buff);
        PA_FREE(buff);
    }
    return result;
}
    
ED_Alignment edt_FetchParamAlignment( PA_Tag* pTag, ED_Alignment eDefault, XP_Bool bVAlign ){
    PA_Block buff;
    char *str;
    ED_Alignment retVal = eDefault;

    buff = PA_FetchParamValue(pTag, bVAlign ? PARAM_VALIGN : PARAM_ALIGN);
    if (buff != NULL)
    {
        XP_Bool floating;

        floating = FALSE;
        PA_LOCK(str, char *, buff);
        lo_StripTextWhitespace(str, XP_STRLEN(str));
        // LTNOTE: this is a hack.  We should do a better job maping these
        //  could cause problems in the future.
        retVal = (ED_Alignment) lo_EvalAlignParam(str, &floating);
        PA_UNLOCK(buff);
        PA_FREE(buff);
    }
    return retVal;
}

int32 edt_FetchParamInt( PA_Tag *pTag, char* param, int32 defValue ){
    PA_Block buff;
    char *str;
    int32 retVal = defValue;

    buff = PA_FetchParamValue(pTag, param);
    if (buff != NULL)
    {
        PA_LOCK(str, char *, buff);
        if( str != 0 ){
            retVal = XP_ATOI(str);
        }
        PA_UNLOCK(buff);
        PA_FREE(buff);
    }
    return retVal;
}

ED_Color edt_FetchParamColor( PA_Tag *pTag, char* param ){
    PA_Block buff = PA_FetchParamValue(pTag, param);
    ED_Color retVal;
    retVal.SetUndefined();
    if (buff != NULL)
    {
        char *color_str;
        PA_LOCK(color_str, char *, buff);
        lo_StripTextWhitespace(color_str, XP_STRLEN(color_str));
        uint8 red, green, blue;
        LO_ParseRGB(color_str, &red, &green, &blue);
        retVal = EDT_RGB(red,green,blue);
        PA_UNLOCK(buff);
        PA_FREE(buff);
    }
    return retVal;
}

XP_Bool edt_NameInList( char* pName, char** ppNameList ){
    int i = 0;
    while( ppNameList && ppNameList[i] ){
        if( XP_STRCASECMP( pName, ppNameList[i]) == 0 ){
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

char* edt_FetchParamExtras( PA_Tag *pTag, char**ppKnownParams ){
    char** ppNames;
    char** ppValues;
    char* pRet = 0;
    int i = 0;
    char *pSpace = "";
    int iMax;

    iMax = PA_FetchAllNameValues( pTag, &ppNames, &ppValues );

    while( i < iMax ){
        if( !edt_NameInList( ppNames[i], ppKnownParams ) ){
            if( ppValues[i] ){
                pRet = PR_sprintf_append( pRet, "%s%s=%s",
                    pSpace, ppNames[i], edt_MakeParamString( ppValues[i] ) );
            }
            else {
                pRet = PR_sprintf_append( pRet, "%s%s", pSpace, ppNames[i] );
            }
            pSpace = " ";
        }
        i++;
    }

    i = 0;
    while( i < iMax ){
        if( ppNames[i] ) XP_FREE( ppNames[i] );
        if( ppValues[i] ) XP_FREE( ppValues[i] );
        i++;
    }

    if( ppNames ) XP_FREE( ppNames );
    if( ppValues ) XP_FREE( ppValues );

    return pRet;
}

// Override existing.
void edt_FetchParamString2(PA_Tag* pTag, char* param, char*& data){
    char* result = edt_FetchParamString( pTag, param );
    if ( result ) {
        if ( data ){
            XP_FREE(data);
        }
        data = result;
    }
}

// Override existing.
void edt_FetchParamColor2( PA_Tag *pTag, char* param, ED_Color& data ){
    ED_Color result = edt_FetchParamColor(pTag, param);
    if ( result.IsDefined() ) {
        data = result;
    }
}

// Append to existing extras. To Do: Have new versions of a parameter overwrite the old ones.
void edt_FetchParamExtras2( PA_Tag *pTag, char**ppKnownParams, char*& data){
    char* result = edt_FetchParamExtras( pTag, ppKnownParams);
    if ( result ) {
        if ( data ) {
            data = PR_sprintf_append(data, " %s", result);
            XP_FREE(result);
        }
        else {
            data = result;
        }
    }
}

LO_Color* edt_MakeLoColor(ED_Color c) {
    if( !c.IsDefined()){
        return 0;
    }
    LO_Color *pColor = XP_NEW( LO_Color );
    pColor->red = EDT_RED(c);
    pColor->green = EDT_GREEN(c);
    pColor->blue = EDT_BLUE(c);
    return pColor;
}

void edt_SetLoColor( ED_Color c, LO_Color *pColor ){
    if( !c.IsDefined()){
        XP_ASSERT(FALSE);
        return;
    }
    pColor->red = EDT_RED(c);
    pColor->green = EDT_GREEN(c);
    pColor->blue = EDT_BLUE(c);
}


#define CHARSET_SIZE    256
static PA_AmpEsc *ed_escapes[CHARSET_SIZE];

void edt_InitEscapes(MWContext *pContext){
    PA_AmpEsc *pEsc = PA_AmpEscapes;

    XP_BZERO( ed_escapes, CHARSET_SIZE * sizeof( PA_AmpEsc* ) );


	int csid = INTL_DefaultWinCharSetID(pContext);

    while( pEsc->value ){
        if( ed_escapes[ (unsigned char) pEsc->value ] == 0 ){
			char ch = pEsc->value;

            if (csid == CS_MAC_ROMAN || csid == CS_LATIN1)
                // For MacRoman, use the table to convert 8bit char to Latin1
                // Name entities as much as possible.
           		ed_escapes[ (unsigned char)ch ] = pEsc;
			else if ((unsigned char)ch <= 0x7F)
           		ed_escapes[ (unsigned char)ch ] = pEsc;
                
        }
        pEsc++;
    }
}

void edt_PrintWithEscapes( CPrintState *ps, char *p, XP_Bool bBreakLines ){
    char *pBegin;
    PA_AmpEsc *pEsc;

    // break the lines after 70 characters if we can, but don't really do it 
    // in the formatted case.
    while( p && *p ){
        pBegin = p;
        while( *p && ps->m_iCharPos < 70 && ed_escapes[(unsigned char)*p] == 0){
            p++;
            ps->m_iCharPos++;
        }
        while( *p && *p != ' ' && ed_escapes[(unsigned char)*p] == 0){
            ps->m_iCharPos++;
            p++;
        }
        ps->m_pOut->Write( pBegin, p-pBegin);

        if( *p && *p == ' ' && ps->m_iCharPos >= 70 ){
            if( bBreakLines ){
                ps->m_pOut->Write( "\n", 1 );
            }
            else {
                ps->m_pOut->Write( " ", 1 );
            }
            ps->m_iCharPos = 0;
        }
        else if( *p && (pEsc = ed_escapes[(unsigned char)*p]) != 0 ){
            ps->m_pOut->Write( "&",1 );
            ps->m_pOut->Write( pEsc->str, pEsc->len );
            ps->m_pOut->Write( ";",1 );
            ps->m_iCharPos += pEsc->len+2;
        }

        // move past the space or special char
        if( *p ){
            *p++;
        }
    }
}

char *edt_LocalName( char *pURL ){
    char *pDest = FE_URLToLocalName( pURL );
#if 0
    if( pDest && FE_EditorPrefConvertFileCaseOnWrite( ) ){
        char *url_ptr = pDest;
        while ( *url_ptr != '\0' ){
            *url_ptr = XP_TO_LOWER(*url_ptr); 
            url_ptr++;
        }
    }
#endif
    return pDest;
}

PA_Block PA_strdup( char* s ){
    char *new_str;
    PA_Block new_buff;

    if( s == 0 ){
        return 0;
    }

    new_buff = (PA_Block)PA_ALLOC(XP_STRLEN(s) + 1);
    if (new_buff != NULL)
    {
        PA_LOCK(new_str, char *, new_buff);
        XP_STRCPY(new_str, s);
        PA_UNLOCK(new_buff);
    }
    return new_buff;
}

void edt_SetTagData( PA_Tag* pTag, char* pTagData){
    PA_Block buff;
    char *locked_buff;
    int iLen;

    iLen = XP_STRLEN(pTagData);
    buff = (PA_Block)PA_ALLOC((iLen+1) * sizeof(char));
    if (buff != NULL)
    {
        PA_LOCK(locked_buff, char *, buff);
        XP_BCOPY(pTagData, locked_buff, iLen);
        locked_buff[iLen] = '\0';
        PA_UNLOCK(buff);
    }
    else { 
        // LTNOTE: out of memory, should throw an exception
        return;
    }
    pTag->data = buff;
    pTag->data_len = iLen;
    pTag->next = NULL;
    return;
}

//
// Create a tag and add it to the list
//
void edt_AddTag( PA_Tag*& pStart, PA_Tag*& pEnd, TagType t, XP_Bool bIsEnd,
        char *pTagData  ){
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    pTag->type = t;
    pTag->is_end = bIsEnd;
    edt_SetTagData(pTag, pTagData ? pTagData : ">" );
    if( pStart == 0 ){
        pStart = pTag;
    }
    if( pEnd ){
        pEnd->next = pTag;
    }
    pEnd = pTag;
}



//-----------------------------------------------------------------------------
// CEditPosition
//-----------------------------------------------------------------------------

void CEditPositionComparable::CalcPosition( TXP_GrowableArray_int32* pA, 
            CEditPosition *pPos ){

    CEditElement *pElement, *pParent, *pSib;
    int i;

    pA->Empty();
    pA->Add( pPos->Offset() );
    pElement = pPos->Element();
    if( pElement ){
        while( (pParent = pElement->GetParent()) != NULL ){
            i = 0;
            pSib = pParent->GetChild();        
            while( pSib != pElement ){
                pSib = pSib->GetNextSibling();
                i++;
            }
            pA->Add(i);
            pElement = pParent;
        }
    }
}

int CEditPositionComparable::Compare( CEditPosition *pPos ){
    TXP_GrowableArray_int32 toArray;

    CalcPosition( &toArray, pPos );

    int32 i = m_Array.Size();
    int32 iTo = toArray.Size();
    int32 iDiff;

    while( i && iTo ){
        iDiff = toArray[--iTo] - m_Array[--i];
        if( iDiff != 0 ) {
            return (iDiff > 0 ? 1 : -1 );
        }
    }
    if( i == 0 && iTo == 0){
        return 0;
    }
    iDiff = iTo - i;
    return (iDiff > 0 ? 1 : -1 );
}


// CEditInsertPoint

CEditInsertPoint::CEditInsertPoint() {
    m_pElement = 0;
    m_iPos = 0;
    m_bStickyAfter = FALSE;
}

CEditInsertPoint::CEditInsertPoint(CEditLeafElement* pElement, ElementOffset iPos) {
    m_pElement = pElement;
    m_iPos = iPos;
    m_bStickyAfter = FALSE;
}

CEditInsertPoint::CEditInsertPoint(CEditElement* pElement, ElementOffset iPos){
    m_pElement = pElement->Leaf();
    m_iPos = iPos;
    m_bStickyAfter = FALSE;
}

CEditInsertPoint::CEditInsertPoint(CEditElement* pElement, ElementOffset iPos, XP_Bool bStickyAfter){
    m_pElement = pElement->Leaf();
    m_iPos = iPos;
    m_bStickyAfter = bStickyAfter;
}

XP_Bool CEditInsertPoint::operator==(const CEditInsertPoint& other ) {
    return m_pElement == other.m_pElement && m_iPos == other.m_iPos;
}

XP_Bool CEditInsertPoint::operator!=(const CEditInsertPoint& other ) {
    return ! (*this == other);
}

XP_Bool CEditInsertPoint::operator<(const CEditInsertPoint& other ) {
    return Compare(other) < 0;
}

XP_Bool CEditInsertPoint::operator<=(const CEditInsertPoint& other ) {
    return Compare(other) <= 0;
}

XP_Bool CEditInsertPoint::operator>=(const CEditInsertPoint& other ) {
    return Compare(other) >= 0;
}

XP_Bool CEditInsertPoint::operator>(const CEditInsertPoint& other ) {
    return Compare(other) > 0;
}

CEditElement* CEditInsertPoint::FindNonEmptyElement( CEditElement *pStartElement ){
    CEditElement* pOldElement = NULL;
    while ( pStartElement && pStartElement != pOldElement ) {
        pOldElement = pStartElement;
        if( !pStartElement->IsLeaf() ){
            pStartElement = pStartElement->PreviousLeaf();
        }
        else if( pStartElement->Leaf()->GetLayoutElement() == 0 ){
           pStartElement = pStartElement->PreviousLeaf();
        }
    }
    return pStartElement;
}

int CEditInsertPoint::Compare(const CEditInsertPoint& other ) {
    XP_Bool bAIsBreak = FALSE;
    LO_Element* pA = m_pElement->GetLayoutElement();
    if ( ! pA ) {
        bAIsBreak = TRUE;
        pA = FindNonEmptyElement(m_pElement)->Leaf()->GetLayoutElement();
    }
    XP_Bool bBIsBreak = FALSE;
    LO_Element* pB = other.m_pElement->GetLayoutElement();
    if ( ! pB ) {
        bBIsBreak = TRUE;
        pB = FindNonEmptyElement(other.m_pElement)->Leaf()->GetLayoutElement();
    }
    if ( !pA || !pB ) {
        XP_ASSERT(FALSE); // Phantom insert points.
        return 0;
    }
    int32 aIndex = pA->lo_any.ele_id;
    int32 bIndex = pB->lo_any.ele_id;
    if ( aIndex < bIndex ) {
        return -1;
    }
    else if ( aIndex == bIndex ) {
        // Same element. Compare positions.
        if ( m_iPos < other.m_iPos ) {
            return -1;
        }
        else if ( m_iPos == other.m_iPos ) {
            if ( bAIsBreak < bBIsBreak ) {
                return -1;
            }
            else if ( bAIsBreak == bBIsBreak ) {
                return 0;
            }
            else {
                return 1;
            }
        }
        else {
            return 1;
        }
    }
    else {
        return 1;
    }
}

XP_Bool CEditInsertPoint::IsDenormalizedVersionOf(const CEditInsertPoint& other){
    return other.m_iPos == 0 &&
        m_iPos == m_pElement->GetLen() &&
        m_pElement->NextLeaf() == other.m_pElement;
}

XP_Bool CEditInsertPoint::IsStartOfElement() {
    return m_iPos <= 0;
}

XP_Bool CEditInsertPoint::IsEndOfElement() {
    return m_iPos >= m_pElement->GetLen();
}

XP_Bool CEditInsertPoint::IsStartOfContainer() {
    return IsStartOfElement()
        && m_pElement->PreviousLeafInContainer() == NULL;
}

XP_Bool CEditInsertPoint::IsEndOfContainer() {
    return IsEndOfElement()
        && m_pElement->LeafInContainerAfter() == NULL;
}

XP_Bool CEditInsertPoint::IsStartOfDocument(){
    return IsStartOfElement() &&
        m_pElement->PreviousLeaf() == NULL;
}

XP_Bool CEditInsertPoint::IsEndOfDocument(){
    return m_pElement->IsEndOfDocument();
}

XP_Bool CEditInsertPoint::GapWithBothSidesAllowed(){
    XP_Bool bResult = FALSE;
    XP_Bool bAllowBothSidesOfGap = m_pElement->AllowBothSidesOfGap();
    if ( bAllowBothSidesOfGap
            && IsEndOfElement()
            && !IsEndOfContainer() ) {
        bResult = TRUE;
    }
    else if ( IsStartOfElement() && ! IsStartOfContainer() ) {
        if ( bAllowBothSidesOfGap ) {
            bResult = TRUE;
        }
        else {
            CEditLeafElement* pPrev = m_pElement->PreviousLeaf();
            if ( pPrev && pPrev->AllowBothSidesOfGap() ) {
                bResult = TRUE;
            }
        }
    }
    return bResult;
}

XP_Bool CEditInsertPoint::IsLineBreak(){
    return IsHardLineBreak() || IsSoftLineBreak();
}

XP_Bool CEditInsertPoint::IsSoftLineBreak(){
    XP_Bool result = FALSE;
    intn iOffset = 0;
    CEditTextElement* pText = CEditTextElement::Cast(m_pElement);
    if ( pText ){
        int iOffset;
        LO_Element* pLOElement;
        if ( pText->GetLOElementAndOffset(m_iPos, m_bStickyAfter,
                pLOElement, iOffset) ){
            if ( pLOElement->type == LO_LINEFEED ){
                result = TRUE;
            }
        }
    }
    return result;
}

XP_Bool CEditInsertPoint::IsHardLineBreak(){
    return IsStartOfElement() && m_pElement->CausesBreakBefore()
        || IsEndOfElement() && m_pElement->CausesBreakAfter();
}

XP_Bool CEditInsertPoint::IsSpace() {
    XP_Bool result = FALSE;
    if ( m_pElement->IsA(P_TEXT) ) {
        if ( m_iPos == m_pElement->GetLen() ){
            CEditLeafElement *pNext = m_pElement->TextInContainerAfter();
            if( pNext
                && pNext->IsA(P_TEXT)
                && pNext->Text()->GetLen() != 0
                && pNext->Text()->GetText()[0] == ' ') {
                result = TRUE;
            }
        }
        else if ( m_pElement->Text()->GetText()[m_iPos] == ' ' ) {
            result = TRUE;
        }
    }
    return result;
}

XP_Bool CEditInsertPoint::IsSpaceBeforeOrAfter() {
    XP_Bool result = IsSpace();
    if ( !result ) {
        CEditInsertPoint before = PreviousPosition();
        if ( before != *this ) {
            result = before.IsSpace();
        }
    }
    return result;
}

CEditInsertPoint CEditInsertPoint::NextPosition(){
    CEditInsertPoint result;
    m_pElement->NextPosition(m_iPos, result.m_pElement, result.m_iPos);
    return result;
}

CEditInsertPoint CEditInsertPoint::PreviousPosition(){
    CEditInsertPoint result;
    m_pElement->PrevPosition(m_iPos, result.m_pElement, result.m_iPos);
    // Work around what is probably a bug in PrevPosition.
    if ( m_iPos == 0 && result.m_iPos == result.m_pElement->GetLen() ) {
        result.m_pElement->PrevPosition(result.m_iPos, result.m_pElement, result.m_iPos);
    }
    return result;
}

#ifdef DEBUG
void CEditInsertPoint::Print(IStreamOut& stream) {
    stream.Printf("0x%08lx.%d%s", m_pElement, m_iPos, m_bStickyAfter ? "+" : "" );
}
#endif



// CEditSelection

CEditSelection::CEditSelection(){
    m_bFromStart = FALSE;
}

CEditSelection::CEditSelection(CEditElement* pStart, intn iStartPos,
    CEditElement* pEnd, intn iEndPos, XP_Bool fromStart)
    : m_start(pStart, iStartPos), m_end(pEnd, iEndPos), m_bFromStart(fromStart)
{
}

CEditSelection::CEditSelection(const CEditInsertPoint& start,
    const CEditInsertPoint& end, XP_Bool fromStart)
    : m_start(start), m_end(end), m_bFromStart(fromStart)
{
}

XP_Bool CEditSelection::operator==(const CEditSelection& other ){
    return m_start == other.m_start &&
    m_end == other.m_end && m_bFromStart == other.m_bFromStart;
}

XP_Bool CEditSelection::operator!=(const CEditSelection& other ){
    return ! (*this == other);
}

XP_Bool CEditSelection::EqualRange(CEditSelection& other){
    return m_start == other.m_start && m_end == other.m_end;
}

XP_Bool CEditSelection::IsInsertPoint() {
    /* It's an insert point if the two edges are equal, OR if the
     * start is at the end of an element, and the
     * end is at the start of an element, and
     * the two elements are next to each other,
     * and they're in the same container. (Whew!)
     */
    return m_start == m_end ||
        ( m_start.IsEndOfElement() &&
          m_end.IsStartOfElement() &&
          m_start.m_pElement->LeafInContainerAfter() == m_end.m_pElement
        );
}

CEditInsertPoint* CEditSelection::GetEdge(XP_Bool bEnd){
    if ( bEnd ) return &m_end;
    else return &m_start;
}

CEditInsertPoint* CEditSelection::GetActiveEdge(){
     return GetEdge(!m_bFromStart);
}

CEditInsertPoint* CEditSelection::GetAnchorEdge(){
    return GetEdge(m_bFromStart);
}

XP_Bool CEditSelection::Intersects(CEditSelection& other) {
    return m_start < other.m_end && other.m_start < m_end;
}

XP_Bool CEditSelection::Contains(CEditInsertPoint& point) {
    return m_start <= point && point < m_end;
}

XP_Bool CEditSelection::Contains(CEditSelection& other) {
    return ContainsStart(other) && other.m_end <= m_end;
}

XP_Bool CEditSelection::ContainsStart(CEditSelection& other) {
    return Contains(other.m_start);
}

XP_Bool CEditSelection::ContainsEnd(CEditSelection& other) {
    return Contains(other.m_end) || other.m_end == m_end;
}

XP_Bool CEditSelection::ContainsEdge(CEditSelection& other, XP_Bool bEnd){
    if ( bEnd )
        return ContainsEnd(other);
    else
        return ContainsStart(other);
}

XP_Bool CEditSelection::CrossesOver(CEditSelection& other) {
    XP_Bool bContainsStart = ContainsStart(other);
    XP_Bool bContainsEnd = ContainsEnd(other);
    return bContainsStart ^ bContainsEnd;
}

XP_Bool CEditSelection::ClipTo(CEditSelection& other) {
    // True if the result is defined. No change to this if it isn't
    XP_Bool intersects = Intersects(other);
    if ( intersects ) {
        if ( m_start < other.m_start ) {
            m_start = other.m_start;
        }
        if ( other.m_end < m_end ) {
            m_end = other.m_end;
        }
    }
    return intersects;
}

CEditElement* CEditSelection::GetCommonAncestor(){
    return m_start.m_pElement->GetCommonAncestor(m_end.m_pElement);
}

XP_Bool CEditSelection::CrossesSubDocBoundary(){
    // The intent of this method is "If this selection were cut, would the
    // document become malformed?"

    // A document could be malformed if the selection crosses the table
    // boundary, or if it would leave a table without it's protective
    // buffer of containers.

    XP_Bool result = FALSE;
    if ( ! IsInsertPoint() ){
        CEditElement* pStartCell = m_start.m_pElement->GetSubDocOrMultiColumnSkipRoot();
        CEditElement* pEndCell = m_end.m_pElement->GetSubDocOrMultiColumnSkipRoot();
        CEditElement* pClosedEndCell = GetClosedEndContainer()->GetSubDocOrMultiColumnSkipRoot();
        result = pStartCell != pEndCell || pStartCell != pClosedEndCell;
        if ( result ) {
            // This is only OK if the selection spans an entire table.
            Bool bStartBad = FALSE;
            Bool bEndBad = FALSE;
            if ( pStartCell ) {
                CEditSelection all;
                pStartCell->GetTableOrMultiColumnIgnoreSubdoc()->GetAll(all);
                bStartBad = ! Contains(all);
            }

            if ( pEndCell ) {
                CEditSelection all;
                pEndCell->GetTableOrMultiColumnIgnoreSubdoc()->GetAll(all);
                bEndBad = ! Contains(all);
            }
            result = bStartBad || bEndBad;
        }
        else if ( pStartCell && pEndCell != pClosedEndCell ) {
            // If the selection spans an entire cell, but not an entire table, that's bad
            CEditSelection all;
            pStartCell->GetTableOrMultiColumnIgnoreSubdoc()->GetAll(all);
            result = *this != all;
        }
        else {
            // One further check: If the selection starts at the start of
            // a paragraph, and the selection ends at the end of a paragraph
            // and the previous item is a table, and the
            // next item is not a container, then we can't cut. The reason is
            // that tables need to have containers after them.

            // Note that m_end is one more than is selected.
            if ( m_start.IsStartOfContainer() &&
                m_end.IsStartOfContainer() ) {
                // The selection spans whole containers.
                CEditContainerElement* pStartContainer = GetStartContainer();
                CEditElement* pPreviousSib = pStartContainer->GetPreviousSibling();
                
                if ( pPreviousSib && pPreviousSib->IsTable() ) {
                    CEditContainerElement* pEndContainer = GetClosedEndContainer();
                    CEditElement* pNextSib = pEndContainer->GetNextSibling();
                    if ( !pNextSib || pNextSib->IsEndContainer() || pNextSib->IsTable() ) {
                        result = TRUE;
                    }
                }
            }
        }
    }
    return result;
}

CEditSubDocElement* CEditSelection::GetEffectiveSubDoc(XP_Bool bEnd){
    CEditInsertPoint* pEdge = GetEdge(bEnd);
    CEditSubDocElement* pSubDoc = pEdge->m_pElement->GetSubDoc();
    while ( pSubDoc ) {
        // If the edge is actually the edge of a subdoc, skip out of the subdoc.
        CEditSelection wholeSubDoc;
        pSubDoc->GetAll(wholeSubDoc);
        // If this is a wholeSubDoc Selection, bump up
        if ( *this != wholeSubDoc ) {
            if ( *pEdge != *wholeSubDoc.GetEdge(bEnd) )
                break;
            // If the other end of the selection is is in this same subdoc, stop
            if ( wholeSubDoc.ContainsEdge(*this, !bEnd) )
                break;
        }
        CEditElement* pSubDocParent = pSubDoc->GetParent();
        if ( !pSubDocParent )
            break;
        CEditSubDocElement* pParentSubDoc = pSubDocParent->GetSubDoc();
        if ( ! pParentSubDoc )
            break;
        pSubDoc = pParentSubDoc;
    }
    return pSubDoc;
}

XP_Bool CEditSelection::ExpandToNotCrossStructures(){
    XP_Bool bChanged = FALSE;
    if ( ! IsInsertPoint() ){
        CEditElement* pGrandfatherStart = m_start.m_pElement->GetParent()->GetParent();
        CEditElement* pGrandfatherEnd;
        CEditElement* pEffectiveEndContianer;
        if ( m_end.IsEndOfDocument() || EndsAtStartOfContainer()) {
            pEffectiveEndContianer = m_end.m_pElement->PreviousLeaf()->GetParent();
        }
        else {
            pEffectiveEndContianer = m_end.m_pElement->GetParent();
        }
        pGrandfatherEnd = pEffectiveEndContianer->GetParent();
        if ( pGrandfatherStart != pGrandfatherEnd ){
            bChanged = TRUE;
            CEditElement* pAncestor = pGrandfatherStart->GetCommonAncestor(pGrandfatherEnd);
            if ( ! pAncestor ) {
                XP_ASSERT(FALSE);
                return FALSE;
            }
            // If we're in a table, we need to return the whole table.
            if ( pAncestor->IsTableRow() ) {
                pAncestor = pAncestor->GetTable();
                if ( ! pAncestor ) {
                    XP_ASSERT(FALSE);
                    return FALSE;
                }
            }
            if ( pAncestor->IsTable() ) {
                pAncestor->GetAll(*this);
                return TRUE;
            }
            CEditLeafElement* pCleanStart = pAncestor->GetChildContaining(m_start.m_pElement)->GetFirstMostChild()->Leaf();
            CEditLeafElement* pCleanEnd = pAncestor->GetChildContaining(pEffectiveEndContianer)->GetLastMostChild()->NextLeaf();
            m_start.m_pElement = pCleanStart;
            m_start.m_iPos = 0;
            if ( pCleanEnd ) {
                m_end.m_pElement = pCleanEnd;
                m_end.m_iPos = 0;
            }
            else {
                XP_ASSERT(FALSE);   // Somehow we've lost the end of the document element.
                pCleanEnd = m_end.m_pElement->GetParent()->GetLastMostChild()->Leaf();
                m_end.m_pElement = pCleanEnd;
                m_end.m_iPos = pCleanEnd->Leaf()->GetLen();
            }
        }
    }
    return bChanged;
}

void CEditSelection::ExpandToBeCutable(){
    // Very similar to ExpandToNotCrossDocumentStructures, except that
    // the result is always Cutable.
    if ( ! IsInsertPoint() ){
        CEditElement* pGrandfatherStart = m_start.m_pElement->GetParent()->GetParent();
        CEditElement* pGrandfatherEnd;
        CEditElement* pEffectiveEndContianer;
        if ( m_end.IsEndOfDocument()) {
            pEffectiveEndContianer = m_end.m_pElement->PreviousLeaf()->GetParent();
        }
        else {
            pEffectiveEndContianer = m_end.m_pElement->GetParent();
        }
        pGrandfatherEnd = pEffectiveEndContianer->GetParent();
        if ( pGrandfatherStart != pGrandfatherEnd ){
            CEditElement* pAncestor = pGrandfatherStart->GetCommonAncestor(pGrandfatherEnd);
            if ( ! pAncestor ) {
                XP_ASSERT(FALSE);
                return;
            }
            // If we're in a table, we need to return the whole table.
            if ( pAncestor->IsTableRow() ) {
                pAncestor = pAncestor->GetTable();
                if ( ! pAncestor ) {
                    XP_ASSERT(FALSE);
                    return;
                }
            }
            if ( pAncestor->IsTable() ) {
                pAncestor->GetAll(*this);
                return;
            }
            CEditLeafElement* pCleanStart = pAncestor->GetChildContaining(m_start.m_pElement)->GetFirstMostChild()->Leaf();
            CEditLeafElement* pCleanEnd = pAncestor->GetChildContaining(pEffectiveEndContianer)->GetLastMostChild()->NextLeaf();
            m_start.m_pElement = pCleanStart;
            m_start.m_iPos = 0;
            if ( pCleanEnd ) {
                m_end.m_pElement = pCleanEnd;
                m_end.m_iPos = 0;
            }
            else {
                XP_ASSERT(FALSE);   // Somehow we've lost the end of the document element.
                pCleanEnd = m_end.m_pElement->GetParent()->GetLastMostChild()->Leaf();
                m_end.m_pElement = pCleanEnd;
                m_end.m_iPos = pCleanEnd->Leaf()->GetLen();
            }
        }
    }
}

void CEditSelection::ExpandToIncludeFragileSpaces() {
    // Expand the selection to include spaces that will be
    // trimmed if we did a cut of the original selection.
    if ( ! IsInsertPoint() ){
        CEditInsertPoint before = m_start.PreviousPosition();
        // Either we backed up one position, or we hit the beginning
        // of the document.
        if ( before == m_start || before.IsSpace() ) {
            if ( m_end.IsSpace() ) {
                if ( ! m_end.m_pElement->InFormattedText() ) {
                    m_end = m_end.NextPosition();
                }
            }
        }
    }
}

void CEditSelection::ExpandToEncloseWholeContainers(){
    XP_Bool bWasInsertPoint = IsInsertPoint();
    CEditLeafElement* pCleanStart = m_start.m_pElement->GetParent()->GetFirstMostChild()->Leaf();
    m_start.m_pElement = pCleanStart;
    m_start.m_iPos = 0;
    if ( !m_end.IsStartOfContainer() || bWasInsertPoint ) {
        CEditLeafElement* pCleanEnd = m_end.m_pElement->GetParent()->GetLastMostChild()->NextLeaf();
        if ( pCleanEnd ) {
            m_end.m_pElement = pCleanEnd;
            m_end.m_iPos = 0;
        }
        else {
            XP_ASSERT(FALSE);   // Somehow we've lost the end of the document element.
            pCleanEnd = m_end.m_pElement->GetParent()->GetLastMostChild()->Leaf();
            m_end.m_pElement = pCleanEnd;
            m_end.m_iPos = pCleanEnd->Leaf()->GetLen();
        }
    }
}


XP_Bool CEditSelection::EndsAtStartOfContainer() {
    return m_end.IsStartOfContainer();
}

XP_Bool CEditSelection::StartsAtEndOfContainer() {
    return m_start.IsEndOfContainer();
}

XP_Bool CEditSelection::AnyLeavesSelected(){
    // FALSE if insert point or container end.
    return ! (IsInsertPoint() || IsContainerEnd());
}

XP_Bool CEditSelection::IsContainerEnd(){
    XP_Bool result = FALSE;
    if ( ! IsInsertPoint() && EndsAtStartOfContainer()
        && StartsAtEndOfContainer() ) {
        CEditContainerElement* pStartContainer = m_start.m_pElement->FindContainer();
        CEditContainerElement* pEndContainer = m_end.m_pElement->FindContainer();
        if ( pStartContainer && pEndContainer ) {
            CEditContainerElement* pNextContainer =
                CEditContainerElement::Cast(pStartContainer->FindNextElement(&CEditElement::FindContainer, 0));
            if ( pNextContainer == pEndContainer) {
                result = TRUE;
            }
        }
    }
    return result;
}

void CEditSelection::ExcludeLastDocumentContainerEnd() {
    // Useful for cut & delete, where you don't replace the final container mark
    if ( ContainsLastDocumentContainerEnd() ){
        CEditLeafElement* pEnd = m_end.m_pElement;
        pEnd = pEnd->PreviousLeaf();
        if ( pEnd ) {
            m_end.m_pElement = pEnd;
            m_end.m_iPos = pEnd->Leaf()->GetLen();
            if ( m_start > m_end ) {
                m_start = m_end;
            }
        }
    }
}

XP_Bool CEditSelection::ContainsLastDocumentContainerEnd() {
    // Useful for cut & delete, where you don't replace the final container mark
    XP_Bool bResult = FALSE;
    CEditLeafElement* pEnd = m_end.m_pElement;
    if ( pEnd && pEnd->GetElementType() == eEndElement ){
        bResult = TRUE;
    }
    return bResult;
}

CEditContainerElement* CEditSelection::GetStartContainer() {
    CEditElement* pParent = m_start.m_pElement->GetParent();
    if ( pParent && pParent->IsContainer() ) return pParent->Container();
    else return NULL;
}

CEditContainerElement* CEditSelection::GetClosedEndContainer() {
    CEditElement* pEnd = m_end.m_pElement;
    if ( m_end.IsStartOfContainer() ) {
        // Back up one
        CEditElement* pPrev = pEnd->PreviousLeaf();
        if ( pPrev ) {
            pEnd = pPrev;
        }
        else pEnd = m_start.m_pElement;
    }
    CEditElement* pParent = pEnd->GetParent();
    if ( pParent && pParent->IsContainer() ) return pParent->Container();
    else return NULL;
}

#ifdef DEBUG
void CEditSelection::Print(IStreamOut& stream){
    stream.Printf("start ");
    m_start.Print(stream);
    stream.Printf(" end ");
    m_end.Print(stream);
    stream.Printf(" FromStart %ld", (long)m_bFromStart);
}
#endif

// Persistent selections
CPersistentEditInsertPoint::CPersistentEditInsertPoint()
{
    m_index = -1;
    m_bStickyAfter = TRUE;
}

CPersistentEditInsertPoint::CPersistentEditInsertPoint(ElementIndex index)
    : m_index(index), m_bStickyAfter(TRUE)
{}

CPersistentEditInsertPoint::CPersistentEditInsertPoint(ElementIndex index, XP_Bool bStickyAfter)
    : m_index(index), m_bStickyAfter(bStickyAfter)
{
}

XP_Bool CPersistentEditInsertPoint::operator==(const CPersistentEditInsertPoint& other ) {
    return m_index == other.m_index;
}

XP_Bool CPersistentEditInsertPoint::operator!=(const CPersistentEditInsertPoint& other ) {
    return ! (*this == other);
}

XP_Bool CPersistentEditInsertPoint::operator<(const CPersistentEditInsertPoint& other ) {
    return m_index < other.m_index;
}

XP_Bool CPersistentEditInsertPoint::operator<=(const CPersistentEditInsertPoint& other ) {
    return *this < other || *this == other;
}

XP_Bool CPersistentEditInsertPoint::operator>=(const CPersistentEditInsertPoint& other ) {
    return ! (*this < other);
}

XP_Bool CPersistentEditInsertPoint::operator>(const CPersistentEditInsertPoint& other ) {
    return ! (*this <= other);
}

#ifdef DEBUG
void CPersistentEditInsertPoint::Print(IStreamOut& stream) {
    stream.Printf("%ld%s", (long)m_index, m_bStickyAfter ? "+" : "" );
}
#endif

void CPersistentEditInsertPoint::ComputeDifference(
    CPersistentEditInsertPoint& other, CPersistentEditInsertPoint& delta){
    // delta = this - other;
    delta.m_index = m_index - other.m_index;
}

void CPersistentEditInsertPoint::AddRelative(
    CPersistentEditInsertPoint& delta, CPersistentEditInsertPoint& result){
    // result = this + delta;
    result.m_index = m_index + delta.m_index;
}

XP_Bool CPersistentEditInsertPoint::IsEqualUI(
    const CPersistentEditInsertPoint& other ) {
    return m_index == other.m_index && m_bStickyAfter == other.m_bStickyAfter;
}

// class CPersistentEditSelection

CPersistentEditSelection::CPersistentEditSelection()
{
    m_bFromStart = FALSE;
}

CPersistentEditSelection::CPersistentEditSelection(const CPersistentEditInsertPoint& start, const CPersistentEditInsertPoint& end)
    : m_start(start), m_end(end)
{
    m_bFromStart = FALSE;
}

ElementIndex CPersistentEditSelection::GetCount() {
    return m_end.m_index - m_start.m_index;
}

XP_Bool CPersistentEditSelection::IsInsertPoint(){
    return m_start == m_end;
}

XP_Bool CPersistentEditSelection::operator==(const CPersistentEditSelection& other ) {
    return SelectedRangeEqual(other) && m_bFromStart == other.m_bFromStart;
}

XP_Bool CPersistentEditSelection::operator!=(const CPersistentEditSelection& other ) {
    return ! ( *this == other );
}

XP_Bool CPersistentEditSelection::SelectedRangeEqual(const CPersistentEditSelection& other ) {
    return m_start == other.m_start && m_end == other.m_end;
}

CPersistentEditInsertPoint* CPersistentEditSelection::GetEdge(XP_Bool bEnd){
    return bEnd ? &m_end : &m_start;
} 

#ifdef DEBUG
void CPersistentEditSelection::Print(IStreamOut& stream) {
    stream.Printf("start ");
    m_start.Print(stream);
    stream.Printf(" end ");
    m_end.Print(stream);
    stream.Printf(" FromStart %ld", (long)m_bFromStart);
}
#endif

// Used for undo
// change this by the same way that original was changed into modified.
void CPersistentEditSelection::Map(CPersistentEditSelection& original,
    CPersistentEditSelection& modified){
    CPersistentEditSelection delta;
    CPersistentEditSelection result;
    modified.m_start.ComputeDifference(original.m_start, delta.m_start);
    modified.m_end.ComputeDifference(original.m_end, delta.m_end);
    m_start.AddRelative(delta.m_start, result.m_start);
    m_end.AddRelative(delta.m_end, result.m_end);
    *this = result;
}

//
//  Call this constructor with a maximum number, a series of bits and 
//  BIT_ARRAY_END for example: CBitArray a(100, 1 ,4 9 ,7, BIT_ARRAY_END);
// 
CBitArray::CBitArray(long n, int iFirst, ...) {

	m_Bits = 0;
	size = 0;

	int i;

	if( n ) {
	  SetSize(n);
	}
	va_list stack;
#ifdef OSF1
	va_start( stack, n );
#else
	va_start( stack, iFirst );

	(*this)[iFirst] = 1;
#endif

	while( (i = va_arg(stack,int)) != BIT_ARRAY_END ){
	   (*this)[i] = 1;
	}
}

/* CLM: Helper to gray UI items not allowed when inside Java Script
 * Note that the return value is FALSE if partial selection, 
 *   allowing the non-script text to be changed
 * Current Font Size, Color, and Character attributes will suppress
 *   setting other attributes, so it is OK to call these when mixed
*/
XP_Bool EDT_IsJavaScript(MWContext * pMWContext)
{
    if(!pMWContext) return FALSE;
    XP_Bool bRetVal = FALSE;
    EDT_CharacterData * pData = EDT_GetCharacterData(pMWContext);
    if( pData) {
        bRetVal = ( (0 != (pData->mask & TF_SERVER)) &&
                    (0 != (pData->values & TF_SERVER)) ) ||
                  ( (0 != (pData->mask & TF_SCRIPT )) &&
                    (0 != (pData->values & TF_SCRIPT)) );
        EDT_FreeCharacterData(pData);
    }
    return bRetVal;
}

/* Helper to use for enabling Character properties 
 * (Bold, Italic, etc., but DONT use for clearing (TF_NONE)
 *  or setting Java Script (Server or Client)
 * Tests for:
 *   1. Good edit buffer and not blocked because of some action,
 *   2. Caret or selection is NOT entirely within Java Script, 
 *   3. Caret or selection has some text or is mixed selection
 *      (thus FALSE if single non-text object is selected)
*/
XP_Bool EDT_CanSetCharacterAttribute(MWContext * pMWContext)
{
    if( !pMWContext || pMWContext->waitingMode || EDT_IsBlocked(pMWContext) ){
        return FALSE;
    }
    ED_ElementType type = EDT_GetCurrentElementType(pMWContext);
    return ( (type == ED_ELEMENT_TEXT || type == ED_ELEMENT_SELECTION) &&
             !EDT_IsJavaScript(pMWContext) );
}
#endif
