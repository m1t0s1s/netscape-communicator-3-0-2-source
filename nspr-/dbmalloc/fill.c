/*
 * (c) Copyright 1990, 1991, 1992 Conor P. Cahill (cpcahil@virtech.vti.com)
 *
 * This software may be distributed freely as long as the following conditions
 * are met:
 * 		* the distribution, or any derivative thereof, may not be
 *		  included as part of a commercial product
 *		* full source code is provided including this copyright
 *		* there is no charge for the software itself (there may be
 *		  a minimal charge for the copying or distribution effort)
 *		* this copyright notice is not modified or removed from any
 *		  source file
 */
#include <stdio.h>
#include "mallocin.h"

static long 	fillmalloc = -1;
static long 	fillfree   = -1;

#define FILLINIT() if( fillmalloc == -1 )  FillInit()


/*
 * Fill check types
 */
#define FILL_UNDER 	1
#define FILL_OVER	2
#define FILL_FREED	3

/*
 * FillInit() - fill initialization routine
 */
VOIDTYPE
FillInit()
{
	char	* ptr;
	int	  i;
	
	ptr = (char *) &fillmalloc;

	for( i=0; i < sizeof(long); i++)
	{
		*ptr++ = M_FILL;
	}

	ptr = (char *) &fillfree;

	for( i=0; i < sizeof(long); i++)
	{
		*ptr++ = M_FREE_FILL;
	}

} /* FillInit(... */

/*
 * FillCheck()	check for overflow and/or underflow using the filled regions
 *		in the specified malloc segment.
 */
int
FillCheck(func,file,line,ptr,showerrors)
	CONST char	* func;
	CONST char	* file;
	int		  line;
	struct mlist	* ptr;
	int		  showerrors;
{
	int		  rtn = -1;

	FILLINIT();

	/*
	 * if this block has no filling
	 */
	if( (ptr->flag & (M_FILLED|M_DFILLED)) == 0 )
	{
		rtn = 0;
	}
	/*
	 * else if this block is in use or it was not free-filled
	 */
	else if(    ((ptr->flag & M_INUSE)  != 0) 
		 || ((ptr->flag & M_FILLED) == 0) )
	{
		/*
		 * check for data underflow and data overflow
		 */
		rtn =   FillCheckData(func,file,line,ptr,FILL_UNDER,showerrors)
		      + FillCheckData(func,file,line,ptr,FILL_OVER, showerrors);
	}
	/*
	 * else the block must have been freed
	 */
	else
	{
		/*
		 * check for data underflow and free filling
		 */
		rtn =   FillCheckData(func,file,line,ptr,FILL_UNDER,showerrors)
		      + FillCheckData(func,file,line,ptr,FILL_FREED,showerrors);
	}

	return(rtn);

} /* FillCheck(... */

/*
 * FillCheckData() - routine to check the data areas to ensure that they
 * 		 are filled with the appropriate fill characters.
 */
int
FillCheckData(func,file,line,ptr,checktype,showerrors)
	CONST char	* func;
	CONST char	* file;
	int		  line;
	struct mlist	* ptr;
	int		  checktype;
	int		  showerrors;
{
	register char		* data = NULL;
	int			  errcode = 0;
	char			  filler = '\0';
	long			  fillword = 0;
	register SIZETYPE	  i = 0;
	register long		* ldata;
	SIZETYPE		  limit = ptr->s.size;
	int			  rtn = -1;

	if( (ptr->flag & (M_DFILLED | M_FILLED)) == 0 )
	{
		return(0);
	}

	switch(checktype)
	{
		case FILL_UNDER:
			if(    ((ptr->flag & M_DFILLED) == 0 )
			    || (ptr->s.filler[LONGFILL-1] == fillmalloc) )
			{
				rtn = 0;
			}
			else
			{

				/*
				 * if showing errors
			 	 */
				if( showerrors )
				{
					/*
					 * give the underrun error message
					 */
					malloc_errno = M_CODE_UNDERRUN;
					malloc_warning(func,file,line,ptr);
					/*
					 * fix the problem
					 */
					ptr->s.filler[LONGFILL-1] = fillmalloc;
				}
				rtn = 1;
			}
			break;

		case FILL_OVER:
			if( (ptr->flag & M_DFILLED) == 0)
			{
				rtn = 0;
			}
			else
			{
				i        = ptr->r_size;
				errcode  = M_CODE_OVERRUN;
				filler   = M_FILL;
				fillword = fillmalloc;
				data	 = ptr->data;
			}
			break;

		case FILL_FREED:
			if( (ptr->flag & M_FILLED) == 0)
			{
				rtn = 0;
			}
			else
			{
				i        = 0;
				errcode  = M_CODE_REUSE;
				filler   = M_FREE_FILL;
				fillword = fillfree;
				data	 = ptr->data;
			}
			break;

		default:
			i = 0; 
			limit = 0;
			break;
	}

	/*
	 * if there is nothing to check, just return 
	 */
	if( rtn != -1 )
	{
		return(rtn);
	}
			
	rtn = 0;
	data += i;
	i = limit - i;
	while( ((long)data) & (sizeof(long)-1) )
	{
		if( *data != filler )
		{
			if( showerrors )
			{
				malloc_errno = errcode;
				malloc_warning(func,file,line,ptr);

				/*
				 * fix the underrun so we only get this
				 * message once
				 */
				while( i-- > 0 )
				{
					*(data++) = filler;
				}
			}

			rtn++;
					
			break;
		}
		data++;
		i--;
	}

	/*
	 * if we haven't found an error yet
	 */
	if( rtn == 0 )
	{
		/*
		 * convert to use longword pointer
		 */
		ldata = (long *) data;
		i >>= 2;

		/*
		 * while more longwords to check
		 */	
		while( i > 0 )
		{
			if( *ldata != fillword )
			{
				if( showerrors )
				{
					malloc_errno = errcode;
					malloc_warning(func,file,line,ptr);
	
					/*
					 * fix the underrun so we only get this
					 * message once
					 */
					while( i-- > 0 )
					{
						*(ldata++) = fillword;
					}
				}
	
				rtn++;
				break;
			}

			ldata++;
			i--;
		
		}
		
	}

	return(rtn);

} /* FillCheckData(... */

