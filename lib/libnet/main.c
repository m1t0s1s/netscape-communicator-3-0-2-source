#include "mkutils.h"
#include "mkgeturl.h"
#include "mkparse.h"
#include "mkinit.h"
#include "xp.h"
#include "mkformat.h"
#include "glhist.h"
#include "cvproxy.h"

/* comment from Jeff's account */

#include "mkstream.h"

#include "mktcp.h"

/* comment from Phil's account */

#include "pa_parse.h"
#include "parse.h"  /* temporary */

#define BUFFER_SIZE 2048

#define MAKE_FE_FUNCS_PREFIX(func) TESTFE_##func
#define MAKE_FE_FUNCS_EXTERN
#include "mk_cx_fn.h"


/* stuff for select
 */
PRIVATE fd_set net_read_fds;
PRIVATE fd_set net_connect_fds;
PRIVATE int    net_fdset_size;
PRIVATE XP_List * select_list=0;

char *log_file_name=0;  /* for WAIS */

PRIVATE void parse_and_retrieve_headers (URL_Struct *URL_s);
PUBLIC void sample_exit_routine(URL_Struct *URL_s,int status,MWContext *window_id);
PRIVATE void call_process_net(void);

/* a comment from lou to test CVS */

int main (int argc, char **argv)
{
    char input_buffer[BUFFER_SIZE];
    char request[40], URL[2000], type[40];
    int fields;
    URL_Struct * URL_s=0;
    Bool user_mode = TRUE;
    Bool http1 = FALSE;
    Bool parse_mode = FALSE;
    int request_type;
    MWContext context;
    XP_StatStruct stat_struct;
    PA_Functions funcs;
	ContextFuncs cfuncs;

	/* build the context up
	 */
	memset(&context, 0, sizeof(MWContext));
	context.funcs = &cfuncs;

	memset(&cfuncs, 0, sizeof(ContextFuncs));

    cfuncs.Progress = TESTFE_Progress;
    cfuncs.Alert = TESTFE_Alert;
    cfuncs.SetCallNetlibAllTheTime = TESTFE_SetCallNetlibAllTheTime;
    cfuncs.ClearCallNetlibAllTheTime = TESTFE_ClearCallNetlibAllTheTime;
    cfuncs.GraphProgressInit = TESTFE_GraphProgressInit;
    cfuncs.GraphProgressDestroy = TESTFE_GraphProgressDestroy;
    cfuncs.GraphProgress = TESTFE_GraphProgress;
    cfuncs.UseFancyFTP = TESTFE_UseFancyFTP;
    cfuncs.UseFancyNewsgroupListing = TESTFE_UseFancyNewsgroupListing;
    cfuncs.FileSortMethod = TESTFE_FileSortMethod;
    cfuncs.ShowAllNewsArticles = TESTFE_ShowAllNewsArticles;
    cfuncs.Confirm = TESTFE_Confirm;
    cfuncs.Prompt = TESTFE_Prompt;
    cfuncs.PromptPassword = TESTFE_PromptPassword;
    cfuncs.PromptUsernameAndPassword = TESTFE_PromptUsernameAndPassword;
    cfuncs.EnableClicking = TESTFE_EnableClicking;
    cfuncs.AllConnectionsComplete = TESTFE_AllConnectionsComplete;
	
    if(XP_Stat(".WWWtraceon", &stat_struct, xpGeneric) != -1)
	    MKLib_trace_flag = 2;

    argc--;
    while(argc > 0) {
	    if(!strncmp(argv[argc],"-u",2))
	        user_mode = TRUE;
	    else if(!strncmp(argv[argc],"-s",2))
	        user_mode = FALSE;

	    argc--;
    }

    XP_MEMSET(&net_read_fds, 0, sizeof(fd_set));
    XP_MEMSET(&net_connect_fds, 0, sizeof(fd_set));
    net_fdset_size = 0;

    /* initializations 
     */
    NET_InitNetLib(32768, 20); 
/*
    NET_InitNetLib(32, 20); 
*/

	NET_SetMemoryCacheSize(20);
	NET_SetDiskCacheSize(10000000);

	NET_SetMailRelayHost("warp.netscape.com");

	GH_SetGlobalHistoryTimeout(600);

    funcs.mem_allocate = NULL;
    funcs.mem_free = NULL;
    funcs.PA_ParsedTag = LO_Format;

    PA_ParserInit(&funcs);

    if(user_mode) /* */ 
	  {
	    MKLib_trace_flag = 2;

		NET_SetProxyServer(FTP_PROXY, getenv("ftp_proxy"));
		NET_SetProxyServer(HTTP_PROXY, getenv("http_proxy"));
		NET_SetProxyServer(GOPHER_PROXY, getenv("gopher_proxy"));
		NET_SetProxyServer(NEWS_PROXY, getenv("news_proxy"));
		NET_SetProxyServer(WAIS_PROXY, getenv("wais_proxy"));
		NET_SetProxyServer(NO_PROXY, getenv("no_proxy"));
	  }

	NET_RegisterExternalViewerCommand(TEXT_HTML, "netscape %s&", 0); 
	NET_RegisterContentTypeConverter("*", FO_OUT_TO_PROXY_CLIENT,
                                        (void *) 0, NET_ProxyConverter);
	NET_RegisterContentTypeConverter("*", FO_CACHE_AND_OUT_TO_PROXY_CLIENT,
                                        (void *) 0, NET_CacheConverter);
	NET_RegisterContentTypeConverter("*", FO_CACHE_AND_PRESENT,
                                        (void *) 0, NET_CacheConverter);


    /* #### temporary hack to override types */
    NET_RegisterExternalViewerCommand ("*", "xv %s&", 0);

    /* user input loop for testing */
    do {

	    if(user_mode)
	        printf("\n\nURL> ");
	    fgets(input_buffer, BUFFER_SIZE, stdin);

	    /* user quit routine */
	    if((user_mode && toupper(*input_buffer) == 'Q') ||
	       *input_buffer == 4)
		  {
    		NET_ShutdownNetLib();
	        exit(0);
		  }

		/* separate the request */
		fields = sscanf(input_buffer, "%40s %2000s %40s", request, URL, type);

		if(user_mode && !strcasecomp(request, "GO"))
	  	{
	    	fprintf(stderr,"\nStarting ProcessNet loop\n");
	    	call_process_net();
	    	continue;
	  	}

		if(user_mode && !strcasecomp(request, "PARSE"))
	    	parse_mode = TRUE;	
	

		if(user_mode && !strncasecomp(request, "int",3))
	  	{
	    	fprintf(stderr,"\nInterrupting process #%d\n", atoi(URL));
	    	NET_InterruptWindow((MWContext *)atoi(URL));
	    	continue;
      	}

		if(fields < 2)
	   	continue;

		/* check for HTTP/1.0 */
		if(fields > 2 && !strcasecomp(type,"HTTP/1.0"))
	    	http1 = TRUE;
		else
 	    	http1 = FALSE;

		URL_s = NET_CreateURLStruct(URL, FALSE);

		if(!strcmp(request,"POST"))
	   		URL_s->method = URL_POST_METHOD;

		if(http1) 
	    	parse_and_retrieve_headers(URL_s);
	
		if(user_mode)
		  {
	    	fprintf(stderr,"\n\nNET_GetURL returned with: %d\n",
		     	NET_GetURL(URL_s, FO_CACHE_AND_PRESENT, &context,sample_exit_routine));
		  }
		else
		  {
			/* only cache http docs
			 */
			if(!strncasecomp(URL_s->address, "http:",5))
	    	    NET_GetURL(URL_s, FO_CACHE_AND_OUT_TO_PROXY_CLIENT, &context, sample_exit_routine);
			else
	    	    NET_GetURL(URL_s, FO_OUT_TO_PROXY_CLIENT, &context, sample_exit_routine);
		  }

		if(!user_mode)
	    	call_process_net();


    } while(user_mode);

    NET_ShutdownNetLib();

    return(0);
}

