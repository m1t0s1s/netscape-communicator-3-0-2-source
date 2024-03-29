/* -*- Mode: C; tab-width: 4 -*- */

/* Please leave outside of ifdef for window precompiled headers */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT

/* Publicly released Netscape cache access routines.
 *
 * These routines are shared between the netscape executable
 * and the programs released as a cache developers kit.
 *
 * Copyright (R) 1995 Netscape Communications Corporation, all rights reserved.
 * Created: Lou Montulli <montulli@netscape.com>, July-95.
 */

#ifndef EXT_DB_ROUTINES
#include "cert.h"
#include "sechash.h"
#endif

#include "extcache.h"  /* include this for everything */

#ifdef EXT_DB_ROUTINES

#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

typedef struct {
	int32 len;
	char *data;
} SECItem;

#ifdef _sgi
#include <sys/endian.h>
#endif /* _sgi */


/* URL methods
 */
 #define URL_GET_METHOD  0
 #define URL_POST_METHOD 1
 #define URL_HEAD_METHOD 2

#endif /* DB_STORE */


MODULE_PRIVATE DBT *
net_CacheDBTDup(DBT *obj)
{
	DBT * rv = XP_NEW(DBT);

	if(!rv)
		return(NULL);

	rv->size = obj->size;
	rv->data = XP_ALLOC(rv->size);

	if(!rv->data)
	  {
		FREE(rv);
		return(NULL);
	  }

	XP_MEMCPY(rv->data, obj->data, rv->size);

	return(rv);

}

/* free the cache object
 */
MODULE_PRIVATE void net_freeCacheObj (net_CacheObject * cache_obj)
{

    FREEIF(cache_obj->address);
    FREEIF(cache_obj->post_data);
    FREEIF(cache_obj->post_headers);
    FREEIF(cache_obj->content_type);
    FREEIF(cache_obj->charset);
    FREEIF(cache_obj->content_encoding);
    FREEIF(cache_obj->filename);
	
#ifndef EXT_DB_ROUTINES
#ifdef HAVE_SECURITY /* added by jwz */
	if ( cache_obj->certificate ) {
		CERT_DestroyCertificate(cache_obj->certificate);
	}
#endif /* !HAVE_SECURITY -- added by jwz */
#endif
	
    FREE(cache_obj);
}

/* returns true if this DBT looks like a valid
 * entry.  It looks at the checksum and the
 * version number to see if it's valid
 */
#define MAX_VALID_DBT_SIZE 10000

MODULE_PRIVATE Bool
net_IsValidCacheDBT(DBT *obj)
{
    char *cur_ptr, *max_ptr;
    uint32 len;

	if(!obj || obj->size < 9 || obj->size > MAX_VALID_DBT_SIZE)
		return(FALSE);

    cur_ptr = (char *)obj->data;
    max_ptr = cur_ptr+obj->size;

    /* get the total size of the struct out of
     * the first field to check it
     */
    COPY_INT32(&len, cur_ptr);
    cur_ptr += sizeof(int32);

    if(len != obj->size)
      {
        TRACEMSG(("Size going in is not the same as size coming out"));
        return(FALSE);
      }

    /* get the version number of the written structure
     */
    if(cur_ptr > max_ptr)
        return(FALSE);
    COPY_INT32(&len, cur_ptr);
    cur_ptr += sizeof(int32);

    if(len != CACHE_FORMAT_VERSION)
      {
        TRACEMSG(("Version of cache structure is wrong!: %d", len));
        return(FALSE);
      }

	/* looks good to me... */
	return(TRUE);
}


/* takes a cache object and returns a malloc'd 
 * (void *) suitible for passing in as a database
 * data storage object
 */
