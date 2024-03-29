/*
 * @(#)jpegdecoder.c	1.8 95/11/27 Jim Graham
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

/*
 * This file was based upon the example.c stub file included in the
 * release 6 of the Independent JPEG Group's free JPEG software.
 */

/* First, if system header files define "boolean" map it to "system_boolean" */
#define boolean system_boolean

#include <stdio.h>
#include <setjmp.h>
#if XP_MAC
#include "s_awt_image_JPEGImageDecoder.h"
#include "jdk_java_io_InputStream.h"
#else
#include "sun_awt_image_JPEGImageDecoder.h"
#include "java_io_InputStream.h"
#endif

#include "jinclude.h"
#include "jpeglib.h"

/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct sun_jpeg_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct sun_jpeg_error_mgr * sun_jpeg_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF void
sun_jpeg_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a sun_jpeg_error_mgr struct */
  sun_jpeg_error_ptr myerr = (sun_jpeg_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  /* (*cinfo->err->output_message) (cinfo); */
  /* For Java, we will format the message and put it in the error we throw. */

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

/*
 * INPUT HANDLING:
 *
 * The JPEG library's input management is defined by the jpeg_source_mgr
 * structure which contains two fields to convey the information in the
 * buffer and 5 methods which perform all buffer management.  The library
 * defines a standard input manager that uses stdio for obtaining compressed
 * jpeg data, but here we need to use Java to get our data.
 *
 * We need to make the Java class information accessible to the source_mgr
 * input routines.  We also need to store a pointer to the start of the
 * Java array being used as an input buffer so that it is not moved or
 * garbage collected while the JPEG library is using it.  To store these
 * things, we make a private extension of the standard JPEG jpeg_source_mgr
 * object.
 *
 * Here's the extended source manager struct:
 */

struct sun_jpeg_source_mgr {
  struct jpeg_source_mgr pub;	/* "public" fields */

  HArrayOfByte *hInputBuffer;
  Hjava_io_InputStream *hInputStream;
  int suspendable;
  long remaining_skip;
  /* Always have a pointer to the array on the C stack to prevent compaction */
  JOCTET *array_head;
};

typedef struct sun_jpeg_source_mgr * sun_jpeg_source_ptr;

/*
 * Initialize source.  This is called by jpeg_read_header() before any
 * data is actually read.  Unlike init_destination(), it may leave
 * bytes_in_buffer set to 0 (in which case a fill_input_buffer() call
 * will occur immediately).
 */

GLOBAL void
sun_jpeg_init_source(j_decompress_ptr cinfo)
{
    sun_jpeg_source_ptr src = (sun_jpeg_source_ptr) cinfo->src;

    src->array_head = (JOCTET *) unhand(src->hInputBuffer)->body;
    src->pub.next_input_byte = src->array_head;
    src->pub.bytes_in_buffer = 0;
}

/*
 * This is called whenever bytes_in_buffer has reached zero and more
 * data is wanted.  In typical applications, it should read fresh data
 * into the buffer (ignoring the current state of next_input_byte and
 * bytes_in_buffer), reset the pointer & count to the start of the
 * buffer, and return TRUE indicating that the buffer has been reloaded.
 * It is not necessary to fill the buffer entirely, only to obtain at
 * least one more byte.  bytes_in_buffer MUST be set to a positive value
 * if TRUE is returned.  A FALSE return should only be used when I/O
 * suspension is desired (this mode is discussed in the next section).
 */
/*
 * Note that with I/O suspension turned on, this procedure should not
 * do any work since the JPEG library has a very simple backtracking
 * mechanism which relies on the fact that the buffer will be filled
 * only when it has backed out to the top application level.  When
 * suspendable is turned on, the sun_jpeg_fill_suspended_buffer will
 * do the actual work of filling the buffer.
 */

GLOBAL boolean
sun_jpeg_fill_input_buffer(j_decompress_ptr cinfo)
{
    sun_jpeg_source_ptr src = (sun_jpeg_source_ptr) cinfo->src;
    ExecEnv *ee;
    int ret, buflen;

    if (src->suspendable) {
	return FALSE;
    }
    if (src->remaining_skip) {
	src->pub.skip_input_data(cinfo, 0);
    }
    ee = EE();
    buflen = obj_length(src->hInputBuffer);
    ret = execute_java_dynamic_method(ee, (void *)src->hInputStream,
				      "read", "([BII)I",
				      src->hInputBuffer, 0, buflen);
    if (exceptionOccurred(ee)) {
	cinfo->err->error_exit((struct jpeg_common_struct *) cinfo);
    }
    if (ret <= 0) {
	/* Silently accept truncated JPEG files */
	src->array_head[0] = JPEG_EOI;
	ret = 1;
    }

    src->pub.next_input_byte = src->array_head;
    src->pub.bytes_in_buffer = ret;

    return TRUE;
}

/*
 * Note that with I/O suspension turned on, the JPEG library requires
 * that all buffer filling be done at the top application level.  Due
 * to the way that backtracking works, this procedure should save all
 * of the data that was left in the buffer when suspension occured and
 * only read new data at the end.
 */

GLOBAL void
sun_jpeg_fill_suspended_buffer(j_decompress_ptr cinfo)
{
    sun_jpeg_source_ptr src = (sun_jpeg_source_ptr) cinfo->src;
    ExecEnv *ee = EE();
    int ret, offset, buflen;

    ret = execute_java_dynamic_method(ee, (void *)src->hInputStream,
				      "available", "()I");
    if (ret <= src->remaining_skip) {
	return;
    }
    if (src->remaining_skip) {
	src->pub.skip_input_data(cinfo, 0);
    }
    /* Save the data currently in the buffer */
    offset = src->pub.bytes_in_buffer;
    if (src->pub.next_input_byte > src->array_head) {
	memcpy(src->array_head, src->pub.next_input_byte, offset);
    }
    buflen = obj_length(src->hInputBuffer) - offset;
    if (buflen <= 0) {
	return;
    }
    ret = execute_java_dynamic_method(ee, (void *)src->hInputStream,
				      "read", "([BII)I",
				      src->hInputBuffer, offset, buflen);
    if (exceptionOccurred(ee)) {
	cinfo->err->error_exit((struct jpeg_common_struct *) cinfo);
    }
    if (ret <= 0) {
	/* Silently accept truncated JPEG files */
	src->array_head[offset] = JPEG_EOI;
	ret = 1;
    }

    src->pub.next_input_byte = src->array_head;
    src->pub.bytes_in_buffer = ret + offset;

    return;
}

/*
 * Skip num_bytes worth of data.  The buffer pointer and count should
 * be advanced over num_bytes input bytes, refilling the buffer as
 * needed.  This is used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).  In some applications
 * it may be possible to optimize away the reading of the skipped data,
 * but it's not clear that being smart is worth much trouble; large
 * skips are uncommon.  bytes_in_buffer may be zero on return.
 * A zero or negative skip count should be treated as a no-op.
 */
/*
 * Note that with I/O suspension turned on, this procedure should not
 * do any I/O since the JPEG library has a very simple backtracking
 * mechanism which relies on the fact that the buffer will be filled
 * only when it has backed out to the top application level.
 */

GLOBAL void
sun_jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    sun_jpeg_source_ptr src = (sun_jpeg_source_ptr) cinfo->src;
    ExecEnv *ee;
    int ret;

    if (num_bytes < 0) {
	return;
    }
    num_bytes += src->remaining_skip;
    src->remaining_skip = 0;
    ret = src->pub.bytes_in_buffer;
    if (ret >= num_bytes) {
	src->pub.next_input_byte += num_bytes;
	src->pub.bytes_in_buffer -= num_bytes;
	return;
    }
    num_bytes -= ret;
    if (src->suspendable) {
	src->remaining_skip = num_bytes;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = src->array_head;
	return;
    }

    ee = EE();
    /* Note that the signature for the method indicates that it takes
     * and returns a long.  Casting the int num_bytes to a long on
     * the input should work well enough, and if we assume that the
     * return value for this particular method should always be less
     * than the argument value (or -1), then the return value coerced
     * to an int should return us the information we need...
     */
    ret = execute_java_dynamic_method(ee, (void *) src->hInputStream,
				      "skip", "(J)J",
				      int2ll(num_bytes));
    if (exceptionOccurred(ee)) {
	cinfo->err->error_exit((struct jpeg_common_struct *) cinfo);
    }
    if (ret < num_bytes) {
	/* Silently accept truncated JPEG files */
	src->array_head[0] = JPEG_EOI;
	src->pub.bytes_in_buffer = 1;
	src->pub.next_input_byte = src->array_head;
    } else {
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = src->array_head;
    }
}

