/*
	single filename1 filename2 
*/
#include <stdio.h>
#include <stdlib.h>

void GenMap(char* name1, char* name2, 
	unsigned short array1[], unsigned short array2[])
{
	int i,j,found;

	printf("/*     Translation %s -> %s   */\n",name1, name2);
	ReportUnmap(array1,array2);
	printf("/*        x0x1 x2x3 x4x5 x6x7 x8x9 xAxB xCxD xExF   */\n");
        for(i=0;i<256;i++)
        {
		if((i%16) == 0)
			printf("/*%Xx*/  $\"",i/16);
                for(found=0,j=0;j<256;j++)
                {
                        if(array1[i] == array2[j])
                        {
				printf("%02X",j);
                                found = 1;
                                break;
                        }
                }
                if(found == 0)
                {
			printf("%2X",i);
                }
		if((i%16) == 15)
			printf("\"\n");
		else if(i%2)
			printf(" ");
        }
}
