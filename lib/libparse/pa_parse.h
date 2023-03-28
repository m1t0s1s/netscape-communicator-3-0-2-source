#include "xp.h"
#include "pa_tags.h"
#include "edttypes.h"

#define PA_ABORT    -1
#define PA_PARSED   0
#define PA_COMPLETE 1

#define COMMENT_NO  0
#define COMMENT_YES 1
#define COMMENT_MAYBE   2


/*************************
 * The following is to speed up case conversion
 * to allow faster checking of caseless equal among strings.
 * Used in pa_TagEqual().
 *************************/
#ifdef NON_ASCII_STRINGS
# define TOLOWER(x) (tolower((unsigned int)(x)))
#else /* ASCII TABLE LOOKUP */
  extern unsigned char lower_lookup[256];
# define TOLOWER(x)      (lower_lookup[(unsigned int)(x)])
#endif /* NON_ASCII_STRINGS */


/*******************************
 * PRIVATE STRUCTURES
 *******************************/
typedef struct pa_DocData_struct {
    int32 doc_id;
    MWContext *window_id;
    URL_Struct *url_struct;
    PA_OutputFunction *output_tag;
    intn hold;
    XP_Block hold_buf;
    int32 hold_size;
    int32 hold_len;
    int32 brute_tag;
    int32 comment_bytes;
    Bool lose_newline;
    void *layout_state;
    char *url;
    Bool from_net;
    FO_Present_Types format_out;
#ifdef EDITOR
    ED_Buffer *edit_buffer;
#endif
    NET_StreamClass *parser_stream;
    uint newline_count;
} pa_DocData;

struct pa_TagTable { char *name; int id; };
 

/*******************************
 * PUBLIC FUNCTIONS
 *******************************/
extern void PA_FreeTag(PA_Tag *);
extern PA_Block PA_FetchParamValue(PA_Tag *, char *);
extern int32 PA_FetchAllNameValues(PA_Tag *, char ***, char ***);
extern Bool PA_TagHasParams(PA_Tag *);

/*******************************
 * PRIVATE FUNCTIONS
 *******************************/
extern intn pa_TagEqual(char *, char *);
extern char *pa_FindMDLTag(pa_DocData *, char *, int32, intn *);
extern char *pa_FindMDLEndTag(pa_DocData *, char *, int32);
extern char *pa_FindMDLEndComment(pa_DocData *, char *, int32);
extern PA_Tag *pa_CreateTextTag(pa_DocData *, char *, int32);
extern PA_Tag *pa_CreateMDLTag(pa_DocData *, char *, int32);
extern char *pa_ExpandEscapes(char *, int32, int32 *, Bool);
extern struct pa_TagTable * pa_LookupTag (char* str, unsigned int len);

extern const char *pa_PrintTagToken(int32);    /* for debugging only */
extern intn pa_tokenize_tag(char *str);

/*
extern intn LO_Format(int32, PA_Tag *, intn);
*/

