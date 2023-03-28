
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
#include "tostring.h"

#define STACK_ALLOC	100 /* allocate 100 elements at a time */
#ifndef lint
static
char rcs_hdr[] = "$Id: stack.c,v 1.1 1996/06/18 03:29:27 warren Exp $";
#endif

#define OUTBUFSIZE	128
#define ERRSTR "I/O error durring stack dump"

#define WRITEOUT(fd,str,len)	if( write(fd,str,(WRTSIZE)(len)) != (len) ) \
				{ \
					VOIDCAST write(2,ERRSTR,\
						     (WRTSIZE)strlen(ERRSTR));\
					exit(120); \
				}

#define COPY(s,t,buf,len)	while( (*(s) = *((t)++) ) \
				       && ( (s) < ((buf)+(len)-5) ) ) { (s)++; }


/*
 * stack.c - this file contains info used to maintain the malloc stack
 *	     trees that the user may use to identify where malloc is called
 *	     from.  Ideally, we would like to be able to read the stack
 *	     ourselves, but that is very system dependent.
 *
 * 	The user interface to the stack routines will be as follows:
 *
 *	#define malloc_enter(a)	StackEnter(a,__FILE__,__LINE__)
 *	#define malloc_leave(a) StackLeave(a,__FILE__,__LINE__)
 *
 *	NOTE: These functions depend upon the fact that they are called in a
 *	symentric manner.  If you skip one of them, you will not have valid
 *	information maintained in the stack.
 *
 *	Rather than keep a segment of the stack in each malloc region, we will
 * 	maintain a stack tree here and the malloc segment will have a pointer 
 *	to the current leaf of the tree.  This should save considerably on the
 *	amount of space taken up by maintaining this tree.
 */

struct stack	* current;
struct stack 	  root_node;

/*
 * local function prototpyes
 */
struct stack	* StackNew __STDCARGS(( CONST char * func, CONST char * file,
					int line));
int		  StackMatch __STDCARGS((struct stack * this,CONST char * func,
					 CONST char * file, int line ));

/*
 * stk_enter() - called when one enters a new function.  This function adds the 
 * 		 specified function as a new entry (unless it is already in the
 * 		 list, in which case it just sets the current pointer).
 */

void
StackEnter( func, file, line )
	CONST char	* func;
	CONST char	* file;
	int		  line;
{
	int		  match;
	struct stack	* this;

	/*
	 * if there are no current entries yet
	 */
	if( current == NULL )
	{
		this = &root_node;
	}
	else
	{
		this = current;
	}

	/*
	 * if there are no entries below this func yet,
	 */
	if( this->below == NULL )
	{
		this->below = StackNew(func,file,line);
		this->below->above = this;
		current = this->below;
	}
	else
	{
		/*
		 * drop down to the next level and look around for the
		 * specified function.
		 */
		this = this->below;

		/*
		 * scan across this level looking for a match
		 */
		while(      ( ! (match=StackMatch(this,func,file,line)) )
			 && (this->beside != NULL) )
		{
			this = this->beside;
		}

		/*
		 * if we found the entry
		 */
		if( match )
		{
			current = this;
		}
		else
		{
			current = this->beside = StackNew(func,file,line);
			this->beside->above = this->above;
		}
	}

} /* StackEnter(... */

/*
 * StackNew() - allocate a new stack structure and fill in the default values.
 *		This is here as a function so that we can manage a list of
 *		entries and therefore don't have to call malloc too often.
 *
 * NOTE: this function does not link the current function to the tree.  That
 * must be done by the calling function.
 */
