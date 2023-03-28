#ifndef __xfe_pref_helpers_h
#define __xfe_pref_helpers_h

#include <X11/Intrinsic.h>
#include "net.h"

struct fe_prefs_helpers_data
{
  MWContext *context;
 
  /* Helpers page */
  Widget helpers_selector, helpers_page;
  Widget helpers_list;
 
  /* New Helper and Plugin stuff */
  Widget mime_types_desc_text;
  Widget mime_types_text;
  Widget mime_types_suffix_text;
 
  /* Editor */
  Widget navigator_b;
  Widget plugin_b;
  Widget plugin_option;
  Widget plugin_pulldown;
  Widget app_b;
  Widget app_text;
  Widget app_browse;
  Widget save_b;
  Widget unknown_b;

  Widget editor;
  Widget edit_b;
  Widget new_b;
  Widget delete_b;
 
  /* Data Stuff */
  int pos;
  NET_cdataStruct *cd;
  char **plugins;
};


extern void
fe_helpers_build_mime_list(struct fe_prefs_helpers_data *fep);

extern struct fe_prefs_helpers_data *
fe_helpers_make_helpers_page (MWContext *context, Widget parent);

void
fe_helpers_prepareForDestroy(struct fe_prefs_helpers_data *fep);

/* Given a mimetype string, will return the mailcap entry (md_list)
 * associated with this mimetype. If the mimetype has no mailcap entry
 * associated with this, returns NULL.
 */
extern NET_mdataStruct *
fe_helpers_get_mailcap_from_type(char *type);

/* Adds a new entry into the mailcap list (md_list) */
extern void
fe_helpers_add_new_mailcap_data(char *contenttype, char* src_str,
				char *command, char *xmode, Boolean isLocal);

/* Update the xmode of a mailcap entry. Also the src_string is updated.
 * If the mailcap entry is NULL, this will create a new one.
 */
extern void
fe_helpers_update_mailcap_entry(char *contenttype, NET_mdataStruct *md,
				char *xmode);
#endif
