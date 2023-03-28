/* -*- Mode: C; tab-width: 8 -*-
   sunaudio.c --- streaming audio support for Suns
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 19-Aug-94.
 */

#include "mozilla.h"
#include "xfe.h"

#include <string.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <sys/file.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>

extern int audio_decode_filehdr();
extern int audio__setplayhdr();
extern int audio__flush();
extern int audio_cmp_hdr();
extern int audio_enc_to_str();
extern int audio_drain();

#define AUDIO_BUFFER_SIZE 2048

struct sunau_data
{
  int fd;
  char scratch [sizeof (Audio_hdr) * 2];
  char audio_buffer[AUDIO_BUFFER_SIZE];
  int  audio_buffer_in_use;
  Audio_hdr file_hdr;
  Audio_hdr dev_hdr;
  unsigned long bytes_read;
  Boolean reset_device_p;
};


static int
fe_sunau_write_method (void *closure, const char *str, int32 len)
{
  int write_size;
  struct sunau_data *sad = (struct sunau_data *) closure;

  if (sad->bytes_read < sizeof (sad->scratch))
    {
      /* We haven't finished reading the file header; maybe this time? */
      int rest = sizeof (sad->scratch) - sad->bytes_read;
      if (rest > len) rest = len;
      strncpy ((char *) str, sad->scratch, rest);
      sad->bytes_read += rest;
      str += rest;
      len -= rest;
      if (sad->bytes_read > sizeof (sad->scratch))
	{
	  abort ();
	}
      else if (sad->bytes_read == sizeof (sad->scratch))
	{
	  unsigned int len;
  	  int setval;
	  if (AUDIO_SUCCESS != audio_decode_filehdr (sad->scratch,
						     &sad->file_hdr,
						     &len))
	    {
	      fprintf (stderr, "%s: invalid audio data\n", fe_progname);
	      return -1;
	    }

	  sad->fd = open ("/dev/audio", (O_WRONLY | O_NDELAY));

	  if (AUDIO_SUCCESS != audio_get_play_config (sad->fd, &sad->dev_hdr))
	    {
	      fprintf (stderr, "%s: not a valid audio device", fe_progname);
	      return -1;
	    }
	  audio_flush_play (sad->fd);

	  if (0 != audio_cmp_hdr (&sad->dev_hdr, &sad->file_hdr))
	    {
	      Audio_hdr new_hdr;
	      new_hdr = sad->file_hdr;
	      sad->reset_device_p = True;
	      if (AUDIO_SUCCESS != audio_set_play_config (sad->fd, &new_hdr))
		{
		  char buf1 [100], buf2 [100];
		  audio_enc_to_str (&sad->file_hdr, buf1);
		  audio_enc_to_str (&new_hdr, buf2);
		  fprintf (stderr, "%s: audio: wanted %s, got %s",
			   fe_progname, buf1, buf2);
		  return -1;
		}
	    }

	  /* make file descriptor non blocking
	   */
	  setval = 1;
    	  ioctl(sad->fd, FIONBIO, &setval);

	}
    }

  write_size = write (sad->fd, str, len);

  if(write_size < len)
    {
      /* stick the rest on the audio buffer 
       */
      BlockAllocCopy(sad->audio_buffer, str+write_size, len-write_size);
      sad->audio_buffer_in_use = len-write_size;
    }

  return 1;
}


/* use up buffered data if there is any.
 * only return greater than zero when there is no
 * buffered data
 */
static unsigned int
fe_sunau_write_ready_method (void *closure)
{
  struct sunau_data *sad = (struct sunau_data *) closure;
  int write_size;

  if(sad->audio_buffer_in_use)
    {
      write_size = write (sad->fd, sad->audio_buffer, sad->audio_buffer_in_use);

      if(write_size < sad->audio_buffer_in_use)
        {
          /* move the buffer around
           */
          XP_MEMMOVE(sad->audio_buffer, 
			sad->audio_buffer+write_size, 
			   sad->audio_buffer_in_use-write_size);
          sad->audio_buffer_in_use = sad->audio_buffer_in_use-write_size;
        }
      else
        {
	  sad->audio_buffer_in_use = 0;
          return (AUDIO_BUFFER_SIZE);
        }
    }
  else
    {
      return (AUDIO_BUFFER_SIZE);
    }
}

static void
fe_sunau_complete_method (void *closure)
{
  struct sunau_data *sad = (struct sunau_data *) closure;
  if (sad->fd)
    {
      audio_drain (sad->fd, 1);
      if (sad->reset_device_p)
	audio_set_play_config (sad->fd, &sad->dev_hdr);
      close (sad->fd);
    }

  if(sad->audio_buffer)
      free(sad->audio_buffer);
  free (sad);
}

static void
fe_sunau_abort_method (void *closure, int status)
{
  struct sunau_data *sad = (struct sunau_data *) closure;
  if (sad->fd)
    {
      audio_flush_play (sad->fd);
      if (sad->reset_device_p)
	audio_set_play_config (sad->fd, &sad->dev_hdr);
      close (sad->fd);
    }
  free (sad);
}


NET_StreamClass *
fe_MakeSunAudioStream (int format_out, void *data_obj,
		       URL_Struct *url_struct, MWContext *context)
{
  struct sunau_data *sad = calloc (sizeof (struct sunau_data), 0);
  NET_StreamClass *stream = (NET_StreamClass *)
    calloc (sizeof (NET_StreamClass), 1);
  stream->name           = "SunAudio";
  stream->complete       = fe_sunau_complete_method;
  stream->abort          = fe_sunau_abort_method;
  stream->put_block      = fe_sunau_write_method;
  stream->is_write_ready = fe_sunau_write_ready_method;
  stream->data_object    = sad;
  stream->window_id      = context;
  return stream;
}