MODULE_PRIVATE DBT *
net_CacheStructToDBData(net_CacheObject * old_obj)
{
    int32 len;
	char *cur_ptr;
	void *new_obj;
	int32 total_size;
	DBT *rv;

	rv = XP_NEW(DBT);

	if(!rv)
		return(NULL);

	total_size = sizeof(net_CacheObject);

#define ADD_STRING_SIZE(string) \
total_size += old_obj->string ? XP_STRLEN(old_obj->string)+1 : 0

	ADD_STRING_SIZE(address);
	total_size += old_obj->post_data_size+1;
	ADD_STRING_SIZE(post_headers);
	ADD_STRING_SIZE(content_type);
	ADD_STRING_SIZE(content_encoding);
	ADD_STRING_SIZE(charset);
	ADD_STRING_SIZE(filename);
	ADD_STRING_SIZE(key_cipher);
	total_size += sizeof(uint32); /* certificate length */

#ifndef EXT_DB_ROUTINES
	if ( old_obj->certificate ) {
		total_size += old_obj->certificate->derCert.len;
	}
#endif

#undef ADD_STRING_SIZE
	
	new_obj = XP_ALLOC(total_size * sizeof(char));

	if(!new_obj)
	  {
		FREE(rv);
		return NULL;
	  }

	XP_MEMSET(new_obj, 0, total_size * sizeof(char));

	/*
	 * order is:
	 * int32	size of the entire structure;
	 *
	 * int32    version of the structure format (CACHE_FORMAT_VERSION)
	 *
     * time_t                  last_modified;
     * time_t                  last_accessed;
     * time_t                  expires;
     * uint32                  content_length;
     * Bool                    is_netsite;
	 *
 	 * time_t                  lock_date;
	 *
     * char                  * filename;
     * int32                   filename_len;
	 *
     * int32                   security_on;
     * int32                   key_size;
     * int32                   key_secret_size;             
     * char                  * key_cipher;
	 *
     * struct SECCertificateStr *certificate;
	 *
     * int32                   method;
	 *
	 * # don't store address, or post_data stuff
	 * # char                  * address;
     * # uint32                  post_data_size;
     * # char                  * post_data;
	 *
     * char                  * post_headers;
     * char                  * content_type;
     * char                  * content_encoding;
     * char                  * charset;
	 *
	 * Bool                    incomplete_file;
	 * uint32                  total_content_length;
 	 * 
	 * string lengths all include null terminators
	 * all integer constants are stored as 4 bytes
	 * all booleans are stored as one byte
	 */

	/* VERY VERY IMPORTANT.  Whenever the
	 * format of the record structure changes
 	 * you must verify that the byte positions	
	 * in extcache.h are updated
	 */

#define STUFF_STRING(string) 									\
{	 															\
  len = (old_obj->string ? XP_STRLEN(old_obj->string)+1 : 0);	\
  COPY_INT32((void *)cur_ptr, &len);							\
  cur_ptr = cur_ptr + sizeof(int32);							\
  if(len)														\
      XP_MEMCPY((void *)cur_ptr, old_obj->string, len);			\
  cur_ptr += len;												\
}

#define STUFF_NUMBER(number) 									\
{ 																\
  COPY_INT32((void *)cur_ptr, &old_obj->number);				\
  cur_ptr = cur_ptr + sizeof(int32);							\
}

#define STUFF_TIMET(number) 									\
{ 																\
  COPY_INT32((void *)cur_ptr, &old_obj->number);				\
  cur_ptr = cur_ptr + sizeof(time_t);							\
}

#define STUFF_BOOL(bool_val)	 									\
{ 																	\
  if(old_obj->bool_val)												\
    ((char *)(cur_ptr))[0] = 1;										\
  else																\
    ((char *)(cur_ptr))[0] = 0;										\
  cur_ptr = cur_ptr + sizeof(char);									\
}

	cur_ptr = (char *)new_obj;

	/* put the total size of the struct into
	 * the first field so that we have
	 * a cross check against corruption
	 */
  	COPY_INT32((void *)cur_ptr, &total_size);
  	cur_ptr = cur_ptr + sizeof(int32);

	/* put the version number of the structure
	 * format that we are using
	 * By using a version string when writting
	 * we can support backwards compatibility
	 * in our reading code
	 * (use "len" as a temp variable)
	 */
	len = CACHE_FORMAT_VERSION;
  	COPY_INT32((void *)cur_ptr, &len);
  	cur_ptr = cur_ptr + sizeof(int32);

	STUFF_TIMET(last_modified);
	STUFF_TIMET(last_accessed);
	STUFF_TIMET(expires);
	STUFF_NUMBER(content_length);
	STUFF_BOOL(is_netsite);

	STUFF_TIMET(lock_date);

	STUFF_STRING(filename);
	STUFF_NUMBER(filename_len);

	STUFF_BOOL(is_relative_path);

	STUFF_NUMBER(security_on);
	STUFF_NUMBER(key_size);
	STUFF_NUMBER(key_secret_size);
	STUFF_STRING(key_cipher);

#ifndef EXT_DB_ROUTINES
    /* save the certificate */
    if ( old_obj->certificate ) {
		len = old_obj->certificate->derCert.len;
		COPY_INT32((void *)cur_ptr, &len);
		cur_ptr = cur_ptr + sizeof(int32);

		XP_MEMCPY((void *)cur_ptr, old_obj->certificate->derCert.data, len);
		cur_ptr += len;
	} else {
#else
    {
#endif
		len = 0;
		COPY_INT32((void *)cur_ptr, &len);
		cur_ptr = cur_ptr + sizeof(int32);
	}

	STUFF_NUMBER(method);

#ifdef STORE_ADDRESS_AND_POST_DATA

	STUFF_STRING(address);
	STUFF_NUMBER(post_data_size);
	
	/* post_data 
	 * this is special since it not necessarily a string
	 */
	if(old_obj->post_data_size)
	  {
	    XP_MEMCPY(cur_ptr, old_obj->post_data, old_obj->post_data_size+1);
	    cur_ptr += old_obj->post_data_size+1;
	  }

#endif /* STORE_ADDRESS_AND_POST_DATA */

	STUFF_STRING(post_headers);

	STUFF_STRING(content_type);
	STUFF_STRING(content_encoding);
	STUFF_STRING(charset);

	STUFF_BOOL(incomplete_file);
	STUFF_NUMBER(real_content_length);

#undef STUFF_STRING
#undef STUFF_NUMBER
#undef STUFF_BOOL

	rv->data = new_obj;
	rv->size = total_size;

	return(rv);

}

/* takes a database storage object and returns a malloc'd
 * cache data object.  The cache object needs all of
 * it's parts free'd.
 *
 * returns NULL on parse error 
 */
MODULE_PRIVATE net_CacheObject *
net_DBDataToCacheStruct(DBT * db_obj)
{
	net_CacheObject * rv = XP_NEW(net_CacheObject);
	char * cur_ptr;
	char * max_ptr;
	uint32 len;
	int32 version;
	SECItem seccert;

	if(!rv)
		return NULL;

	XP_MEMSET(rv, 0, sizeof(net_CacheObject));

/* if any strings are larger than this then
 * there was a serious database error
 */
#define MAX_HUGE_STRING_SIZE 10000

#define RETRIEVE_STRING(string)					\
{												\
    if(cur_ptr > max_ptr)                       \
	  {					 						\
		net_freeCacheObj(rv);				 	\
        return(NULL);                           \
	  }											\
	COPY_INT32(&len, cur_ptr);					\
	cur_ptr += sizeof(int32);					\
	if(len)										\
	  {											\
		if(len > MAX_HUGE_STRING_SIZE)			\
	      {				 						\
		    net_freeCacheObj(rv);				\
			return(NULL);						\
	      }										\
	    rv->string = (char*)XP_ALLOC(len);		\
		if(!rv->string)							\
	      {				 						\
		    net_freeCacheObj(rv);				\
			return(NULL);						\
	      }										\
	    XP_MEMCPY(rv->string, cur_ptr, len);	\
	    cur_ptr += len;							\
	  }											\
}											

#define RETRIEVE_NUMBER(number)					\
{												\
    if(cur_ptr > max_ptr)                       \
        return(rv);                             \
	COPY_INT32(&rv->number, cur_ptr);			\
	cur_ptr += sizeof(int32);					\
}

#define RETRIEVE_TIMET(number)					\
{												\
    if(cur_ptr > max_ptr)                       \
        return(rv);                             \
    COPY_INT32(&rv->number, cur_ptr);			\
	cur_ptr += sizeof(time_t);					\
}

#define RETRIEVE_BOOL(bool)				\
{										\
  if(cur_ptr > max_ptr)                 \
    return(rv);                         \
  if(((char *)(cur_ptr))[0])			\
	rv->bool = TRUE;					\
  else 									\
	rv->bool = FALSE;					\
  cur_ptr += sizeof(char);				\
}

	cur_ptr = (char *)db_obj->data;

	max_ptr = cur_ptr+db_obj->size;

	/* get the total size of the struct out of
	 * the first field to check it
	 */
	COPY_INT32(&len, cur_ptr);
	cur_ptr += sizeof(int32);

	if(len != db_obj->size)
	  {
		TRACEMSG(("Size going in is not the same as size coming out"));
		FREE(rv);
		return(NULL);
	  }

	/* get the version number of the written structure
	 */
    if(cur_ptr > max_ptr)
        return(rv);
	COPY_INT32(&version, cur_ptr);
	cur_ptr += sizeof(int32);

	if(version != CACHE_FORMAT_VERSION)
	  {
		TRACEMSG(("Version of cache structure is wrong!: %d", version));
		FREE(rv);
		return(NULL);
	  }

	RETRIEVE_TIMET(last_modified);
	RETRIEVE_TIMET(last_accessed);
	RETRIEVE_TIMET(expires);
	RETRIEVE_NUMBER(content_length);
	RETRIEVE_BOOL(is_netsite);

	RETRIEVE_TIMET(lock_date);

	RETRIEVE_STRING(filename);
	RETRIEVE_NUMBER(filename_len);

	RETRIEVE_BOOL(is_relative_path);

	RETRIEVE_NUMBER(security_on);
	RETRIEVE_NUMBER(key_size);
	RETRIEVE_NUMBER(key_secret_size);
	RETRIEVE_STRING(key_cipher);

    /* certificate length */
    if(cur_ptr > max_ptr)
        return(rv);
	COPY_INT32(&len, cur_ptr);
	cur_ptr += sizeof(int32);

    seccert.len = len;

    if ( len ) {
		/* copy certificate */
		if(cur_ptr > max_ptr)
			return(rv);

		seccert.data = (unsigned char *)XP_ALLOC(len);
		if ( !seccert.data )
			return(rv);

		XP_MEMCPY(seccert.data, cur_ptr, len);
		cur_ptr += len;

#ifndef EXT_DB_ROUTINES
#ifdef HAVE_SECURITY /* added by jwz */
		rv->certificate = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
												  &seccert, NULL, PR_FALSE,
												  PR_TRUE);
#endif /* !HAVE_SECURITY -- added by jwz */
		XP_FREE(seccert.data);
#endif
	}

	RETRIEVE_NUMBER(method);

#ifdef STORE_ADDRESS_AND_POST_DATA

	RETRIEVE_STRING(address);
	RETRIEVE_NUMBER(post_data_size);
	
	/* post_data 
	 * this is special since it not necessarily a string
	 */
	if(rv->post_data_size)
	  {
		rv->post_data = XP_ALLOC(rv->post_data_size+1);
		if(rv->post_data)
			XP_MEMCPY(rv->post_data, cur_ptr, rv->post_data_size+1);
		cur_ptr += rv->post_data_size+1;
	  }

#endif /* STORE_ADDRESS_AND_POST_DATA */

	RETRIEVE_STRING(post_headers);

	RETRIEVE_STRING(content_type);
	RETRIEVE_STRING(content_encoding);
	RETRIEVE_STRING(charset);

	RETRIEVE_BOOL(incomplete_file);
	RETRIEVE_NUMBER(real_content_length);

#undef RETRIEVE_STRING
#undef RETRIEVE_NUMBER
#undef RETRIEVE_BOOL

	return(rv);
}

