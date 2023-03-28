#include "types.r"
#include "systypes.r"
#include "csid.h"

/* STR# 5028 and STR# 5029 should have the same sequence */
/* We need to localize 5028, but not 5029 */
resource 'STR#' (5028, "Language/Region in Human Readable Form", purgeable) {{
	"English",
	"English/United States",
	"English/United Kingdom",
	
	"French",
	"French/France",
	"French/Canada",
	
	"German",
	
	"Japanese",
	
	"Chinese",
	"Chinese/China",
	"Chinese/Taiwan",
	
	"Korean",
	
	"Italian",
	
	"Spanish",
	"Spanish/Spain",

	"Portuguese",
	"Portuguese/Brazil"
	
}};
resource 'STR#' (5029, "Language/Region in ISO639-ISO3166 code", purgeable) {{
	"en",
	"en-US",
	"en-GB",
	
	"fr",
	"fr-FR",
	"fr-CA",
	
	"de",
	
	"ja",
	
	"zh",
	"zh-CN",
	"zh-TW",
	
	"ko",
	
	"it",
	
	"es",
	"es-ES",

	"pt",
	"pt-BR"
}};

/*	CSID List for Unicode Font */
type 'UCSL' {
	integer = $$CountOf(table);
	wide array table	{
		unsigned integer;	/* Just an int16 */
	};
};

resource 'UCSL' (0, "CSID List for Unicode Font", purgeable) {{
	CS_MAC_ROMAN,
	CS_SJIS,
	CS_BIG5,
	CS_GB_8BIT,
	CS_CNS_8BIT,
	CS_KSC_8BIT,
	CS_MAC_CE,
	CS_SYMBOL,
	CS_DINGBATS
}};
