#ifndef _JIO_H_
#define _JIO_H_

/* JIO control bits. These can be or'd together */
#define JIO_UNRESTRICTED	~0
#define JIO_NONE		0
#define JIO_ALLOW_STDIN		0x01
#define JIO_ALLOW_STDOUT	0x02
#define JIO_ALLOW_SOCKET	0x04
#define JIO_ALLOW_FILE_INPUT	0x08
#define JIO_ALLOW_FILE_OUTPUT	0x10

typedef enum {
    JFS_UNRESTRICTED,		/* no C level filesystem checks */
    JFS_NONE			/* no filesystem access allowed */
} JavaFSMode;

typedef enum {
    JRT_UNRESTRICTED,		/* no C level runtime checks */
    JRT_NONE			/* no runtime access allowed */
} JavaRTMode;

/*
** Change the java io mode. The java io mode is the lowest level security
** check done by the C implementation of the native i/o methods.
*/
extern void set_java_io_mode(int new_mode);

/*
** Change the java fs mode. The java fs mode is the lowest level security
** check done by the C implementation of the native filesystem methods.
*/
extern void set_java_fs_mode(JavaFSMode new_mode);

/*
** Change the java runtime mode. The java runtime mode is the lowest
** level security check done by the C implementation of the native
** runtime methods.
*/
extern void set_java_rt_mode(JavaRTMode new_mode);

#endif /* _JIO_H_ */
