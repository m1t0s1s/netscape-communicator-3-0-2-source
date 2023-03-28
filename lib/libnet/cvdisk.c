/* Please leave outside of ifdef for windows precompiled headers */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT

#include "mkstream.h"
#include "mkgeturl.h"
#include "xp.h"

typedef struct _DataObject {
    FILE * fp;
    char * filename;
} DataObject;


PRIVATE int net_SaveToDiskWrite (DataObject * obj, CONST char* s, int32 l)
{
    fwrite(s, 1, l, obj->fp); 
    return(1);
}

/* is the stream ready for writeing?
 */
PRIVATE unsigned int net_SaveToDiskWriteReady (DataObject * obj)
{
   return(MAX_WRITE_READY);  /* always ready for writing */ 
}


PRIVATE void net_SaveToDiskComplete (DataObject * obj)
{
    fclose(obj->fp);

    FREEIF(obj->filename);

    FREE(obj);
    return;
}

PRIVATE void net_SaveToDiskAbort (DataObject * obj, int status)
{
    fclose(obj->fp);

    if(obj->filename)
      {
        remove(obj->filename);
        FREE(obj->filename);
      }

    return;
}


PUBLIC NET_StreamClass * 
fe_MakeSaveAsStream (int         format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id)
{
    DataObject* obj;
    NET_StreamClass* stream;
    static int count=0;
    char filename[256];
    FILE *fp = stdout;
    
    TRACEMSG(("Setting up display stream. Have URL: %s\n", URL_s->address));

    PR_snprintf(filename, sizeof(filename), "foo%d.unknown",count++);
    fp = fopen(filename,"w");

    stream = XP_NEW(NET_StreamClass);
    if(stream == NULL) 
        return(NULL);

    obj = XP_NEW(DataObject);
    if (obj == NULL) 
        return(NULL);
    

    stream->name           = "FileWriter";
    stream->complete       = (MKStreamCompleteFunc) net_SaveToDiskComplete;
    stream->abort          = (MKStreamAbortFunc) net_SaveToDiskAbort;
    stream->put_block      = (MKStreamWriteFunc) net_SaveToDiskWrite;
    stream->is_write_ready = (MKStreamWriteReadyFunc) net_SaveToDiskWriteReady;
    stream->data_object    = obj;  /* document info object */
    stream->window_id      = window_id;

    obj->fp = fp;
    obj->filename = 0;
    StrAllocCopy(obj->filename, filename);

    TRACEMSG(("Returning stream from NET_SaveToDiskConverter\n"));

    return stream;
}

#endif /* MOZILLA_CLIENT */
