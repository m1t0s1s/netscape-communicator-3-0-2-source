/*
 * @(#)exception.c	1.27 96/03/28  
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*-
 *      Implementation of class Throwable
 *
 *      former classruntime.c, Wed Jun 26 18:43:20 1991
 */

#include <stdio.h>
#include <signal.h>

#include "tree.h"
#include "javaString.h"

#include "monitor.h"
#include "javaThreads.h"
#include "exceptions.h"

#include "java_lang_Throwable.h"
#include "java_lang_Thread.h"
#include "java_io_PrintStream.h"

#if defined(min)
#undef min
#endif
#define min(x, y) (((int32_t)(x) < (int32_t)(y)) ? (x) : (y))

extern HArrayOfByte *MakeByteString(char *str, long len);

static int
getStackDepth(JavaFrame* start_frame)
{
    JavaFrame frame_buf;
    JavaFrame* frame;
    int size;
    for (size = 0, frame = start_frame; frame != NULL; ) {
	if (frame->current_method != NULL) {
	    size++;
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
        } else {
	    frame = frame->prev;
        }
    }
    return size;
}

void
printStackTrace(Hjava_lang_Throwable *o, StackPrinter printer,
		void* stream, size_t len)
{
    /* ignore len */
    struct HArrayOfInt *backtrace = (HArrayOfInt *)(unhand(o)->backtrace);
    if (backtrace != NULL) {
	int32_t *real_data = unhand(backtrace)->body;
	unsigned char **data;
	unsigned char **enddata = 
	    (unsigned char **)(real_data + obj_length(backtrace));
	ExecEnv* ee = EE();
	int currentStackDepth = getStackDepth(ee->current_frame);
	int savedDepth = 0, currentLine = 0;
	for (data = (unsigned char**)real_data; data < enddata; data++) {
	    if (*data != 0)
		savedDepth++;
	}
	for (data = (unsigned char**)real_data; data < enddata; data++) { 
	    if (*data != 0) {
		#define LEN 128
		char where[LEN];
		int i = 0;
		
		if ((savedDepth - currentLine++) == currentStackDepth) {
		    strncpy(where + i, "* at ", 5); i += 5;
		} else {
		    strncpy(where + i, "  at ", 5); i += 5;
		}
		pc2string(*data, 0, where + i, where + sizeof(where));
		i = strlen(where);
		i = min(i, LEN-2);
		strncpy(where + i, "\n\0", 2); i += 1;	/* don't include the null */
			
		(void)printer(ee, stream, 0, where, i);
	    }
	}
    }
}

static int
printToPrintStream(void* env,
		   void* dest, size_t destLen,
		   const char* srcBuf, size_t srcLen)
{
    /* ignore context, destLen */
    ExecEnv* ee = (ExecEnv*)env;
    Hjava_io_PrintStream* s = (Hjava_io_PrintStream*)dest;
    HArrayOfByte* bytes = MakeByteString((char*)srcBuf, srcLen);
    execute_java_dynamic_method(ee, (HObject*)s,
				"write", "([B)V", bytes);
    return 0;
}

void
java_lang_Throwable_printStackTrace0(Hjava_lang_Throwable *o,
				     Hjava_io_PrintStream *s)
{
    printStackTrace(o, printToPrintStream, s, 0);
}

/* The pc's are saved as an int[] (to make it easy for the GC).  The
 * following constant indicates how may array elements are needed per pc
 * It will be 1 on 32-bit machines and 2 on 64-bit machines.
 */
#define MULTIPLIER (sizeof(char *)/sizeof(long))

void 
fillInStackTrace(Hjava_lang_Throwable *o, ExecEnv *ee) {
    int         size;
    Classjava_lang_Throwable *obj = unhand(o);
    JavaFrame    *start_frame = ee->current_frame;
    struct HArrayOfInt *backtrace = (HArrayOfInt *)obj->backtrace;
    JavaFrame   *frame;
    int32_t *real_data;
    unsigned char **data, **enddata;
    JavaFrame start_frame_buf, frame_buf; /* two buffers to avoid aliasing conflicts */
    
    /* Go backwards, removing all frames that are just part of the <init> of
     * the throwable object "o"
     */
    while (start_frame != NULL) {
    	if (start_frame->current_method != NULL) {
	    if (strcmp(fieldname(&start_frame->current_method->fb), "<init>") ||
		start_frame->vars[0].h != (JHandle *)o)
		break;

	    start_frame = start_frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(start_frame, &start_frame_buf) : start_frame->prev;
        } else {
	start_frame = start_frame->prev;
        }
    }
    /* how many useful frames are there on the stack? */
    size = getStackDepth(start_frame);

    /* if backtrace is null or not long enough, increase it */
    if ((backtrace == NULL) || obj_length(backtrace) < size * MULTIPLIER) {
	backtrace = (HArrayOfInt *)ArrayAlloc(T_INT, size * MULTIPLIER);
	if (backtrace == NULL) 
	    return;
	obj->backtrace = (HObject *)backtrace;
    }

    /* Fill in the backtrace array with the pc's */
    real_data = unhand(backtrace)->body;
    data = (unsigned char **)(real_data);
    enddata = (unsigned char **)(real_data + obj_length(backtrace));
    for (frame = start_frame; 
	     frame != NULL && data < enddata; ) { /* second test "just in case" */
	if (frame->current_method != NULL) {
	    struct methodblock *mb = frame->current_method;
	    if (mb->fb.access & ACC_MACHINE_COMPILED) {
		*data++ = CompiledCodePC(frame, mb);
                frame = CompiledFramePrev(frame, &frame_buf);
	    } else {
	        *data++ = frame->lastpc;
		frame = frame->prev;
            }
        } else {
	    frame = frame->prev;
	}
    } 
    /* Zero out any remaining space. */
    while (data < enddata) 
	*data++ = 0;
}


/*
 * Fill in the current stack trace in this exception.  This is
 * usually called automatically when the exception is created but it
 * may also be called explicitly by the user.  This routine returns
 * `this' so you can write 'throw e.fillInStackTrace();'
 */
Hjava_lang_Throwable *
java_lang_Throwable_fillInStackTrace(Hjava_lang_Throwable *obj) {
    ExecEnv *ee = (ExecEnv *)(THREAD(threadSelf())->eetop);

    fillInStackTrace(obj, ee);
    return obj;
}

