#include "xp.h"


Bool
lo_is_location_in_poly(int32 x, int32 y, int32 *coords, int32 coord_cnt)
{
    int32 totalv = 0;
    int32 totalc = 0;
    int32 intersects = 0;
    int32 wherex = x;
    int32 wherey = y;
    int32 xval, yval;
    int32 *pointer;
    int32 *end;

    totalv = coord_cnt / 2;
    totalc = totalv * 2;

    xval = coords[totalc - 2];
    yval = coords[totalc - 1];
    end = &coords[totalc];

    pointer = coords + 1;

    if ((yval >= wherey) != (*pointer >= wherey)) 
        if ((xval >= wherex) == (*coords >= wherex)) 
	    intersects += (xval >= wherex);
        else 
            intersects += (xval - (yval - wherey) * 
                          (*coords - xval) /
                          (*pointer - yval)) >= wherex;

    while(pointer < end)  {
        yval = *pointer;
        pointer += 2;

        if(yval >= wherey)  {
	   while((pointer < end) && (*pointer >= wherey))
		pointer+=2;
	   if (pointer >= end)
		break;
           if( (*(pointer-3) >= wherex) == (*(pointer-1) >= wherex) )
	        intersects += (*(pointer-3) >= wherex);
 	    else
		intersects += (*(pointer-3) - (*(pointer-2) - wherey) *
                              (*(pointer-1) - *(pointer-3)) /
                              (*pointer - *(pointer - 2))) >= wherex;

        }  else  {
	   while((pointer < end) && (*pointer < wherey))
		pointer+=2;
	   if (pointer >= end)
		break;
           if( (*(pointer-3) >= wherex) == (*(pointer-1) >= wherex) )
		intersects += (*(pointer-3) >= wherex);
 	   else
	        intersects += (*(pointer-3) - (*(pointer-2) - wherey) *
                              (*(pointer-1) - *(pointer-3)) /
                              (*pointer - *(pointer - 2))) >= wherex;
        }  
    }
    if (intersects & 1)
        return (TRUE);
    else
        return (FALSE);
}

