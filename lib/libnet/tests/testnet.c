#include "xp.h"
#include "memory.h"
#include "net.h"
#include "structs.h"
#include "ctxtfunc.h"

#include "xp_core.h"
#include "xpgetstr.h"

#include "common.h"
#include "netwrap.h"

#ifdef XP_WIN

/*  DNS object cache management
 */

typedef struct DNS_OBJECT__
{
    struct DNS_OBJECT__ *m_next;
    HANDLE m_handle;
    struct hostent *m_hostent;
    BOOL m_finished;
    int m_error;
    char *m_host;
}
    DNS_OBJECT;

DNS_OBJECT *firstDnsObject = 0;

DNS_OBJECT *GetDnsObject(char *host)
{
    DNS_OBJECT *dnsObject;

    dnsObject = firstDnsObject;

    while (dnsObject)
    {
        if (0 == stricmp(dnsObject->m_host, host))
            return dnsObject;

        dnsObject = dnsObject->m_next;
    }

    return 0;
}

DNS_OBJECT *GetDnsObjectUsingHandle(HANDLE handle)
{
    DNS_OBJECT *dnsObject;

    dnsObject = firstDnsObject;

    while (dnsObject)
    {
        /*  Handles may be reused, so check for objects that are not finished
         */
        if ((!dnsObject->m_finished) && (handle == dnsObject->m_handle))
            return dnsObject;

        dnsObject = dnsObject->m_next;
    }

    return 0;
}

DNS_OBJECT *AddDnsObject(char *host)
{
    DNS_OBJECT *dnsObject;

    dnsObject = GetDnsObject(host);
    if (dnsObject) return dnsObject;

    dnsObject = (DNS_OBJECT *)XP_ALLOC(sizeof(DNS_OBJECT));

    dnsObject->m_next = firstDnsObject;
    firstDnsObject = dnsObject;

    dnsObject->m_hostent = (struct hostent *)XP_CALLOC(1, MAXGETHOSTSTRUCT * sizeof(char));
    dnsObject->m_host = XP_STRDUP(host);
    dnsObject->m_error = 0;
    dnsObject->m_finished = FALSE;

    return dnsObject;
}

void FreeDnsObject(char *host)
{
    DNS_OBJECT *dnsObject, *prevDnsObject;

    dnsObject = firstDnsObject;
    prevDnsObject = 0;

    while (dnsObject)
    {
        if (0 == stricmp(dnsObject->m_host, host))
        {
            if (prevDnsObject) prevDnsObject->m_next = dnsObject->m_next;
            else firstDnsObject = dnsObject->m_next;

            if (dnsObject->m_hostent) XP_FREE(dnsObject->m_hostent);
            if (dnsObject->m_host) XP_FREE(dnsObject->m_host);
            XP_FREE(dnsObject);

            return;
        }

        prevDnsObject = dnsObject;
        dnsObject = dnsObject->m_next;
    }
}

void FreeAllDnsObjects(void)
{
    while (firstDnsObject)
    {
        DNS_OBJECT *nextDnsObject;

        nextDnsObject = firstDnsObject->m_next;

        if (firstDnsObject->m_hostent) XP_FREE(firstDnsObject->m_hostent);
        if (firstDnsObject->m_host) XP_FREE(firstDnsObject->m_host);
        XP_FREE(firstDnsObject);

        firstDnsObject = nextDnsObject;
    }

    firstDnsObject = 0;
}

/*
 *  DNS notification window (Windows only)
 */

#define DNSWINDOWMESSAGE    (WM_USER + 1000)
#define DNSWINDOWMESSAGE1   (WM_USER + 1001)

HWND DNSWindow = 0;

