/**************************************************************************************
 *
 * This file contains a test program for the function PR_FormatTimeUsEnglish().
 *
 *
 **************************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <locale.h>
#include "prtime.h"
#include "prtypes.h"


#define  timeBufferSize 1024
   

int main( int argc, char* argv[] )
{
   char    oldTimeBuffer[ timeBufferSize ];
   char    newTimeBuffer[ timeBufferSize ];
   size_t  oldReturn;
   size_t  newReturn;

   int64         time;
   int64         timeIncr;
   PRTime        expandedTime;

   char*         format = "%a %A %b %B";
   int           errorCount = 0;
   int           verbose = 0;
   int           i;

   /* Setup the timeIncr value.   We increment time by a day + 4 min + 3 seconds.
    * In 370 iterations of the loop we get to see (almost) all values of each field
    * appear.
    */
   {
      int64     usecPerSec;
      int64     secPerDay;
      int64     usecPerDay;
      int64     secIn4Min3Sec;
      
      LL_I2L(usecPerSec, PR_USEC_PER_SEC);
      LL_I2L(secPerDay, (60 * 60 * 24));
      LL_MUL(usecPerDay, usecPerSec, secPerDay);
      LL_I2L(secIn4Min3Sec, 243);
      LL_MUL(timeIncr, usecPerSec, secIn4Min3Sec);
      LL_ADD(timeIncr, timeIncr, usecPerDay);
   }

   /* Process command line arguments. */
   switch( argc )
   {
   case 1:
      break;

   case 2:
      if( strcmp(argv[1], "-v") == 0 )
         verbose = 1;
      else
         format = argv[1];
      break;

   case 3:
      if( strcmp(argv[1], "-v") == 0 )
         verbose = 1;
      format = argv[2];
      break;

   default:
      fprintf(stderr,"Usage: usdate [-v] [<format string>]\n");
      return 1;
   }

   /* Listen to the environment variables for locale info */
   setlocale(LC_ALL,"");
   
   /* Create a start date. */
   expandedTime.tm_isdst = 1;
   expandedTime.tm_year = 1993;
   expandedTime.tm_mon = 6;
   expandedTime.tm_mday = 30;
   expandedTime.tm_hour = 11;
   expandedTime.tm_min  = 30;
   expandedTime.tm_sec = 0;
   expandedTime.tm_usec = 0;
   time = PR_ComputeTime( &expandedTime );

   
   for( i=0; i<370; i++ )
   {
      PR_ExplodeTime( &expandedTime, time );
      
      oldReturn = PR_FormatTime(oldTimeBuffer,timeBufferSize,format,&expandedTime);
      newReturn = PR_FormatTimeUSEnglish( newTimeBuffer, timeBufferSize,
                                          format, &expandedTime );

      if( (strcmp(oldTimeBuffer, newTimeBuffer) != 0) || (oldReturn != newReturn) )
      {
         if( verbose )
         {
            printf("Old time is: \"%s\" with length %d\n", oldTimeBuffer, oldReturn );
            printf("New time is: \"%s\" with length %d\n", newTimeBuffer, newReturn );
         }
         errorCount++;
      }
      else
      {
         if( verbose )
            printf("Formatted time is: %s\n", newTimeBuffer );
      }

      /* Increment time. */
      LL_ADD(time, time, timeIncr);
   }

   printf("The number of errors is %d\n",errorCount);
   return errorCount;
}




