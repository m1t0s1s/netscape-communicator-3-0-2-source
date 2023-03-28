/
/
/ SCO syscall interface, not much to it
/
/#include <sys.s>
/
/#define SELECT 0x2428
/#define SYSCALL 0x7
/
/
        .globl  _R_OS_SELECT
        .globl  _cerror
_R_OS_SELECT:
        movl    $0x2428,%eax	/ select() entry value
        lcall   $0x7,$0 	/ call syscall()
        jc      _cerror
        ret