LRESULT CALLBACK NWDNSWindowProc(
    HWND        hWnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    switch (message)
    {
        case DNSWINDOWMESSAGE:
        {
            DNS_OBJECT *dnsObject;

            dnsObject = GetDnsObjectUsingHandle((HANDLE)wParam);

            if (dnsObject)
            {
                dnsObject->m_finished = TRUE;
                dnsObject->m_error = WSAGETASYNCERROR(lParam);
            }

            break;
        }

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

BOOL CreateDNSWindow(void)
{
    WNDCLASS windowClass;

    if (DNSWindow) return TRUE;

    windowClass.style = 0;
    windowClass.lpfnWndProc = NWDNSWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = "__nw_dns_window_class__";

    if (0 == RegisterClass(&windowClass))
        return NULL;

    DNSWindow = CreateWindow("__nw_dns_window_class__", "_nw_dns_window_",
        WS_DISABLED | WS_OVERLAPPED, -1000, -1000, 10, 10,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    return (NULL != DNSWindow);
}

void DestroyDNSWindow(void)
{
    DestroyWindow(DNSWindow);
    DNSWindow = 0;
}


int FE_SetupAsyncSelect(unsigned int sock, MWContext * context)
{
    /* WSAAsyncSelect(sock, DNSWindow, DNSWINDOWMESSAGE1, 0); */
    return 0;
}

int FE_StartAsyncDNSLookup(
        MWContext *context,
        char * host_port,
        void **hoststruct_ptr_ptr,
        int sock)
{

    DNS_OBJECT *dnsObject;
    int result;

    *hoststruct_ptr_ptr = NULL;
    result = MK_WAITING_FOR_LOOKUP;
    
    dnsObject = GetDnsObject(host_port);

    if (!dnsObject)
    {
        /* start the lookup
         */
        dnsObject = AddDnsObject(host_port);
        dnsObject->m_handle = WSAAsyncGetHostByName(DNSWindow, DNSWINDOWMESSAGE,
                host_port, (char *)dnsObject->m_hostent, MAXGETHOSTSTRUCT);
    }
    else if (dnsObject->m_finished)
    {
        if (0 == dnsObject->m_error)
        {
            /*  copy data to hoststruct_ptr_ptr
             */
            *hoststruct_ptr_ptr = dnsObject->m_hostent;
            result = 0;
        }
        else
        {
            /*  DNS lookup failed -- delete object from list
             */
            FreeDnsObject(host_port);
            result = -1;
        }
    }

    return result;

}
#endif /* XP_WIN */

extern int MK_OUT_OF_MEMORY;
extern int XP_TRANSFER_INTERRUPTED;

NW_NetReturnData *NW_CreateReturnDataStruct(void)
{
	return(XP_CALLOC(1, sizeof(NW_NetReturnData)));
}

void NW_FreeNetReturnData(NW_NetReturnData * return_data)
{
	XP_ASSERT(return_data);
	if(!return_data)
		return;

	if(return_data->data)
	  {
		XP_FREE(return_data->data);
		return_data->data = NULL;
	  }

    if (return_data->error_msg)
	  {
        XP_FREE(return_data->error_msg);
		return_data->error_msg = NULL;
	  }

	XP_FREE(return_data);
}

void TestExit(URL_Struct *URL_s, int status, MWContext *window_id)
{
	NW_NetReturnData *return_data = (NW_NetReturnData *)URL_s->fe_data;

	XP_ASSERT(return_data);

    if(return_data)
    {
        return_data->exit_status = status;
        if (URL_s->error_msg)
            return_data->error_msg = XP_STRDUP(URL_s->error_msg);
    }

    /*
	if(URL_s->error_msg)
 	  {  
		FE_Alert(window_id, URL_s->error_msg);
		printf(URL_s->error_msg);
	  }
    */
}

int FE_GetURL (MWContext *context, URL_Struct *url)
{
    /* Pass in context, not &context, right?  CCM */
    return NET_GetURL(url, FO_PRESENT, context, TestExit);
}

#ifdef XP_WIN
char * NOT_NULL (const char *x)
{
    return (char*)x;
}
#endif

typedef struct {
	XP_File   fp;
	char	* filename;
} FileWriteConverterDataObject;

PRIVATE int lw_SaveToDiskWrite (FileWriteConverterDataObject * obj,char* s, int32 l)
{
    XP_FileWrite(s, l, obj->fp); 
    return(1);
}

/* is the stream ready for writeing?
 */
PRIVATE unsigned int lw_SaveToDiskWriteReady (FileWriteConverterDataObject * obj)
{
   return(MAX_WRITE_READY);  /* always ready for writing */ 
}

PRIVATE void lw_SaveToDiskComplete (FileWriteConverterDataObject * obj)
{
    XP_FileClose(obj->fp);

    if(obj->filename)
    	XP_FREE(obj->filename);

    XP_FREE(obj);
    return;
}

PRIVATE void lw_SaveToDiskAbort (FileWriteConverterDataObject * obj, int status)
{
    XP_FileClose(obj->fp);

    if(obj->filename)
      {
        remove(obj->filename);
        XP_FREE(obj->filename);
      }

    return;
}

NET_StreamClass * LW_FileWriteConverter2 (int format_out,
    									void *registration,  
    									URL_Struct * request,
    									MWContext *context) 
{
    NET_StreamClass * new_stream = XP_CALLOC(1, sizeof(NET_StreamClass));
	FileWriteConverterDataObject * data_obj;
	char *filename;

	if(!new_stream)
		return NULL;

	data_obj = XP_CALLOC(1, sizeof(FileWriteConverterDataObject));

	if(!data_obj)
		return NULL;

    new_stream->name           = "FileWriter";
    new_stream->complete       = (MKStreamCompleteFunc) lw_SaveToDiskComplete;
    new_stream->abort          = (MKStreamAbortFunc) lw_SaveToDiskAbort;
    new_stream->put_block      = (MKStreamWriteFunc) lw_SaveToDiskWrite;
    new_stream->is_write_ready = (MKStreamWriteReadyFunc) lw_SaveToDiskWriteReady;
    new_stream->data_object    = data_obj;  /* document info object */
    new_stream->window_id      = context;

	filename = "foo.bar";  /* @#@@@@@ */

    StrAllocCopy(data_obj->filename, filename);

	if(!(data_obj->fp = XP_FileOpen(data_obj->filename, xpTemporary, XP_FILE_WRITE_BIN)))
		return NULL;

    TRACEMSG(("Returning stream from LW_SaveToDiskConverter\n"));
	
    return new_stream;
}

typedef struct {
	char    * data;
	uint32    data_size;
	uint32    malloc_size;
	NW_NetReturnData *return_data;
} MemWriteConverterDataObject;

#define EXPAND_INTERVAL_SIZE 1024

PRIVATE int lw_SaveToMemWrite (MemWriteConverterDataObject * obj,char* s, int32 l)
{ 
	if(obj->malloc_size < obj->data_size+l)
	{
		int32 expand_by;

		/* expand malloc_size */
		if(l > EXPAND_INTERVAL_SIZE)
			expand_by = l;
		else
			expand_by = EXPAND_INTERVAL_SIZE;
		
		if(obj->data_size > 0)
			obj->data = XP_REALLOC(obj->data, (obj->malloc_size+expand_by)*sizeof(char));
		else
			obj->data = XP_ALLOC(expand_by*sizeof(char));
		
		obj->malloc_size += expand_by;
		
		if(!obj->data)
			return(MK_OUT_OF_MEMORY);
	}

	XP_MEMCPY(obj->data+obj->data_size, s, l);
	obj->data_size += l;

    return(1);
}

/* is the stream ready for writeing?
 */
PRIVATE unsigned int lw_SaveToMemWriteReady (MemWriteConverterDataObject * obj)
{
   return(MAX_WRITE_READY);  /* always ready for writing */ 
}

PRIVATE void lw_SaveToMemComplete (MemWriteConverterDataObject * obj)
{
    
	obj->return_data->data = obj->data;
	obj->return_data->size = obj->data_size;

    XP_FREE(obj);
    return;
}

PRIVATE void lw_SaveToMemAbort (MemWriteConverterDataObject * obj, int status)
{

	if(obj->data)
		XP_FREE(obj->data);
	XP_FREE(obj);
    return;
}

NET_StreamClass * LW_MemWriteConverter (int format_out,
    									void *registration,  
    									URL_Struct * URL_s,
    									MWContext *context) 
{
    NET_StreamClass * new_stream = XP_CALLOC(1, sizeof(NET_StreamClass));
	MemWriteConverterDataObject * data_obj;

	if(!new_stream)
		return NULL;

	data_obj = XP_CALLOC(1, sizeof(MemWriteConverterDataObject));

	if(!data_obj)
		return NULL;

    new_stream->name           = "Memory Writer";
    new_stream->complete       = (MKStreamCompleteFunc) lw_SaveToMemComplete;
    new_stream->abort          = (MKStreamAbortFunc) lw_SaveToMemAbort;
    new_stream->put_block      = (MKStreamWriteFunc) lw_SaveToMemWrite;
    new_stream->is_write_ready = (MKStreamWriteReadyFunc) lw_SaveToMemWriteReady;
    new_stream->data_object    = data_obj;  /* document info object */
    new_stream->window_id      = context;

    TRACEMSG(("Returning stream from LW_SaveToMemConverter\n"));

	data_obj->return_data = URL_s->fe_data;

    return new_stream;
}


#define MAKE_FE_FUNCS_PREFIX(f)         LWFE_##f
#define MAKE_FE_FUNCS_EXTERN
#include "mk_cx_fn.h"


#if defined(XP_WIN) || defined(XP_MAC) 
extern char * XP_AppCodeName;    /* Changed to match ns/include/gui.h  CCM */
extern char * XP_AppVersion;
#else
extern const char *XP_AppName, *XP_AppCodeName, *XP_AppVersion;
#endif

void CheckForHtmlError(NW_NetReturnData *return_data, URL_Struct *url_s)
{
    /*  A bunch of checks to guess whether there was an error, even though none
     *  is indicated.
     */

    /*  First check whether there is already an error message. If so we can
     *  just return. Also, if there is no data, we can go on because there is
     *  no text to use as error message.
     */
    if (return_data->error_msg)
        return;

    /*  Now check whether the error status is positive. If so, there may still
     *  be an error. This happens if an HTTP server returns something outside
     *  the 200-299 range.
     */
    if (return_data->exit_status >= 0)
    {
        if (NET_URL_Type(url_s->address) != HTTP_TYPE_URL)
            return;

        if ((url_s->server_status >= 200) && (url_s->server_status <= 299))
            return;

        /*  An HTTP server error has occured. If the data field is not empty,
         *  then we have an html error.
         */
        return_data->exit_status = -1;
        if (return_data->size != 0)
            goto ThereIsAnHtmlError;

        /*  We have to make up an HTTP error message.
         */
        if (url_s->server_status == 0)
            return_data->error_msg =
                    XP_STRDUP("The server did not respond.");
        else
            return_data->error_msg =
                    XP_STRDUP("The server could not complete the operation.");
        return;
    }

    /*  There is an error, but no error message. Check to see if there is
     *  data. If not, we have no text, so we have to make up a message.
     *
     *  This condition can happen when contacting an FTP server. I tried to
     *  HEAD ftp://mocha/pub/site0/ and ended up here.
     */
    if (return_data->size == 0)
    {
        return_data->error_msg =
                XP_STRDUP("Network or file operation failed.");
        return;
    }

ThereIsAnHtmlError:

    /*  Since there is no error message, and there is result data, then
     *  this data is the error message. It needs to be deHTMLized.
     */
    {
        char *p1, *p2, *pEnd;
        int isTag = FALSE;

        return_data->error_msg = XP_ALLOC(return_data->size + 1);

        p1 = return_data->data;
        p2 = return_data->error_msg;

        pEnd = p1 + return_data->size;

        while ((p1 < pEnd) && *p1)
        {
            if (isTag)
            {
                if (*p1 == '>') isTag = FALSE;
            }
            else if (*p1 == '<')
            {
                isTag = TRUE;
            }
            else
            {
                *p2 = *p1;
                p2++;
            }
            p1++;
        }

        *p2 = '\0';
    }
}

int NW_DoNetAction(URL_Struct *url_s, NW_NetReturnData *return_data,
        NW_FrontEndData *fe)
{
    static MWContext context = {NULL};
	int status, type;

    NW_ReportData rd;

    type = NET_URL_Type(url_s->address);
    
    if(type == SECURE_HTTP_TYPE_URL)
    {
        return_data->exit_status = -1;
        return_data->error_msg = XP_STRDUP(
                "Cannot peform this operation with a secure server");
        return -1;
    }

    if(type == HTTP_TYPE_URL && url_s->method == URL_PUT_METHOD)
    {
        /* HTTP servers need a content length in the request
         * add one here.
         */
        XP_ASSERT(url_s->post_headers == NULL);
        url_s->post_headers = PR_smprintf("Content-Length: %ld"CRLF, 
                                          url_s->post_data_size);        
    }
    

	if(!context.funcs)
	  {
		static ContextFuncs  functions;
#ifdef XP_WIN
		WORD version_requested;
		WSADATA winsock_data;

		version_requested = MAKEWORD(1,1);

		status = WSAStartup(version_requested, &winsock_data);

		if(status != 0)
		  {
			printf("could not initialize winsock");
			exit(1);
		  }
#endif
		XP_AppCodeName = XP_STRDUP("Livewire");
		XP_AppVersion = XP_STRDUP(".9");

		NET_InitNetLib(1024*10, 4);

		NET_RegisterContentTypeConverter("*", FO_PRESENT, 0, LW_MemWriteConverter);


		memset( &context, 0, sizeof (context) );
		memset( &functions, 0, sizeof (functions) );

		context.type = MWContextBrowser;

		#define MAKE_FE_FUNCS_PREFIX(f)     LWFE_##f
		#define MAKE_FE_FUNCS_ASSIGN        functions.
		#include "mk_cx_fn.h"

		context.funcs = &functions;

#ifdef XP_WIN
        CreateDNSWindow();
#endif /* XP_WIN */
	  }

    context.fe.generic_data = fe;
    rd.kind = NW_STATUS_CHECK;
    rd.message = 0;

	url_s->fe_data = return_data;

    NET_GetURL(url_s, FO_PRESENT, &context, TestExit);

	while (0 < NET_ProcessNet((NETSOCK)-1, NET_UNKNOWN_FD))
    {
		/* spin loop to process data */
        if (!fe->report(fe->user, &rd))
        {
            NET_InterruptWindow(&context);
            return_data->exit_status = XP_TRANSFER_INTERRUPTED;
            return_data->error_msg = XP_STRDUP(XP_GetString(XP_TRANSFER_INTERRUPTED));
            break;
        }
    }

    CheckForHtmlError(return_data, url_s);

	return(return_data->exit_status);
}

void NW_ShutDown(void)
{
#ifdef XP_WIN
    WSACleanup();
    FreeAllDnsObjects();
#endif /* XP_WIN */

    NET_ShutdownNetLib();
}

#ifndef NW_WRAPPER

int ReportOrCheck(NW_FrontEndData *fe, NW_ReportData *rd)
{
    return 1;
}    

static int
parse_method(char *method_name)
{
	if(!strcasecomp(method_name, "HEAD"))
		return(URL_HEAD_METHOD);
	else if(!strcasecomp(method_name, "PUT"))
		return(URL_PUT_METHOD);
	else if(!strcasecomp(method_name, "DELETE"))
		return(URL_DELETE_METHOD);
	else if(!strcasecomp(method_name, "MOVE"))
		return(URL_MOVE_METHOD);
	else if(!strcasecomp(method_name, "GET"))
		return(URL_GET_METHOD);
	else 
		return(URL_GET_METHOD); /* default */
}

void main(int argc, char ** argv)
{
    URL_Struct *url_s = NULL;
	int status;
    NW_FrontEndData fe;
	NW_NetReturnData * return_data;
	int url_method = URL_GET_METHOD; /* default method */

	PR_Init("mozilla", 24, 1, 0);

	return_data = NW_CreateReturnDataStruct();
	
    fe.report = ReportOrCheck;
    fe.user = 0;

	while(--argc)
	  {
		if(!strcasecomp(argv[argc], "-debug"))
		  {
			MKLib_trace_flag = 2;
		  }
		else if(!strcasecomp(argv[argc], "-method"))
		  {
			url_method = parse_method(argv[argc+1]);
		  }
		else if(NET_URL_Type(argv[argc]))
		  {
			url_s = NET_CreateURLStruct(argv[1], NET_NORMAL_RELOAD);
		  }
	  }
		
	if(!url_s)
		url_s = NET_CreateURLStruct("http://warp/", NET_NORMAL_RELOAD);

	if(!url_s)
		exit(1);

	url_s->method = url_method;

	/*
		*methods*
		url_s->method = URL_GET_METHOD;
		url_s->method = URL_HEAD_METHOD;
		url_s->method = URL_PUT_METHOD;
		url_s->method = URL_DELETE_METHOD;
   		url_s->method = URL_MOVE_METHOD;
	*/

	status = NW_DoNetAction(url_s, return_data, &fe);


	if(url_s->content_type)
		printf("Content type: %s\n", url_s->content_type);

	if(url_s->content_length)
		printf("Content length: %d\n\n", url_s->content_length);

	if(return_data->data)
	  {
		printf("data:\n");
		printf(return_data->data);
	  }

	if(return_data->exit_status < 0 && return_data->error_msg)
		printf("error: %s\n", return_data->error_msg);

	NET_FreeURLStruct(url_s);

	NW_FreeNetReturnData(return_data);

}

#endif  /* LW_WRAPPER */

