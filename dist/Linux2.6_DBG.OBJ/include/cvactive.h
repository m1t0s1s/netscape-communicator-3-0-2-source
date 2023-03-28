
#ifndef CV_ACTIVE
#define CV_ACTIVE

#include "net.h"

/* define a constant to be passed to CV_MakeMultipleDocumentStream
 * as the data_object to signify that it should return
 * MK_END_OF_MULTIPART_MESSAGE when it gets to the end
 * of the multipart instead of waiting for the complete
 * function to be called
 */
#define CVACTIVE_SIGNAL_AT_END_OF_MULTIPART 999

XP_BEGIN_PROTOS

extern NET_StreamClass * 
CV_MakeMultipleDocumentStream (int         format_out,
                               void       *data_object,
                               URL_Struct *URL_s,
                               MWContext  *window_id);
XP_END_PROTOS

#endif /* CV_ACTIVE */