/*
 * FillData()	fill in the needed areas in the specified segment.  This is used
 *		to place non-zero data in new malloc segments, setup boundary
 *		areas to catch under/overflow, and overwrite freed segments so 
 *		that they can't be usefully re-used.
 */
void
FillData(ptr,alloctype,start,nextptr)
	register struct mlist	* ptr;
	int			  alloctype;
	SIZETYPE		  start;
	struct mlist		* nextptr;
{
	int		  fills;
	int		  filler = 0;
	SIZETYPE	  limit = ptr->s.size;
	
	FILLINIT();

	if( (malloc_opts & (MOPT_MFILL|MOPT_DFILL|MOPT_FFILL)) == 0)
	{
		ptr->flag &= ~(M_FILLED|M_DFILLED);
		return;
	}

	switch(alloctype)
	{
		case FILL_MALLOC:
			fills = MOPT_MFILL | MOPT_DFILL;
			filler = M_FILL;
			break;

		case FILL_FREE:
			fills = MOPT_FFILL;
			filler = M_FREE_FILL;
			break;

		case FILL_SPLIT:
			fills = MOPT_FFILL;
			filler = M_FREE_FILL;
			break;

		case FILL_JOIN:
			if( (ptr->flag&M_INUSE) != 0 )
			{
				if( ptr->flag & M_FILLED )
				{
					fills = MOPT_MFILL;
					filler = M_FILL;
				}
				else if(  ptr->flag & M_DFILLED )
				{
					fills = MOPT_MFILL;
					filler = M_FILL;
					start = ptr->r_size;;
				}
				else
				{
					fills = 0;
				}
			}
			else /* not in use */
			{
				/*
				 * if this segment has bee free-filled
				 */
				if( ptr->flag & M_FILLED )
				{
					fills = MOPT_MFILL;
					filler = M_FREE_FILL;

					/*
					 * if the next segment is already filled
					 */
					if( (nextptr->flag & M_FILLED) != 0 )
					{
						limit = start + M_SIZE;
					}
				}
				/*
				 * else if this segment has overflow filling
				 * enabled, fill the next segment since it is
				 * now overflow of this segment.
				 */
				else if( (ptr->flag & M_DFILLED) != 0 )
				{
					fills = MOPT_DFILL;
					filler = M_FILL;
					start = ptr->r_size;;
				}
				else
				{
					fills = 0;
				}
			}
			break;

		case FILL_REALLOC:
			if( ptr->flag & M_FILLED )
			{
				fills = MOPT_MFILL;
				filler = M_FILL;
			}
			else if( ptr->flag & M_DFILLED )
			{
				fills = MOPT_MFILL;
				filler = M_FILL;
				start = ptr->r_size;;
			}
			else
			{
				fills = 0;
			}
			break;

		case FILL_CALLOC:
			fills = MOPT_DFILL;
			break;

		default:
			fills = 0;
	}


	if( (fills & MOPT_DFILL) != 0 )
	{
		if( (malloc_opts & MOPT_DFILL) != 0 )
		{
			ptr->s.filler[LONGFILL-1] = fillmalloc;
			ptr->flag |= M_DFILLED;
		}
		else if( alloctype != FILL_FREE ) 
		{
			ptr->flag &= ~M_DFILLED;
		}
	}

	if( (malloc_opts & (MOPT_MFILL|MOPT_FFILL) & fills) != 0 )
	{
		/*
		 * fill in the data
		 */
		DataMS(ptr->data+start, filler, (MEMSIZE) (limit - start));

		ptr->flag |= M_FILLED;
	}
	else 
	{
		ptr->flag &= ~M_FILLED;

		/*
		 * if we still have to fill the boundry area and this isn't 
		 * a free-fill, go do it
		 */
		if( ((ptr->flag & M_DFILLED) != 0) && (filler == M_FILL) )
		{
			DataMS(ptr->data+ptr->r_size, M_FILL, 
					(MEMSIZE) (limit - ptr->r_size));
		}

	}

} /* FillData(... */

