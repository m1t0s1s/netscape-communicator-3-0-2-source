/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*- */

#include "sec.h"
#include "net.h"
#include "secnet.h"
#include "sslimpl.h"
#include "fortezza.h"
#include "preenc.h"
#include "glhist.h"
#include "xp.h"
#include "ds.h"


#define CRYPT_BUF_SIZE 16384

extern int MK_OUT_OF_MEMORY;

typedef struct _preencStream {
    int fd;
    NET_StreamClass *stream;
    URL_Struct *url;
    int format_out;
    MWContext *window_id;
    PEContext *pe_cx;
    PEHeader header;
    unsigned char overflow[PE_CRYPT_INTRO_LEN];
    int32 overflow_len;
    unsigned char buf[CRYPT_BUF_SIZE];
    int32 buflen;
    int32 header_len;
    int32 mime_len;
    int32 data_len;
    int32 data_left;
    enum { header_intro, header_body, crypt_header_intro,
           crypt_header_body, crypt_data } state;
    PRBool save_enc;
} preencStream;

static int32
consume(char *dest, int32 want, const char **src, int32 *len)
{
    int32 have = MIN(want, *len);

    PORT_Memcpy(dest, *src, (unsigned int) have);
    
    *len -= have;
    *src += have;
    
    return have;
}

static int
process_header(preencStream *obj)
{
    SECStatus rv;
    PEHeader *header = NULL;
    
    PORT_Assert(obj->buflen == obj->header_len);
    
    if (GetInt2(obj->header.version) != PRE_VERSION) {
        /* XXX Set an error */
        return -1;
    }

    /* convert header to local card information if necessary */
    if (GetInt2(obj->header.type) == PRE_FORTEZZA_STREAM) {
        header = SSL_PreencryptedStreamToFile(obj->fd, &obj->header);
        if (header == NULL) {
            /* XXX set an error */
            return -1;
        }
    }

    /* if we are the cacher, we need to call internal type */
    if (obj->format_out & FO_CACHE_ONLY) {
        /* build stream with correct mime type */
        PORT_Strcpy(obj->url->content_type, INTERNAL_PRE_ENCRYPTED);
        obj->stream = NET_StreamBuilder(obj->format_out, obj->url,
                                        obj->window_id);
        if (!obj->stream) {
            return(MK_UNABLE_TO_CONVERT);
        }
        /* write header */
        rv = (*obj->stream->put_block)(obj->stream->data_object,
                                       (char *)&obj->header, 
                                       obj->header_len);
        obj->buflen = 0;
        obj->state = crypt_header_intro;
        
        /* just return, since we don't want to create context */
        return rv;
    }

    /* create context */
    obj->pe_cx = PE_CreateContext(&obj->header, obj->window_id);
    if (obj->pe_cx == NULL) {
        /* XXX set error */
        return -1;
    }

    /* prepare to start counting into crypted header */
    obj->buflen = 0;
    
    return 0;
}

static int
process_crypt_header(preencStream *obj)
{
    char *mime_type;
    
    obj->data_len = ((unsigned long)obj->buf[8] << 24) |
	((unsigned long)obj->buf[9] << 16) |
        (obj->buf[10] << 8) | obj->buf[11];
    obj->data_left = obj->data_len;
    
    mime_type = (char *)PORT_ZAlloc((unsigned int) obj->mime_len);
    PORT_Strcpy(mime_type, (char *)&obj->buf[PE_CRYPT_INTRO_LEN +
                                          sizeof(obj->data_len)]);
    
    obj->url->content_type = mime_type;
    
    /* build stream with correct mime type */
    obj->stream = NET_StreamBuilder(obj->format_out, obj->url,
                                    obj->window_id);
    if (!obj->stream) {
        return(MK_UNABLE_TO_CONVERT);
    }
    
    return 0;
}

