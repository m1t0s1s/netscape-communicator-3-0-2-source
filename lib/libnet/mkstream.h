
#ifndef MKSTREAM_H
#define MKSTREAM_H

#include "mkgeturl.h"
#include "mkutils.h"

/* prints out all converter mime types during successive calls
 * call with first equal true to reset to the beginning
 * and with first equal false to print out the next
 * converter in the list.  Returns zero when all converters
 * are printed out.
 */
extern char *
XP_ListNextPresentationType(Bool first);

/* prints out all encoding mime types during successive calls
 * call with first equal true to reset to the beginning
 * and with first equal false to print out the next
 * converter in the list.  Returns zero when all converters
 * are printed out.
 */
extern char *
XP_ListNextEncodingType(Bool first);

/* register a mime type and a command to be executed
 */
extern void
NET_RegisterExternalViewerCommand(char * format_in, 
								  char * system_command, 
								  unsigned int stream_block_size);

/* removes all external viewer commands
 */
extern void NET_ClearExternalViewerConverters(void);

MODULE_PRIVATE void
NET_RegisterExternalConverterCommand(char * format_in,
                                     FO_Present_Types format_out,
                                     char * system_command,
                                     char * new_format);

#endif  /* MKSTREAM.h */