/*
 * Terminate source --- called by jpeg_finish_decompress() after all
 * data has been read.  Often a no-op.
 */

GLOBAL void
sun_jpeg_term_source(j_decompress_ptr cinfo)
{
}


/*
 * Sample routine for JPEG decompression.  We assume that the source file name
 * is passed in.  We want to return 1 on success, 0 on error.
 */

void
sun_awt_image_JPEGImageDecoder_readImage(Hsun_awt_image_JPEGImageDecoder *this,
					 Hjava_io_InputStream *hInputStream,
					 HArrayOfByte *hInputBuffer)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct sun_jpeg_error_mgr jerr;
  struct sun_jpeg_source_mgr jsrc;
  /* More stuff */
  union pixptr {
      int32_t		*ip;
      unsigned char	*bp;
  } outbuf;
  HObject *hOutputBuffer;
  int ret;
  ExecEnv *ee = EE();
  unsigned char *bp;
  int32_t *ip, pixel;
  int grayscale;
  int buffered_mode;
  int final_pass;

  /* Step 0: verify the inputs. */

  if (hInputBuffer == 0 || hInputStream == 0) {
    SignalError(0, JAVAPKG "NullPointerException", 0);
    return;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = sun_jpeg_error_exit;
  /* Establish the setjmp return context for sun_jpeg_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    if (!exceptionOccurred(ee)) {
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo.err->format_message) ((struct jpeg_common_struct *) &cinfo,
				      buffer);
	SignalError(0, "sun/awt/image/ImageFormatException", buffer);
    }
    jpeg_destroy_decompress(&cinfo);
    return;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  cinfo.src = &jsrc.pub;
  jsrc.hInputStream = hInputStream;
  jsrc.hInputBuffer = hInputBuffer;
  jsrc.suspendable = FALSE;
  jsrc.remaining_skip = 0;
  jsrc.pub.init_source = sun_jpeg_init_source;
  jsrc.pub.fill_input_buffer = sun_jpeg_fill_input_buffer;
  jsrc.pub.skip_input_data = sun_jpeg_skip_input_data;
  jsrc.pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  jsrc.pub.term_source = sun_jpeg_term_source;

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* select buffered-image mode if it is a progressive JPEG only */
  buffered_mode = cinfo.buffered_image = jpeg_has_multiple_scans(&cinfo);
  grayscale = (cinfo.out_color_space == JCS_GRAYSCALE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *					(nor with the Java input source)
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */
  ret = execute_java_dynamic_method(ee, (void *) this,
				    "sendHeaderInfo", "(IIZZ)Z",
				    cinfo.image_width,
				    cinfo.image_height,
				    grayscale, buffered_mode);
  if (exceptionOccurred(ee) || !ret) {
    /* No more interest in this image... */
    jpeg_destroy_decompress(&cinfo);
    return;
  }
  /* Make a one-row-high sample array with enough room to expand to ints */
  if (grayscale) {
      hOutputBuffer = ArrayAlloc(T_BYTE, cinfo.image_width);
  } else {
      hOutputBuffer = ArrayAlloc(T_INT, cinfo.image_width);
  }
  if (hOutputBuffer == 0) {
    SignalError(0, JAVAPKG "OutOfMemoryError", 0);
    jpeg_destroy_decompress(&cinfo);
    return;
  }

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */
  /* For the first pass for Java, we want to deal with RGB for simplicity */
  /* Unfortunately, the JPEG code does not automatically convert Grayscale */
  /* to RGB, so we have to deal with Grayscale explicitly. */
  /* Annoyingly, we still have to expand the RGB data to 4 byte ARGB below. */
  if (!grayscale) {
      cinfo.out_color_space = JCS_RGB;
  }

  /* Step 5: Start decompressor */

  jpeg_start_decompress(&cinfo);

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   * For Java, we have already made the buffer above
   */ 
  /*
   * For Java, we have already made the buffer above, we just have to
   * unhand it into a stack variable to keep the GC from moving it.
   */
  outbuf.ip = unhand((HArrayOfInt *) hOutputBuffer)->body;

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  if (buffered_mode) {
      final_pass = FALSE;
      cinfo.dct_method = JDCT_IFAST;
  } else {
      final_pass = TRUE;
  }
  do {
      if (buffered_mode) {
	  do {
	      sun_jpeg_fill_suspended_buffer(&cinfo);
	      jsrc.suspendable = TRUE;
	      ret = jpeg_consume_input(&cinfo);
	      jsrc.suspendable = FALSE;
	  } while (ret != JPEG_SUSPENDED && ret != JPEG_REACHED_EOI);
	  if (ret == JPEG_REACHED_EOI) {
	      final_pass = TRUE;
	      cinfo.dct_method = JDCT_ISLOW;
	  }
	  jpeg_start_output(&cinfo, cinfo.input_scan_number);
      }
      while (cinfo.output_scanline < cinfo.output_height) {
	  if (! final_pass) {
	      do {
		  sun_jpeg_fill_suspended_buffer(&cinfo);
		  jsrc.suspendable = TRUE;
		  ret = jpeg_consume_input(&cinfo);
		  jsrc.suspendable = FALSE;
	      } while (ret != JPEG_SUSPENDED && ret != JPEG_REACHED_EOI);
	      if (ret == JPEG_REACHED_EOI) {
		  break;
	      }
	  }
	  (void) jpeg_read_scanlines(&cinfo, (JSAMPARRAY) &outbuf, 1);
	  if (grayscale) {
	      ret = execute_java_dynamic_method(ee, (void *) this,
						"sendPixels", "([BI)Z",
						hOutputBuffer,
						cinfo.output_scanline - 1);
	  } else {
	      ip = outbuf.ip + cinfo.image_width;
	      bp = outbuf.bp + cinfo.image_width * 3;
	      while (ip > outbuf.ip) {
		  pixel = (*--bp);
		  pixel |= (*--bp) << 8;
		  pixel |= (*--bp) << 16;
		  *--ip = pixel;
	      }
	      ret = execute_java_dynamic_method(ee, (void *) this,
						"sendPixels", "([II)Z",
						hOutputBuffer,
						cinfo.output_scanline - 1);
	  }
	  if (exceptionOccurred(ee) || !ret) {
	      /* No more interest in this image... */
	      final_pass = TRUE;
	      break;
	  }
      }
      if (buffered_mode) {
	  jpeg_finish_output(&cinfo);
      }
  } while (! final_pass);

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   * (nor with the Java data source)
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  /* Not needed for Java - the Java code will close the file */
  /* fclose(infile); */

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return;
}


/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.doc for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.doc for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 */
