/* We want position independent code */
#define PIC

#include <sys/asm.h>
#include <sys/regdef.h>
#include <sys/syscall.h>

	.file 1 "os_ReliantUNIX.s"
	.option	pic2
	.text

/*
** Stub for garbage collectors. Save "saved" registers into memory pointed
** at by a0. The registers are being saved into the "gregs" portion of the
** uc_mcontext structure defined in <sys/ucontext.h>.
** cmk: is the gregs part to be reused? s0-s8 are not continuous
**      in gregs!! s0-s7 -> r16-r23, but s8 -> r30 !!!
*/
LEAF(_MD_SaveSRegs)
	sw	s0,0(a0)
	sw	s1,4(a0)
	sw	s2,8(a0)
	sw	s3,12(a0)
	sw	s4,16(a0)
	sw	s5,20(a0)
	sw	s6,24(a0)
	sw	s7,28(a0)
	sw	s8,32(a0)
	jr	ra
	END(_MD_SaveSRegs)

/************************************************************************/

/*
** Thread safe versions of blocking system calls. These are wrapped up in
** C code to keep the NSPR threads from blocking each other by doing
** blocking system calls. They also return a negative version of errno
** when an error occurs so that we don't have a non-thread-safe errno.
*/

#define OS_SYS_CALL(n)	    \
	.set	noreorder;  \
			    \
	li	v0,SYS_##n; \
	syscall;	    \
	beq	a3,zero,9f; \
	nop;		    \
	neg	v0;	    \
			    \
9:	j	ra;	    \
	nop;		    \
			    \
	.set	reorder

#if 0
LEAF(_OS_SELECT)
	OS_SYS_CALL(select)
	END(_OS_SELECT)

LEAF(_OS_CONNECT)
	OS_SYS_CALL(connect)
	END(_OS_CONNECT)
#endif

LEAF(_OS_OPEN)
	OS_SYS_CALL(open)
	END(_OS_OPEN)

LEAF(_OS_CLOSE)
	OS_SYS_CALL(close)
	END(_OS_CLOSE)

LEAF(_OS_FCNTL)
	OS_SYS_CALL(fcntl)
	END(_OS_FCNTL)

LEAF(_OS_IOCTL)
	OS_SYS_CALL(ioctl)
	END(_OS_IOCTL)

#if 0
LEAF(_OS_SOCKET)
	OS_SYS_CALL(socket)
	END(_OS_SOCKET)
#endif

/* Data reception calls */

LEAF(_OS_READ)
	OS_SYS_CALL(read)
	END(_OS_READ)

#if 0
LEAF(_OS_RECV)
	OS_SYS_CALL(recv)
	END(_OS_RECV)

LEAF(_OS_RECVFROM)
	OS_SYS_CALL(recvfrom)
	END(_OS_RECVFROM)

LEAF(_OS_RECVMSG)
	OS_SYS_CALL(recvmsg)
	END(_OS_RECVMSG)

LEAF(_OS_READV)
	OS_SYS_CALL(readv)
	END(_OS_READV)

/* Data transmission calls */

LEAF(_OS_SEND)
	OS_SYS_CALL(send)
	END(_OS_SEND)

LEAF(_OS_SENDTO)
	OS_SYS_CALL(sendto)
	END(_OS_SENDTO)

LEAF(_OS_SENDMSG)
	OS_SYS_CALL(sendmsg)
	END(_OS_SENDMSG)
#endif

LEAF(_OS_WRITE)
	OS_SYS_CALL(write)
	END(_OS_WRITE)

#if 0
LEAF(_OS_WRITEV)
	OS_SYS_CALL(writev)
	END(_OS_WRITEV)
#endif

/* Stream calls */

LEAF(_OS_GETMSG)
	OS_SYS_CALL(getmsg)
	END(_OS_GETMSG)

LEAF(_OS_PUTMSG)
	OS_SYS_CALL(putmsg)
	END(_OS_PUTMSG)

LEAF(_OS_POLL)
	OS_SYS_CALL(poll)
	END(_OS_POLL)
