/* Semi-ripped-off of mips.h */

#include <sys/syscall.h>

#ifdef __ELF__
  /* With ELF, external symbols are not preceeded by underscore.  --jwz. */
# define __OS_CONNECT _OS_CONNECT
# define __OS_RECV    _OS_RECV
# define __OS_SELECT  _OS_SELECT
# define __OS_SEND    _OS_SEND
# define __OS_SOCKET  _OS_SOCKET
#endif /* __ELF__ */

#if 0
#define LINUX_SYS_CALL(name, sysname, args) \
    .globl __OS_##name##;		    \
    .align 4;				    \
  __OS_##name##:			    \
    pushl %ebp;				    \
    movl %esp,%ebp;			    \
    PUSH_##args				    \
    movl $(SYS_##sysname), %eax;	    \
    MOVE_##args				    \
    int $0x80;				    \
    POP_##args				    \
    movl %ebp,%esp;			    \
    popl %ebp;				    \
    ret;

#define PUSH_0	/* No arguments to push.  */
#define PUSH_1	pushl %ebx;
#define PUSH_2	PUSH_1
#define PUSH_3	PUSH_1
#define PUSH_4	pushl %esi; PUSH_3
#define PUSH_5	pushl %edi; PUSH_4

#define MOVE_0  /* No arguments to move.  */
#define MOVE_1  movl 8(%ebp),%ebx;
#define MOVE_2  MOVE_1 movl 12(%ebp),%ecx;
#define MOVE_3  MOVE_2 movl 16(%ebp),%edx;
#define MOVE_4  MOVE_3 movl 20(%ebp),%esi;
#define MOVE_5  MOVE_4 movl 24(%ebp),%edi;

#define POP_0	/* No arguments to pop.  */
#define POP_1	popl %ebx;
#define POP_2	POP_1
#define POP_3	POP_1
#define POP_4	POP_3 popl %esi;
#define POP_5	POP_4 popl %edi;

LINUX_SYS_CALL(OPEN, open, 3)

LINUX_SYS_CALL(CLOSE, close, 1)

LINUX_SYS_CALL(READ, read, 3)

LINUX_SYS_CALL(WRITE, write, 3)

LINUX_SYS_CALL(FCNTL, fcntl, 3)
#endif

/* From /usr/include/linux/net.h */
#define SYS_SOCKET      1               /* sys_socket(2)                */
#define SYS_BIND        2               /* sys_bind(2)                  */
#define SYS_CONNECT     3               /* sys_connect(2)               */
#define SYS_LISTEN      4               /* sys_listen(2)                */
#define SYS_ACCEPT      5               /* sys_accept(2)                */
#define SYS_GETSOCKNAME 6               /* sys_getsockname(2)           */
#define SYS_GETPEERNAME 7               /* sys_getpeername(2)           */
#define SYS_SOCKETPAIR  8               /* sys_socketpair(2)            */
#define SYS_SEND        9               /* sys_send(2)                  */
#define SYS_RECV        10              /* sys_recv(2)                  */
#define SYS_SENDTO      11              /* sys_sendto(2)                */
#define SYS_RECVFROM    12              /* sys_recvfrom(2)              */
#define SYS_SHUTDOWN    13              /* sys_shutdown(2)              */
#define SYS_SETSOCKOPT  14              /* sys_setsockopt(2)            */
#define SYS_GETSOCKOPT  15              /* sys_getsockopt(2)            */

	.globl	__OS_SEND
	.align	4
__OS_SEND:
	pushl  %ebp
	movl   %esp,%ebp
	subl   $0x10,%esp
	pushl  %ebx
	movl   0xc(%ebp),%edx
	movl   0x10(%ebp),%ecx
	movl   0x14(%ebp),%ebx
	movl   %ebx,0xfffffffc(%ebp)
	movl   0x8(%ebp),%eax
	movl   %eax,0xfffffff0(%ebp)
	movl   %edx,0xfffffff4(%ebp)
	movl   %ecx,0xfffffff8(%ebp)
	movl   $(SYS_SEND),%edx
	leal   0xfffffff0(%ebp),%ecx
	movl   $(SYS_socketcall),%eax
	movl   %edx,%ebx
	int    $0x80
	movl   0xffffffec(%ebp),%ebx
	movl   %ebp,%esp
	popl   %ebp
	ret    

	.globl	__OS_RECV
	.align	4
__OS_RECV:
	pushl  %ebp
	movl   %esp,%ebp
	subl   $0x10,%esp
	pushl  %ebx
	movl   0xc(%ebp),%edx
	movl   0x10(%ebp),%ecx
	movl   0x14(%ebp),%ebx
	movl   %ebx,0xfffffffc(%ebp)
	movl   0x8(%ebp),%eax
	movl   %eax,0xfffffff0(%ebp)
	movl   %edx,0xfffffff4(%ebp)
	movl   %ecx,0xfffffff8(%ebp)
	movl   $(SYS_RECV),%edx
	leal   0xfffffff0(%ebp),%ecx
	movl   $(SYS_socketcall),%eax
	movl   %edx,%ebx
	int    $0x80
	movl   0xffffffec(%ebp),%ebx
	movl   %ebp,%esp
	popl   %ebp
	ret    

	.globl	__OS_SELECT
	.align	4
__OS_SELECT:
	pushl  %ebp
	movl   %esp,%ebp
	pushl  %ebx
	movl   $(SYS_select),%eax
	leal   0x8(%ebp),%ebx
	int    $0x80
	movl   0xfffffffc(%ebp),%ebx
	movl   %ebp,%esp
	popl   %ebp
	ret    

	.globl	__OS_CONNECT
	.align	4
__OS_CONNECT:
	pushl  %ebp
	movl   %esp,%ebp
	subl   $0xc,%esp
	pushl  %ebx
	movl   0xc(%ebp),%edx
	movl   0x10(%ebp),%ecx
	movl   0x8(%ebp),%ebx
	movl   %ebx,0xfffffff4(%ebp)
	movl   %edx,0xfffffff8(%ebp)
	movl   %ecx,0xfffffffc(%ebp)
	movl   $(SYS_CONNECT),%edx
	leal   0xfffffff4(%ebp),%ecx
	movl   $(SYS_socketcall),%eax
	movl   %edx,%ebx
	int    $0x80
	movl   0xfffffff0(%ebp),%ebx
	movl   %ebp,%esp
	popl   %ebp
	ret    

	.globl	__OS_SOCKET
	.align	4
__OS_SOCKET:
	pushl  %ebp
	movl   %esp,%ebp
	subl   $0xc,%esp
	pushl  %ebx
	movl   0xc(%ebp),%edx
	movl   0x10(%ebp),%ecx
	movl   0x8(%ebp),%ebx
	movl   %ebx,0xfffffff4(%ebp)
	movl   %edx,0xfffffff8(%ebp)
	movl   %ecx,0xfffffffc(%ebp)
	movl   $(SYS_SOCKET),%edx
	leal   0xfffffff4(%ebp),%ecx
	movl   $(SYS_socketcall),%eax
	movl   %edx,%ebx
	int    $0x80
	movl   0xfffffff0(%ebp),%ebx
	movl   %ebp,%esp
	popl   %ebp
	ret    
