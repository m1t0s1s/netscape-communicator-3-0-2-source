#include "csid.h"
#include "resgui.h"

/*	Conversion Table That Convert From Unicode to Script Text */
type 'UFRM' {
		array uint {
		unsigned integer;	/* Just an uint16 */
	};
};
/*	Conversion Table That Convert From  Script Text to Unicode */
type 'UTO ' {
		array uint {
		unsigned integer;	/* Just an uint16 */
	};
};


/* a Priorized List of csid that will be used to render unicode */
type CSIDLIST_RESTYPE {
	integer = $$CountOf(table);
	wide array table	{
		unsigned integer;	/* csid  */
	};
};


resource 'UFRM' ( CS_MAC_ROMAN, "macroman.uf- CS_MAC_ROMAN", purgeable) {{ 
#include "macroman.uf"
}};
resource 'UFRM' ( CS_SJIS, "sjis.uf- CS_LATIN1", purgeable) {{ 
#include "sjis.uf"
}};
resource 'UFRM' ( CS_MAC_CE, "macce.uf- CS_MAC_CE", purgeable) {{ 
#include "macce.uf"
}};
resource 'UFRM' ( CS_BIG5, "big5.uf- CS_BIG5", purgeable) {{ 
#include "big5.uf"
}};
resource 'UFRM' ( CS_GB_8BIT, "gb2312.uf- CS_GB_8BIT", purgeable) {{ 
#include "gb2312.uf"
}};
resource 'UFRM' ( CS_KSC_8BIT, "ksc5601.uf- CS_KSC_8BIT", purgeable) {{ 
#include "ksc5601.uf"
}};
resource 'UFRM' ( CS_DINGBATS, "macdingb.uf- CS_DINGBATS", purgeable) {{ 
#include "macdingb.uf"
}};
resource 'UFRM' ( CS_SYMBOL, "macsymbo.uf- CS_SYMBOL", purgeable) {{ 
#include "macsymbo.uf"
}};
resource 'UFRM' ( CS_MAC_CYRILLIC, "maccyril.uf- CS_MAC_CYRILLIC", purgeable) {{ 
#include "maccyril.uf"
}};
resource 'UFRM' ( CS_MAC_GREEK, "macgreek.uf- CS_MAC_GREEK", purgeable) {{ 
#include "macgreek.uf"
}};
resource 'UFRM' ( CS_MAC_TURKISH, "macturki.uf- CS_MAC_TURKISH", purgeable) {{ 
#include "macturki.uf"
}};


resource 'UTO '  ( CS_MAC_ROMAN, "macroman.ut- CS_MAC_ROMAN", purgeable) {{ 
#include "macroman.ut"
}};
resource 'UTO '  ( CS_SJIS, "sjis.ut- CS_LATIN1", purgeable) {{ 
#include "sjis.ut"
}};
resource 'UTO '  ( CS_MAC_CE, "macce.ut- CS_MAC_CE", purgeable) {{ 
#include "macce.ut"
}};
resource 'UTO '  ( CS_BIG5, "big5.ut- CS_BIG5", purgeable) {{ 
#include "big5.ut"
}};
resource 'UTO '  ( CS_GB_8BIT, "gb2312.ut- CS_GB_8BIT", purgeable) {{ 
#include "gb2312.ut"
}};
resource 'UTO '  ( CS_KSC_8BIT, "ksc5601.ut- CS_KSC_8BIT", purgeable) {{ 
#include "ksc5601.ut"
}};
resource 'UTO ' ( CS_DINGBATS, "macdingb.ut- CS_DINGBATS", purgeable) {{ 
#include "macdingb.ut"
}};
resource 'UTO ' ( CS_SYMBOL, "macsymbo.ut- CS_SYMBOL", purgeable) {{ 
#include "macsymbo.ut"
}};
resource 'UTO ' ( CS_MAC_CYRILLIC, "maccyril.ut- CS_MAC_CYRILLIC", purgeable) {{ 
#include "macdingb.ut"
}};
resource 'UTO ' ( CS_MAC_GREEK, "macgreek.ut- CS_MAC_GREEK", purgeable) {{ 
#include "macgreek.ut"
}};
resource 'UTO ' ( CS_MAC_TURKISH, "macturki.ut- CS_MAC_TURKISH", purgeable) {{ 
#include "macturki.ut"
}};

resource CSIDLIST_RESTYPE (CSIDLIST_RESID, "Roman/CE/Cy/Gr/J/TC/SC/K/Symbol/Dingbats/Tr", purgeable) {{
    CS_MAC_ROMAN,
    CS_MAC_CE,
    CS_MAC_CYRILLIC,
    CS_MAC_GREEK,
    CS_SJIS,
    CS_BIG5,
    CS_GB_8BIT,
    CS_KSC_8BIT,
    CS_SYMBOL,
    CS_DINGBATS,
    CS_MAC_TURKISH
}};
resource CSIDLIST_RESTYPE (CSIDLIST_RESID+1, "Roman/CE/Cy/Gr/TC/SC/K/J/Symbol/Dingbats/Tr", purgeable) {{
    CS_MAC_ROMAN,
    CS_MAC_CE,
    CS_MAC_CYRILLIC,
    CS_MAC_GREEK,
    CS_BIG5,
    CS_GB_8BIT,
    CS_KSC_8BIT,
    CS_SJIS,
    CS_SYMBOL,
    CS_DINGBATS,
    CS_MAC_TURKISH
}};
resource CSIDLIST_RESTYPE (CSIDLIST_RESID+2, "Roman/CE/Cy/Gr/SC/K/J/TC/Symbol/Dingbats/Tr", purgeable) {{
    CS_MAC_ROMAN,
    CS_MAC_CE,
    CS_MAC_CYRILLIC,
    CS_MAC_GREEK,
    CS_GB_8BIT,
    CS_KSC_8BIT,
    CS_SJIS,
    CS_BIG5,
    CS_SYMBOL,
    CS_DINGBATS,
    CS_MAC_TURKISH
}};
resource CSIDLIST_RESTYPE (CSIDLIST_RESID+3, "Roman/CE/Cy/Gr/K/J/TC/SC/Symbol/Dingbats/Tr", purgeable) {{
    CS_MAC_ROMAN,
    CS_MAC_CE,
    CS_MAC_CYRILLIC,
    CS_MAC_GREEK,
    CS_KSC_8BIT,
    CS_SJIS,
    CS_BIG5,
    CS_GB_8BIT,
    CS_SYMBOL,
    CS_DINGBATS,
    CS_MAC_TURKISH
}};
