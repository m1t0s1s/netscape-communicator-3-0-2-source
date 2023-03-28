
#ifdef __cplusplus
extern "C" {
#endif

/* typedef int (*NW_ReportOrCheck)();  defined bellow */

/*  NW_ReportOrCheck uses the following defines and NW_ReportData structure
 *  to tell the user what, if anything, is being reported.
 */

#define NW_STATUS_CHECK             1
#define NW_STATUS_SHOWMESSAGE       2
#define NW_STATUS_CONFIRM           3
#define NW_STATUS_PROMPT            4
#define NW_STATUS_PASSWORD          5
#define NW_STATUS_USERNAMEPASSWORD  6

#define NW_PROMPT_1_IS_PASSWORD     0x00000001
#define NW_PROMPT_2_IS_PASSWORD     0x00000002
#define NW_PROMPT_YES_NO            0x00000004

typedef struct
{
    int kind;           /* type of report/request, ie: NW_STATUS_CHECK */
    int promptFlags;    /* prompt styles, ie: NW_PROMPT_1_IS_PASSWORD */
    char *message;      /* textual message if appropriate for type of report */
    char *prompt1;      /* prompt text for first entry field */
    char *answer1;      /* answer text from first entry field */
    char *prompt2;      /* prompt text for second entry field */
    char *answer2;      /* answer text from second entry field */
}
    NW_ReportData;

/*  NW_ReportOrCheck() handles all progress and state callbacks for the
 *  user. The user should return non 0 to continue, and 0 to interrupt the
 *  current operation.
 */

typedef int (*NW_ReportOrCheck)(void *fe, NW_ReportData *rd);

/*  NW_FrontEndData represents the user to libnet wrapper.
 */

typedef struct
{
    NW_ReportOrCheck report;
    void *user;     /* user specific, put your own 'context' pointer here */
}
    NW_FrontEndData;

/*  NW_NetReturnData is used to deliver incomming data, such as data from GET
 *  or INDEX, to the user. It should be created and released using the two
 *  routines following it.
 */

typedef struct
{
    char *data;
    uint32 size;
    int32  exit_status;
    char *error_msg;
}
    NW_NetReturnData;

NW_NetReturnData * NW_CreateReturnDataStruct(void);
void NW_FreeNetReturnData(NW_NetReturnData * return_data);

/*  NW_DoNetAction is the main entry point into the libnet wrapper.
 */

int NW_DoNetAction(URL_Struct *url_s, NW_NetReturnData *return_data,
        NW_FrontEndData *fe);

/*
 *  NW_ShutDown terminates the use of NW wrapper as well as libnet subsystem.
 */

void NW_ShutDown(void);

#ifdef __cplusplus
} // extern "C"
#endif