struct stack *
StackNew(func,file,line)
	CONST char		* func;
	CONST char		* file;
	int			  line;
{
	static SIZETYPE		  alloccnt;
	static SIZETYPE		  cnt;
	static struct stack	* data;
	struct mlist		* mptr;
	static struct stack	* this;
	static int		  call_counter;

	/*
	 * if it is time to allocate more entries
	 */
	if( cnt == alloccnt )
	{
		/*
		 * reset the counters
		 */
		cnt = 0;
		alloccnt = STACK_ALLOC;

		/*
		 * go allocate the data
		 */
		data = (struct stack *)malloc(
			(SIZETYPE)(alloccnt*sizeof(*data)) );

		/*
		 * if we failed to get the data, tell the user about it
		 */
		if( data == NULL )
		{
			malloc_errno = M_CODE_NOMORE_MEM;
			malloc_fatal("StackNew",file,line,(struct mlist *)NULL);
		}

		/*
		 * change the id information put in by malloc so that the 
		 * record appears as a stack record (and doesn't get confused
		 * with other memory leaks)
		 */
		mptr = (struct mlist *) (((char *)data) - M_SIZE);
		mptr->id = call_counter++;
		SETTYPE(mptr,M_T_STACK);
		
	}

	/*
	 * grab next element off of the list
	 */
	this = data + cnt;

	/*
	 * setup the new structure and attach it to the tree
	 */
	this->above = this->below = this->beside = NULL;
	this->func  = func;
	this->file  = file;
	this->line  = line;
	
	/*
	 * increment the count since we used yet another entry
	 */
	cnt++;

	/*
	 * return the pointer to the new entry
	 */
	return(this);

} /* StackNew(... */

/*
 * StackMatch() - determine if the specified stack entry matches the specified
 * 		  set of func,file, and line.  We have to compare all three in
 *		  order to ensure that we get the correct function even if
 *		  there are two functions with the same name (or the caller
 *		  specified the wrong name on the arguement list)
 */
int
StackMatch(this,func,file,line)
	struct stack	* this;
	CONST char	* func;
	CONST char	* file;
	int		  line;
{

	return(    (strcmp(this->func,func) == 0)
		&& (strcmp(this->file,file) == 0)
		&& (this->line == line ) 	 ) ;

} /* StackMatch(... */
	

/*
 * StackLeave() - leave the current stack level (called at the end of a
 *		  function.
 */
void
StackLeave( func, file, line )
	CONST char	* func;
	CONST char	* file;
	int		  line;
{
	if( current == NULL )
	{
		malloc_errno = M_CODE_STK_NOCUR;
		malloc_fatal("stk_leave", file, line, (struct mlist *)NULL);
	}
	else if( strcmp(func,current->func) != 0 )
	{
		malloc_errno = M_CODE_STK_BADFUNC;
		malloc_fatal("stk_leave", file, line, (struct mlist *)NULL);
	}
	else
	{
		current = current->above;
	}
	
} /* StackLeave(... */

/*
 * StackCurrent() - get the current stack pointer
 */
struct stack *
StackCurrent()
{

	return( current );

} /* StackCurrent(... */

/*
 * StackDump() - dump the stack from the specified node 
 */
void
StackDump(fd, msg, node )
	int		  fd;
	CONST char	* msg;
	struct stack	* node;
{
	char		  outbuf[OUTBUFSIZE];
	char		* s;
	CONST char	* t;

	/*
	 * if there is nothing to show, just return
	 */
	if( (node == NULL) || (node == &root_node) )
	{
		return;
	}

	/*
	 * if caller specified a message to print out, print it out
	 */
	if( msg )
	{
		WRITEOUT(fd,msg,strlen(msg));
	}

	/*
	 * Ok, we have the info, so lets print it out
	 */
	do
	{
		WRITEOUT(fd,"         -> ",12);

		s = outbuf;	

		/*
		 * perform some simple sanity checking on the node pointer
		 */
		if(    (((DATATYPE *)node) < malloc_data_start)
		    || (((DATATYPE *)node) > malloc_data_end)
		    || ((((long)node) & 0x1) != 0) )
		{
			WRITEOUT(fd,"INVALID/BROKEN STACK CHAIN!!!\n",30);
			break;
		}
		

		/*
		 * build the string for this level
		 */
		s = outbuf;	
		t = node->func;
		COPY(s,t,outbuf,OUTBUFSIZE);
		t = "() in ";
		COPY(s,t,outbuf,OUTBUFSIZE);
		t = node->file;
		COPY(s,t,outbuf,OUTBUFSIZE);
		*s++ = '(';
		s += tostring(s,(ULONG) node->line,0,10,' ');
		*s++ = ')';
		*s++ = '\n';

		/*
		 * write out the string
		 */
		WRITEOUT(fd,outbuf,s-outbuf);

		/*
	 	 * move up one level in the stack
		 */
		node = node->above;

		/*
		 * until we get to the top of the tree
		 */
	} while( (node != NULL) && (node != &root_node) );

} /* StackDump(... */



