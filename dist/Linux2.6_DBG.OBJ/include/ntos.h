/**********************************************************************
 *  ntOS.h - functionality used bt NT Operating System
 *
 **********************************************************************/

#ifndef _ntos_h
#define _ntos_h


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/* prototypes for info.c */
typedef enum {
    OS_WIN95,
    OS_WINNT,
    OS_WIN32S,
    OS_UNKNOWN
} OS_TYPE;

typedef enum {
    PROCESSOR_I386,
    PROCESSOR_ALPHA,
    PROCESSOR_MIPS,
    PROCESSOR_PPC,
    PROCESSOR_UNKNOWN
} PROCESSOR_TYPE;
    
OS_TYPE INFO_GetOperatingSystem (); 
DWORD INFO_GetOperatingSystemMajorVersion (); 
DWORD INFO_GetOperatingSystemMinorVersion (); 
void OS_GetComputerName  (LPTSTR computerName, int nComputerNameLength ); 
PROCESSOR_TYPE OS_GetProcessor (); 


/* prototypes for path.c */
DWORD WINAPI PATH_RemoveRelative ( char * path );
DWORD WINAPI PATH_ConvertNtSlashesToUnix( LPCTSTR  lpszNtPath, LPSTR lpszUnixPath );


/* prototypes for registry.c */
BOOL REG_CheckIfKeyExists( HKEY hKey, LPCTSTR registryKey );
BOOL REG_CreateKey( HKEY hKey, LPCTSTR registryKey );
BOOL REG_DeleteKey( HKEY hKey, LPCTSTR registryKey );
		 	
BOOL
REG_GetRegistryParameter(
    HKEY hKey, 
	LPCTSTR registryKey, 
	LPTSTR QueryValueName,
	LPDWORD ValueType,
	LPBYTE ValueBuffer, 
	LPDWORD ValueBufferSize
	);
		 	
BOOL
REG_SetRegistryParameter(
    HKEY hKey, 
	LPCTSTR registryKey, 
	LPTSTR valueName,
	DWORD valueType,
	LPCTSTR ValueString, 
	DWORD valueStringLength
	);

BOOL
REG_GetSubKeysInfo( 
    HKEY hKey, 
    LPCTSTR registryKey, 
    LPDWORD lpdwNumberOfSubKeys, 
    LPDWORD lpdwMaxSubKeyLength 
    );

BOOL
REG_GetSubKey( HKEY hKey, 
    LPCTSTR registryKey, 
    DWORD nSubKeyIndex, 
    LPTSTR registrySubKeyBuffer, 
    DWORD subKeyBufferSize 
    );

/* prototypes for service.c */
#define SERVRET_ERROR     0
#define SERVRET_INSTALLED 1
#define SERVRET_STARTING  2
#define SERVRET_STARTED   3
#define SERVRET_STOPPING  4
#define SERVRET_REMOVED   5

DWORD SERVICE_GetNTServiceStatus(LPCTSTR serviceName, LPDWORD lpLastError );
DWORD SERVICE_InstallNTService(LPCTSTR serviceName, LPCTSTR serviceExe );
DWORD SERVICE_RemoveNTService(LPCTSTR serviceName);
DWORD SERVICE_StartNTService(LPCTSTR serviceName);
DWORD SERVICE_StopNTService(LPCTSTR serviceName);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
