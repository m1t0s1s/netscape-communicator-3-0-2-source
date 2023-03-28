#ifndef __SEC_ERR_H_
#define __SEC_ERR_H_


#define SEC_ERROR_BASE				(-0x2000)
#define SEC_ERROR_LIMIT				(SEC_ERROR_BASE + 1000)

#define IS_SEC_ERROR(code) \
    (((code) >= SEC_ERROR_BASE) && ((code) < SEC_ERROR_LIMIT))


#endif /* __SEC_ERR_H_ */