#if defined(DEBUG) && defined(UNIX)
int
cache_test_me()
{

	net_CacheObject test;
	net_CacheObject *rv;
	int32 total_size;
	DBT *db_obj;

	XP_MEMSET(&test, 0, sizeof(net_CacheObject));
	StrAllocCopy(test.address, "test1");
	db_obj = net_CacheStructToDBData(&test);
	rv = net_DBDataToCacheStruct(db_obj);
	printf("test1: %s\n", rv->address);

	XP_MEMSET(&test, 0, sizeof(net_CacheObject));
	StrAllocCopy(test.address, "test2");
	StrAllocCopy(test.charset, "test2");
	db_obj = net_CacheStructToDBData(&test);
	rv = net_DBDataToCacheStruct(db_obj);
	printf("test2: %s	%s\n", rv->address, rv->charset);

	XP_MEMSET(&test, 0, sizeof(net_CacheObject));
	StrAllocCopy(test.address, "test3");
	StrAllocCopy(test.charset, "test3");
	StrAllocCopy(test.key_cipher, "test3");
	test.content_length = 3 ;
	test.method = 3 ;
	test.is_netsite = 3 ;
	db_obj = net_CacheStructToDBData(&test);
	rv = net_DBDataToCacheStruct(db_obj);
	printf("test3: %s	%s	%s	%d	%d	%s\n", 
			rv->address, rv->charset, rv->key_cipher,
			rv->content_length, rv->method, 
			(rv->is_netsite == 3 ? "TRUE" : "FALSE"));

}
#endif