static int
decrypt(preencStream *obj, const char **str, int32 *len)
{
    SECStatus rv;
    int32 bytes, overflow;
    unsigned int ret_len = 0;
    char *src;
    
    /* fill up overflow buf and decrypt */
    if (obj->overflow_len > 0) {
        PORT_Assert(obj->overflow_len < PRE_BLOCK_SIZE);
        
        obj->overflow_len += consume((char *)&obj->overflow[obj->overflow_len],
                                     PRE_BLOCK_SIZE - obj->overflow_len,
                                     str, len);
        if (obj->overflow_len < PRE_BLOCK_SIZE) return 0; 
        
        PORT_Assert(obj->overflow_len == PRE_BLOCK_SIZE);
        
        /* decrypt into obj->buf at correct spot */
        rv = PE_Decrypt(obj->pe_cx, &obj->buf[obj->buflen], &ret_len,
                        (unsigned int) ((int32)CRYPT_BUF_SIZE - obj->buflen), 
			 obj->overflow, (unsigned int) PRE_BLOCK_SIZE);
        if (rv == SECFailure) {
            /* XXX set an error */
            printf("error decrypting overflow\n");
            return -1;
        }
        obj->buflen += ret_len;
        ret_len = 0;
    }
    
    /* decrypt as much remaining data as possible */
    overflow = (CRYPT_BUF_SIZE - obj->buflen) % PRE_BLOCK_SIZE;
    bytes = CRYPT_BUF_SIZE - obj->buflen - overflow;
    
    if (*len < bytes) {
        overflow = *len % PRE_BLOCK_SIZE;
        bytes = *len - overflow;
    }
    
    if (bytes > 0) {
        src = (char *)*str;
        rv = PE_Decrypt(obj->pe_cx, &obj->buf[obj->buflen], &ret_len, 
		(unsigned int) bytes, (unsigned char *)src, 
						(unsigned int) bytes);
        if (rv == SECFailure) {
            /* XXX set an error */
            printf("error decrypting\n");
            return -1;
        }
    }
    
    *str += ret_len;
    *len -= ret_len;
    obj->buflen += ret_len;
    
    return 0;
}

static int
preenc_crypt_write(preencStream *obj, const char **str, int32 *len)
{
    int rv;
    int32 bytes = 0;
    
    rv = decrypt(obj, str, len);
    if (rv < 0) return rv;
    
    switch (obj->state) {
        
    case crypt_header_intro:
        
        if (obj->buflen < PE_CRYPT_INTRO_LEN) return 0;
        
        obj->header_len = (obj->buf[0] << 8) | obj->buf[1];
        obj->mime_len = (obj->buf[2] << 8) | obj->buf[3];
        if (((obj->buf[4] << 8) | obj->buf[5]) != PRE_VERSION) {
            /* set an error */
            return -1;
        }
        /* ignore the last two bytes in the header, which is padding */
        
        obj->state = crypt_header_body;
        /* drop through */
        
    case crypt_header_body:
        
        if (obj->buflen < obj->header_len) return 0;
        
        rv = process_crypt_header(obj);
        if (rv < 0) return rv;
        
        obj->state = crypt_data;
        /* drop through */
        
    case crypt_data:
        
        /* write data, excluding header if it is still in obj->buf */
        if (obj->buflen > obj->header_len) {
            /* only write out data, not file padding */
            bytes = MIN(obj->data_left, obj->buflen - obj->header_len);
            rv = (*obj->stream->put_block)(obj->stream->data_object,
                                           (char *)&obj->buf[obj->header_len],
                                           bytes);
            if (rv < 0) {
                printf("error writing data: %d\n", rv);
                return rv;
            }
        }
        obj->data_left -= bytes;
        obj->header_len = 0;
        obj->buflen = 0;
        /* drop through */
        
    default:
        /* this should never happen */
        break; /* make windows happy */
    }
    
    /* deal with overflow */
    if (*len < PRE_BLOCK_SIZE && *len != 0) {
        obj->overflow_len = consume((char *)obj->overflow, *len, str, len);
    }
    
    return 0;
}

static int
preenc_write(preencStream *obj, const char *str, int32 len)
{
    SECStatus rv;
    
    while (len > 0) {
        switch (obj->state) {
            
        case header_intro:
            
            /* just starting, get beginning of header to find out length */
            PORT_Assert(obj->buflen < PE_INTRO_LEN);
            
            obj->buflen += consume((char *)&obj->header + obj->buflen,
                                   PE_INTRO_LEN - obj->buflen, &str, &len);
            if (obj->buflen < PE_INTRO_LEN) return 0;
            
            if (GetInt2(obj->header.magic) != PRE_MAGIC) {
                /* XXX set an error */
                return -1;
            }
            
            obj->header_len = GetInt2(obj->header.len);
            obj->state = header_body;
            break;
            
        case header_body:
            
            PORT_Assert(obj->buflen < obj->header_len &&
                      obj->buflen >= PE_INTRO_LEN);
            
            /* read in rest of clear header */
            obj->buflen += consume((char *) &obj->header + obj->buflen,
                                   obj->header_len - obj->buflen, &str, &len);
            if (obj->buflen < obj->header_len) return 0;
            
            rv = process_header(obj);
            if (rv < 0) return rv;
            
            obj->buflen = 0;
            obj->state = crypt_header_intro;
            break;
            
        default:
            
            /* if we're the cacher, just write the data to internal type */
            if ((obj->format_out & FO_CACHE_ONLY) ||
		((CLEAR_CACHE_BIT(obj->format_out) == FO_SAVE_AS)
		 && obj->save_enc)) {
                rv = (*obj->stream->put_block)(obj->stream->data_object,
                                               str, len);
                if (rv < 0) return rv;
                /* we're done, so return */
                return 0;
            }
            /* if we're the internal type, decrypt and write */
            rv = preenc_crypt_write(obj, &str, &len);
            if (rv < 0) return rv;
            break;
            
        }
    }
    return 0;
}

