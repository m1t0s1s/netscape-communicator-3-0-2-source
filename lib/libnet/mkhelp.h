
#ifndef MKHELP_H
#define MKHELP_H

/* stream converter to parse an HTML help mapping file
 * and load the url associated with the id
 */
extern NET_StreamClass *
NET_HTMLHelpMapToURL(int         format_out,
                     void       *data_object,
                     URL_Struct *URL_s,
                     MWContext  *window_id);

#endif /* MKHELP_H */