/* generates a key for use in the cache database
 * from a CacheObject struct
 * 
 * Key is based on the address and the post_data
 *
 * looks like:
 *  size checksum | size of address | ADDRESS | size of post data | POST DATA
 */
MODULE_PRIVATE DBT *
net_GenCacheDBKey(char *address, char *post_data, int32 post_data_size)
{
	DBT *rv = XP_NEW(DBT);
	char *hash;
	char *data_ptr;
	int32 str_len;
	int32 size;

#define MD5_HASH_SIZE 16 /* always 16 due to md5 hash type */
	
	if(!rv)
		return(NULL);
	
	if(!address)
	  {
		XP_ASSERT(0);
		rv->size = 0;
		return(rv);
	  } 

	hash = XP_STRCHR(address, '#');

	/* don't include '#' in a key */
	if(hash)
		*hash = '\0';

	str_len = XP_STRLEN(address)+1;

	size  = sizeof(int32);  /* for check sum */
	size += sizeof(int32);  /* for size of address */
	size += str_len;        /* for address string */
	size += sizeof(int32);  /* for size of post_data */

	if(post_data_size)
		size += MD5_HASH_SIZE;

	rv->size = size;
	rv->data = XP_ALLOC(size);

	if(!rv->data)
	  {
		FREE(rv);
		return NULL;
	  }

	data_ptr = (char *) rv->data;

	/* put in the checksum size */
	COPY_INT32(data_ptr, &size);
	data_ptr = data_ptr + sizeof(int32);
	
	/* put in the size of the address string */
	COPY_INT32(data_ptr, &str_len);
	data_ptr = data_ptr + sizeof(int32);

	/* put in the address string data */
	XP_MEMCPY(data_ptr, address, str_len);
	data_ptr = data_ptr + str_len;

	/* set the address back to it's original form */
	if(hash)
		*hash = '#';

	/* put in the size of the post data */
	if(post_data_size)
	  {
		int32 size_of_md5 = MD5_HASH_SIZE;
		unsigned char post_data_hash[MD5_HASH_SIZE];

		MD5_HashBuf(post_data_hash, (unsigned char*)post_data, post_data_size);

		COPY_INT32(data_ptr, &size_of_md5);
		data_ptr = data_ptr + sizeof(int32);

		/* put in the post data if there is any */
		XP_MEMCPY(data_ptr, post_data_hash, sizeof(post_data_hash));
	  }
	else
	  {
		COPY_INT32(data_ptr, &post_data_size);
		data_ptr = data_ptr + sizeof(int32);
	  }

	return(rv);
}

