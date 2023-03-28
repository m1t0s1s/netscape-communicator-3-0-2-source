/* -*- Mode: C; tab-width: 4 -*-
 */

#include "mkutils.h"
#include "mkstream.h"
#include "mkgeturl.h"
#include "xp.h"
#include "mkformat.h"


PUBLIC NET_StreamClass * 
NET_PlainTextConverter   (FO_Present_Types format_out,
                          void       *data_obj,
                          URL_Struct *URL_s,
                          MWContext  *window_id)
{
	NET_StreamClass * stream;
	char plaintext [] = "<plaintext>"; /* avoid writable string lossage... */

	StrAllocCopy(URL_s->content_type, TEXT_HTML);

	/* stuff for text fe */
	if(format_out == FO_VIEW_SOURCE)
	  {
		format_out = FO_PRESENT;
	  }

	stream = NET_StreamBuilder(format_out, URL_s, window_id);

	if(stream)
		(*stream->put_block)(stream->data_object, plaintext, 11);

    return stream;
}
