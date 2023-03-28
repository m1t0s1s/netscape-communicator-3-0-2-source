/*
	This is a disgusting hack.  Because the Mac file system limits file names to 31
	characters and the java runtime expects a one-to-one correspondence between class names
	and file names, we need to do a mapping.  When all of the components can read from
	car (zip?) files, this will no longer be necessary.
	
	This code ***SHOULD NOT SHIP***
*/


#include "prmacos.h"

#include <string.h>


char	*gFileMapping[][2] = {	//								 1234567890123456789012345678901
				{"ArrayIndexOutOfBoundsException.class", 		"ArrayIndexOutOfBoundsExce.class"},
				{"AssignUnsignedShiftRightExpression.class", 	"AssignUnsignedShiftRightE.class"},
				{"IllegalThreadStateException.class", 			"IllegalThreadStateExcepti.class"},
				{"IncompatibleClassChangeError.class", 			"IncompatibleClassChangeEr.class"},
				{"InvalidAudioFormatException.class", 			"InvalidAudioFormatExcepti.class"},
				{"StringExpressionConstantData.class", 			"StringExpressionConstantD.class"},
				{"StringIndexOutOfBoundsException.class", 		"StringIndexOutOfBoundsExe.class"},
				{"UnauthorizedHttpRequestException.class", 		"UnauthorizedHttpRequestEx.class"},
				{"UnsignedShiftRightExpression.class", 			"UnsignedShiftRightExpress.class"}
				};
const int gNumFileMappings = 9;



char *MapPartialToFullMacFile(const char *partialFileName)
{
	int 	i;
	
	
	if (strlen(partialFileName) == MAX_MAC_FILENAME) {
		for (i = 0; i < gNumFileMappings; i++) {
			if (strcmp(partialFileName, gFileMapping[i][1]) == 0)
				return (strdup(gFileMapping[i][0]));
		}
	}
	return NULL;
}


void MapFullToPartialMacFile(char *fullFileName)
{
	int		i;
	
	if (strlen(fullFileName) > MAX_MAC_FILENAME) {
		for (i = 0; i < gNumFileMappings; i++) {
			if (strcmp(fullFileName, gFileMapping[i][0]) == 0) {
				strcpy(fullFileName, gFileMapping[i][1]);
				break;
			}
		}
	}
}