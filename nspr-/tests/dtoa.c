/******************************************************************************
 *
 * This file contains a test program for the function conversion functions
 * for double precision code:
 * PR_strtod
 * PR_dtoa
 * PR_cnvtf
 *
 *****************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <locale.h>
#include "prprf.h"
#include "prdtoa.h"


int main( int argc, char* argv[] )
{
    double num;
    double num1;
    char   cnvt[50];
    
    num = 1e24;
    num1 = PR_strtod("1e24",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n","1e24");
    }

    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("1e24",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }

    num = 0.001e7;
    num1 = PR_strtod("0.001e7",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n","0.001e7");
    }
    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("10000",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }

    num = 0.0000000000000753;
    num1 = PR_strtod("0.0000000000000753",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n",
		"0.0000000000000753");
    }
    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("0.0000000000000753",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }

    num = 1.867e73;
    num1 = PR_strtod("1.867e73",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n","1.867e73");
    }
    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("1.867e73",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }


    num = -1.867e73;
    num1 = PR_strtod("-1.867e73",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n","-1.867e73");
    }
    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("-1.867e73",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }

    num = -1.867e-73;
    num1 = PR_strtod("-1.867e-73",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n","-1.867e-73");
    }

    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("-1.867e-73",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }

#ifndef SCO
    num = -1.867e765;
    num1 = PR_strtod("-1.867e765",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n","-1.867e765");
    }

    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("-Infinity",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }
#endif /* not SCO */

    num = 1.0000000001e21;
    num1 = PR_strtod("1.0000000001e21",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n",
		"1.0000000001e21");
    }

    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("1.0000000001e21",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }


    num = -1.0000000001e-21;
    num1 = PR_strtod("-1.0000000001e-21",NULL);
    if(num1 != num){
	fprintf(stderr,"Failed to convert numeric value %s\n",
		"-1.0000000001e-21");
    }
    PR_cnvtf(cnvt,sizeof(cnvt),20,num);
    if(strcmp("-1.0000000001e-21",cnvt) != 0){
	fprintf(stderr,"Failed to convert numeric value %lf %s\n",num,cnvt);
    }
    return 0;
}