PRIVATE void call_process_net()
{
    fd_set read_fds;
    fd_set write_fds;
    fd_set exps_fds;
    struct timeval timeout;
    int not_done = TRUE;
    int ret, i;

    timeout.tv_sec = 5;
    timeout.tv_usec = 100000;

    TRACEMSG(("Entered call_process_net()\n"));

    if(!XP_ListCount(select_list))
      {
		TRACEMSG(("No fd's to proccess\n"));
		return;
	  }

    while(not_done)
      {

        XP_MEMCPY(&read_fds, &net_read_fds, sizeof(fd_set));
        XP_MEMCPY(&write_fds, &net_connect_fds, sizeof(fd_set));
        XP_MEMCPY(&exps_fds, &net_connect_fds, sizeof(fd_set));

        ret = select(net_fdset_size, &read_fds, &write_fds, &exps_fds, &timeout);

        TRACEMSG(("%d sockets ready\n",ret));

		if(ret == -1)
		  {
			TRACEMSG(("set size: %d   ",net_fdset_size));
            for(i=0; i < net_fdset_size; i++)
                if(FD_ISSET(i, &net_read_fds) || FD_ISSET(i, &net_connect_fds))
                  {
					TRACEMSG(("fd: %d  is set ",i));
                  }
			return;

		  }

		if(ret > 0)
            for(i=0; i < net_fdset_size; i++)
	          {
		        if(FD_ISSET(i, &read_fds) || FD_ISSET(i, &write_fds) || FD_ISSET(i, &exps_fds))
		          {
					TRACEMSG(("fd: %d  is set calling ProcessNet",i));
			        not_done = NET_ProcessNet(i, NET_UNKNOWN_FD);
		            break;
		          }
	          }

		/*
		sginap(.02*CLK_TCK);
		*/
 
	  }

   if(XP_ListCount(select_list))
       TRACEMSG(("Error! %d fds still in select list\n", XP_ListCount(select_list)));
}

