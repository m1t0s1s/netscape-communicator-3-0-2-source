/*
 * prototypes used by the front end
 */
typedef void (*SECNAVDeleteSecModCallback)(void *);

char **SECMOD_GetModuleNickNames(int *count);
void SECMOD_FreeNicknames(char **names,int count);
void SECNAV_EditModule(MWContext *context, char *name);
void SECNAV_LogoutSecurityModule(MWContext *context, char *name);
void SECNAV_NewSecurityModule(MWContext *context, void (*func)(void*), void *);
void SECNAV_LogoutAllSecurityModules(MWContext *context);
void SECNAV_DeleteSecurityModule(MWContext *context, char *name,
        SECNAVDeleteSecModCallback func, void *arg);