/* returns a static string that contains the
 * URL->address of the key
 * 
 * returns NULL on error
 */
MODULE_PRIVATE char *
net_GetAddressFromCacheKey(DBT *key)
{
	uint32 size;
	char *data;

	/* check for minimum size */
	if(key->size < 10)
		return(NULL);

	/* validate size checksum */
	data = (char *)key->data;
	COPY_INT32(&size, data);
	data += sizeof(int32);

	if(size != key->size)
		return(NULL);

	/* get size of address string */
	COPY_INT32(&size, data);
	data += sizeof(int32);

	/* make sure it's a valid c string */
	if(data[size] != '\0')
		return(NULL);
		
	/* it's valid return it */
	return(data);
}


/* checks a date within a DBT struct so
 * that we don't have to convert it into a CacheObject
 *
 * This works because of the fixed length record format
 * of the first part of the specific DBT format I'm
 * using
 * 
 * returns 0 on error
 */
MODULE_PRIVATE time_t 
net_GetTimeInCacheDBT(DBT *data, int byte_position)
{
	time_t date;
	char *ptr = (char *)data->data;

	if(data->size < byte_position+sizeof(time_t))
		return(0);

	if(!net_IsValidCacheDBT(data))
		return(0);

	COPY_INT32(&date, ptr+byte_position);

	/* TRACEMSG(("Got date from cache DBT: %d", date)); */
	
	return(date);

}