static unsigned int
preenc_is_write_ready(void *data)
{
    preencStream *obj;
    
    obj = (preencStream *)data;
    
    if(obj->stream)
        return((*obj->stream->is_write_ready)(obj->stream->data_object));
    else
        return(MAX_WRITE_READY);
}

static void
preenc_complete(void *data)
{
    preencStream *obj;
    
    obj = (preencStream *)data;
    
    if(obj->stream) {
        (*obj->stream->complete)(obj->stream->data_object);
        PORT_Free(obj->stream);
    }
    
    if (obj->pe_cx) PE_DestroyContext(obj->pe_cx, PR_TRUE);
    PORT_Free(obj);
    
    return;
}

static void 
preenc_abort(void *data, int status)
{
    preencStream *obj;
    
    obj = (preencStream *)data;
    
    if(obj->stream) {
    	(*obj->stream->abort)(obj->stream->data_object, status);
        PORT_Free(obj->stream);
    }
    
    if (obj->pe_cx) PE_DestroyContext(obj->pe_cx, PR_TRUE);
    PORT_Free(obj);
    
    return;
}

/*
 * set the fd in our object to access SSLSocket if needed 
 */
void
SEC_SetPreencryptedSocket(void *data, int fd)
{
    preencStream *obj;
    
    obj = (preencStream *)data;
    
    if (obj != NULL) {
        obj->fd = fd;
    }
    return;
}

/*
 * set up a stream for reading in preencrypted files
 */
NET_StreamClass * 
SEC_MakePreencryptedStream(FO_Present_Types format_out, void *data,
                           URL_Struct *url, MWContext *window_id)
{
    preencStream *obj;
    NET_StreamClass *stream;
    
    GH_UpdateGlobalHistory(url);
    
    /* set to disable view source */
    url->is_active = TRUE;
    
    stream = (NET_StreamClass *)PORT_ZAlloc(sizeof(NET_StreamClass));
    if (stream == NULL) 
        return(NULL);
    
    obj = (preencStream *)PORT_ZAlloc(sizeof(preencStream));
    if (obj == NULL) {
        PORT_Free(stream);
        return(NULL);
    }
    
    PORT_Memset(obj, 0, sizeof(preencStream));
    
    stream->name = "preencrypted stream";
    stream->complete = preenc_complete;
    stream->abort = preenc_abort;
    stream->is_write_ready = preenc_is_write_ready;
    stream->data_object = obj;
    stream->window_id = window_id;
    stream->put_block = (MKStreamWriteFunc)preenc_write;
    
    /*
     * enable clicking since it doesn't go through the cache code
     */
    FE_EnableClicking(window_id);
    
    obj->stream = NULL;
    obj->window_id = window_id;
    obj->url = url;
    obj->fd = -1;
    obj->format_out = format_out;
    obj->pe_cx = NULL;
    obj->buflen = 0;
    obj->overflow_len = 0;
    obj->state = header_intro;

    if (!FE_Confirm( window_id, 
	"Save this file encrypted [ok] or unencrypted [cancel]")) {
	obj->save_enc = PR_FALSE;
    } else if (CLEAR_CACHE_BIT(format_out) == FO_SAVE_AS) {
	obj->save_enc = PR_TRUE;
        PORT_Strcpy(obj->url->content_type, "internal/always-save");
        obj->stream = NET_StreamBuilder(obj->format_out, obj->url,
                                        obj->window_id);
        if (!obj->stream) {
            return(MK_UNABLE_TO_CONVERT);
        }
    }
    
    return stream;
}
