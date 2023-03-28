#ifndef __DS_ERR_h_
#define __DS_ERR_h_

#define DS_ERROR_BASE				(-0x1000)
#define DS_ERROR_LIMIT				(DS_ERROR_BASE + 1000)

#define IS_DS_ERROR(code) \
    (((code) >= DS_ERROR_BASE) && ((code) < DS_ERROR_LIMIT))

/*
** DS library error codes
*/
#define DS_ERROR_OUT_OF_MEMORY			(DS_ERROR_BASE + 0)

#endif /* __DS_ERR_h_ */