PRIVATE void parse_and_retrieve_headers (URL_Struct *URL_s)
{
    char in_buf[BUFFER_SIZE];
    int content_length=0;

    StrAllocCopy(URL_s->http_headers,""); /* initialize */

	/* get the first line */
    fgets(in_buf, BUFFER_SIZE, stdin);

    /* copy everything except the final blank line */
    while(strcmp(in_buf, "\n") && strcmp(in_buf,"\r\n")) {

	    StrAllocCat(URL_s->http_headers,in_buf);

        if(NET_URLStruct_Method(URL_s) == URL_POST_METHOD &&
			!strncasecomp(in_buf,"CONTENT-LENGTH:",15))
             {
                content_length = atoi((char *)XP_StripLine(in_buf+15));
		        TRACEMSG(("Got content length from post headers: %d\n", content_length));
             }

        if(!strncasecomp(in_buf,"Pragma: no-cache:",17))
		  {
		    TRACEMSG(("Got Pragma No cache header\n"));
			URL_s->force_reload = NET_NORMAL_RELOAD;
		  }

	     fgets(in_buf, BUFFER_SIZE, stdin);
    }

	/* read the post data */
    if(NET_URLStruct_Method(URL_s) == URL_POST_METHOD && content_length > 0) {
	int bytes_read=0;
	URL_s->post_data = (char *)XP_ALLOC(content_length);
	   if(!URL_s->post_data) 
		return;
	for(;bytes_read < content_length; bytes_read++) 
	    URL_s->post_data[bytes_read] = getc(stdin);

	TRACEMSG(("Got post data: %s\n",URL_s->post_data));
    }

}

PUBLIC void 
sample_exit_routine(URL_Struct *URL_s, int status, MWContext *window_id)
{
    TRACEMSG(("Exit Routine: All done; Freeing URL Struct; status=%d\n",status));

    if(status < 0)
	printf("\
<TITLE>Error</TITLE><H1>Error</H1>\n\
Unable to load requested item for reason: %s\n\
<HR>If you think the proxy may be in error please talk to Lou<p>:lou\n", NET_ExplainError(status));

    NET_FreeURLStruct(URL_s);
}

/* set a socket for READ selection
 */
MODULE_PRIVATE void
FE_SetReadSelect(MWContext * win_id, NETSOCK sock)
{
    if(!select_list)
	   select_list = XP_ListNew();

    XP_ListAddObject(select_list, (void *)sock);

    FD_SET(sock, &net_read_fds);

    TRACEMSG(("Setting readfd #%d\n",sock));

    if(sock >= net_fdset_size)
        net_fdset_size = sock+1;
}

/* set a socket for CONNECT selection
 * actually select on write and exception
 */
MODULE_PRIVATE void
FE_SetConnectSelect(MWContext * win_id, NETSOCK sock)
{
    if(!select_list)
	   select_list = XP_ListNew();

    FD_SET(sock, &net_connect_fds);

    XP_ListAddObject(select_list, (void *)sock);

    TRACEMSG(("Setting connect fd #%d\n",sock));

    if(sock >= net_fdset_size)
        net_fdset_size = sock+1;
}

MODULE_PRIVATE void
FE_ClearReadSelect(MWContext * win_id, NETSOCK sock)
{
    TRACEMSG(("Clearing read fd #%d\n",sock));

    XP_ListRemoveObject(select_list, (void *)sock);

    if(FD_ISSET(sock, &net_read_fds));
        FD_CLR(sock, &net_read_fds);
}

MODULE_PRIVATE void
FE_ClearConnectSelect(MWContext * win_id, NETSOCK sock)
{
    TRACEMSG(("Clearing connect fd #%d\n",sock));

    XP_ListRemoveObject(select_list, (void *)sock);

    if(FD_ISSET(sock, &net_connect_fds));
        FD_CLR(sock, &net_connect_fds);
}

MODULE_PRIVATE void
TESTFE_SetCallNetlibAllTheTime(MWContext *win_id)
{
	FE_SetConnectSelect(win_id, 1);
}

MODULE_PRIVATE void
TESTFE_ClearCallNetlibAllTheTime(MWContext *win_id)
{
	FE_ClearConnectSelect(win_id, 1);
}

MODULE_PRIVATE void
FE_SetFileReadSelect(MWContext * win_id, int file_no)
{
    if(!select_list)
       select_list = XP_ListNew();

    FD_SET(file_no, &net_read_fds);

    XP_ListAddObject(select_list, (void *)file_no);

    TRACEMSG(("Setting read file fd #%d\n",file_no));

    if(file_no >= net_fdset_size)
        net_fdset_size = file_no+1;
}

MODULE_PRIVATE void
FE_ClearFileReadSelect(MWContext * win_id, int file_no)
{
    TRACEMSG(("Clearing file read fd #%d\n",file_no));

    XP_ListRemoveObject(select_list, (void *)file_no);

    if(FD_ISSET(file_no, &net_read_fds));
        FD_CLR(file_no, &net_read_fds);
}

