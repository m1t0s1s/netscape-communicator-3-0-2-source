/* felocale.h - header file for things exported by locale.c */


#include <libi18n.h>

#include <Xm/Xm.h>


extern int16 fe_LocaleCharSetID;

extern char fe_LocaleCharSetName[];


unsigned char *
fe_ConvertFromLocaleEncoding(int16, unsigned char *);

unsigned char *
fe_ConvertToLocaleEncoding(int16, unsigned char *);

XmString
fe_ConvertToXmString(int16, unsigned char *);

char *
fe_GetNormalizedLocaleName(void);

char *
fe_GetTextField(Widget);

void
fe_InitCollation(void);

void
fe_SetTextField(Widget, const char *);

void
fe_SetTextFieldAndCallBack(Widget, const char *);