/* Sets a date within a DBT struct so
 * that we don't have to convert it into a CacheObject
 *
 * This works because of the fixed length record format
 * of the first part of the specific DBT format I'm
 * using
 * 
 * returns 0 on error
 */
MODULE_PRIVATE void
net_SetTimeInCacheDBT(DBT *data, int byte_position, time_t date)
{
	char *ptr = (char *)data->data;

    if(data->size < byte_position+sizeof(time_t))
        return;

	if(!net_IsValidCacheDBT(data))
		return;

    COPY_INT32(ptr+byte_position, &date);

    return;

}

/* Gets the filename within a cache DBT struct so
 * that we don't have to convert it into a CacheObject
 *
 * This works because of the fixed length record format
 * of the first part of the specific DBT format I'm
 * using
 *
 * returns NULL on error
 */
#define MAX_FILE_SIZE 2048
MODULE_PRIVATE char *
net_GetFilenameInCacheDBT(DBT *data)
{
	int32 size;
	char *rv;
	char *ptr = (char*)data->data;

    if(data->size < FILENAME_BYTE_POSITION)
        return(NULL);

	if(!net_IsValidCacheDBT(data))
		return(0);

    COPY_INT32(&size, ptr+FILENAME_SIZE_BYTE_POSITION);

    if(data->size < FILENAME_BYTE_POSITION+size 
		|| size > MAX_FILE_SIZE)
        return(NULL);

	rv = (char *)XP_ALLOC(size);
	if(!rv)
		return(NULL);
    XP_MEMCPY(rv, ptr+FILENAME_BYTE_POSITION, size);

	TRACEMSG(("Got filename: %s from DBT", rv));

    return(rv);
}

/* Gets a int32 within a DBT struct so
 * that we don't have to convert it into a CacheObject
 *
 * This works because of the fixed length record format
 * of the first part of the specific DBT format I'm
 * using
 *
 * returns 0 on error
 */
MODULE_PRIVATE time_t
net_GetInt32InCacheDBT(DBT *data, int byte_position)
{
    int32 num;
	char *ptr = (char *)data->data;

	if(!net_IsValidCacheDBT(data))
		return(0);

    if(data->size < byte_position+sizeof(time_t))
        return(0);

    COPY_INT32(&num, ptr+byte_position);

    /* TRACEMSG(("Got int32 from cache DBT: %d", num)); */
   
    return(num);

}

MODULE_PRIVATE void
net_FreeCacheDBTdata(DBT *stuff)
{
	if(stuff)
	  {
		FREE(stuff->data);
		FREE(stuff);
	  }
}

/* takes a database storage object and returns an un-malloc'd
 * cache data object. The structure returned has pointers
 * directly into the database memory and are only valid
 * until the next call to any database function
 *
 * do not free anything returned by this structure
 */
MODULE_PRIVATE net_CacheObject *
net_Fast_DBDataToCacheStruct(DBT *obj)
{
	static net_CacheObject *rv=0;

	/* free any previous one */
	if(rv)
		net_freeCacheObj(rv);

	rv = net_DBDataToCacheStruct(obj);

	return(rv);

}

#endif /* MOZILLA_CLIENT */
