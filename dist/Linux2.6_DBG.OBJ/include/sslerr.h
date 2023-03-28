#ifndef __SSL_ERR_H_
#define __SSL_ERR_H_


#define SSL_ERROR_BASE				(-0x3000)
#define SSL_ERROR_LIMIT				(SSL_ERROR_BASE + 1000)

#define IS_SSL_ERROR(code) \
    (((code) >= SSL_ERROR_BASE) && ((code) < SSL_ERROR_LIMIT))

#define SSL_ERROR_POST_WARNING			(SSL_ERROR_BASE + 13)

#endif /* __SSL_ERR_H_ */
