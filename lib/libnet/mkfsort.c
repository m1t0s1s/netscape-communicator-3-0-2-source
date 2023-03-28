
#include "mkutils.h"
#include "mkfsort.h"
#include "mkgeturl.h"

#ifdef PROFILE
#pragma profile on
#endif

PRIVATE int net_file_sort_method = SORT_BY_NAME;

/* print a text string in place of the NET_FileEntryInfo special_type int */
PUBLIC char *
NET_PrintFileType(int special_type)
{
	switch(special_type)
	  {
		case NET_FILE_TYPE:
			return("FILE");
		case NET_DIRECTORY:
			return("DIRECTORY");
		case NET_SYM_LINK:
			return("SYMBOLIC-LINK");
		case NET_SYM_LINK_TO_DIR:
			return("SYM-DIRECTORY");
		case NET_SYM_LINK_TO_FILE:
			return("SYM-FILE");
		default:
			XP_ASSERT(0);
			return("FILE");
	  }
}

PUBLIC void 
NET_SetFileSortMethod(int method)
{
	net_file_sort_method = method;
}

MODULE_PRIVATE void NET_FreeEntryInfoStruct(NET_FileEntryInfo *entry_info)
{
    if(entry_info) 
      {
        FREEIF(entry_info->filename);
        /* free the struct */
        XP_FREE(entry_info);
      }
}

MODULE_PRIVATE NET_FileEntryInfo * NET_CreateFileEntryInfoStruct (void)
{
    NET_FileEntryInfo * new_entry = XP_NEW(NET_FileEntryInfo);

    if(!new_entry) 
       return(NULL);

	XP_MEMSET(new_entry, 0, sizeof(NET_FileEntryInfo));

	new_entry->permissions = -1;

	return(new_entry);

}

/* This function is used as a comparer function for the Qsort routine.
 * It uses a function FE_FileSortMethod() to determine the
 * field to sort on.
 *
 */
MODULE_PRIVATE int
NET_CompareFileEntryInfoStructs (const void *ent2, const void *ent1)
{
    int status;
    const NET_FileEntryInfo *entry1 = *(NET_FileEntryInfo **) ent1;
    const NET_FileEntryInfo *entry2 = *(NET_FileEntryInfo **) ent2;

	if(!entry1 || !entry2)
		return(-1);

    switch(net_file_sort_method)
      {
        case SORT_BY_SIZE:
                        /* both equal or both 0 */
                        if(entry1->size == entry2->size)
                            return(XP_STRCMP(entry2->filename, entry1->filename));
                        else
                            if(entry1->size > entry2->size)
                                return(-1);
                            else
                                return(1);
                        /* break; NOT NEEDED */
        case SORT_BY_TYPE:
                        if(entry1->cinfo && entry1->cinfo->desc && 
										entry2->cinfo && entry2->cinfo->desc) 
                          {
                            status = XP_STRCMP(entry1->cinfo->desc, entry2->cinfo->desc);
                            if(status)
                                return(status);
                            /* else fall to filename comparison */
                          }
                        return (XP_STRCMP(entry2->filename, entry1->filename));
                        /* break; NOT NEEDED */
        case SORT_BY_DATE:
                        if(entry1->date == entry2->date) 
                            return(XP_STRCMP(entry2->filename, entry1->filename));
                        else
                            if(entry1->size > entry2->size)
                                return(-1);
                            else
                                return(1);
                        /* break; NOT NEEDED */
        case SORT_BY_NAME:
        default:
                        return (XP_STRCMP(entry2->filename, entry1->filename));
      }
}

/* sort the files
 */
MODULE_PRIVATE void
NET_DoFileSort(SortStruct * sort_list)
{
    NET_DoSort(sort_list, NET_CompareFileEntryInfoStructs);
}

#ifdef PROFILE
#pragma profile off
#endif

