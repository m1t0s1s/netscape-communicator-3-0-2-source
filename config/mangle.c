#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

HANDLE hMangleFile;

void Usage(void)
{
    fprintf(stderr, "MANGLE: <file>\n");
}

BOOL MangleFile( const char *real_name, const char *mangle_name ) 
{
    int len;
    DWORD dwWritten;
    char buffer[2048];

    if( mangle_name && *mangle_name && strcmpi(real_name, mangle_name) ) {
        printf("Mangle: renaming %s to %s\n", real_name, mangle_name);

        if( ! MoveFile(real_name, "X_MANGLE.TMP") ) {
            fprintf(stderr, "MANGLE: cannot rename %s to X_MANGLE.TMP\n", 
                    real_name);
                return FALSE;
        }

        if( ! MoveFile("X_MANGLE.TMP", mangle_name) ) {
            MoveFile("X_MANGLE.TMP", real_name);
            fprintf(stderr, "MANGLE: cannot rename X_MANGLE.TMP to %s\n", 
                    mangle_name);
            return FALSE;
        }

        len = sprintf(buffer, "mv %s %s\r\n", mangle_name, real_name);

        if( (WriteFile( hMangleFile, buffer, len, &dwWritten, NULL ) == FALSE) ||
            (dwWritten != len) ) {
            fprintf(stderr, "MANGLE: error writing to UNMANGLE.BAT\n");
            return FALSE;
        }
    }
    return TRUE;
}


int main( int argc, char *argv[] ) 
{
    WIN32_FIND_DATA find_data;
    HANDLE hFoundFile;

    if( argc != 1 ) {
        Usage();
        return 2;
    }


    hMangleFile = CreateFile("unmangle.bat",    /* name                */
                    GENERIC_READ|GENERIC_WRITE, /* access mode         */
                    0,                          /* share mode          */
                    NULL,                       /* security descriptor */
                    CREATE_NEW,                 /* how to create       */
                    FILE_ATTRIBUTE_NORMAL,      /* file attributes     */
                    NULL );                     /* template file       */

    if( hMangleFile == INVALID_HANDLE_VALUE ) {
        if( GetLastError() == ERROR_FILE_EXISTS ) {
            fprintf(stderr, "MANGLE: UNMANGLE.BAT already exists\n");
        } else {
            fprintf(stderr, "MANGLE: cannot open UNMANGLE.BAT\n");
        }
        return 1;
    }

    if( (hFoundFile = FindFirstFile("*.*", &find_data)) == INVALID_HANDLE_VALUE ) {
        fprintf(stderr, "MANGLE: cannot read directory\n");
        return 1;
    }

    do {
        if( !MangleFile(find_data.cFileName, find_data.cAlternateFileName) ) {
            fprintf(stderr, "MANGLE: cannot rename %s to %s\n",
                    find_data.cFileName, find_data.cAlternateFileName );
    
            FindClose( hFoundFile );
            CloseHandle( hMangleFile );
            return 1;
        }
    } while( FindNextFile(hFoundFile, &find_data) );
    FindClose( hFoundFile );

    {
        int len;
        DWORD dwWritten;
        char buffer[255];

        len = sprintf(buffer, "del unmangle.bat\r\n");
        WriteFile  ( hMangleFile, buffer, len, &dwWritten, NULL );
    }
    CloseHandle( hMangleFile );

    return 0;
}
