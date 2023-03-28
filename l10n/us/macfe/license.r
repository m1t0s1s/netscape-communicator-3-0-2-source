#include "Browser_Defines.h"

#include "resgui.h"					// main window constants

#ifdef ALPHA
	read 'TEXT' (ABOUT_LICENSE, "Export License", purgeable, protected ) "::xp:LICENSE-alpha";

#elif defined (BETA)

	#ifdef EXPORT_VERSION
	read 'TEXT' (ABOUT_LICENSE, "Export License", purgeable, protected ) "::xp:LICENSE-export-beta";
	#else
	read 'TEXT' (ABOUT_LICENSE, "U.S. License", purgeable, protected ) "::xp:LICENSE-us-beta";
	#endif

#else	/* Final release */

	#ifdef NET
		#ifdef EXPORT_VERSION
		read 'TEXT' (ABOUT_LICENSE, "Export License", purgeable, protected ) "::xp:LICENSE-export-net";
		#else
		read 'TEXT' (ABOUT_LICENSE, "U.S. License", purgeable, protected ) "::xp:LICENSE-us-net";
		#endif
	#else /* RETAIL */
		#ifdef EXPORT_VERSION
		read 'TEXT' (ABOUT_LICENSE, "Export License", purgeable, protected ) "::xp:LICENSE-export";
		#else
		read 'TEXT' (ABOUT_LICENSE, "U.S. License", purgeable, protected ) "::xp:LICENSE-us";
		#endif
	#endif
#endif