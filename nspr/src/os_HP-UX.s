	.CODE

machdep_error
	sub %r0,%r28,%r28			 
	bv,n %r0(%r2)				 

_OS_EXIT

	.PROC
	.CALLINFO	NO_CALLS,FRAME=0

	ldil -0x80000,%r1
	ble 4(%sr7,%r1)
	ldi 1  ,%r22
	or,= %r0,%r22,%r0
	b,n machdep_error
	bv,n %r0(%r2)

	.PROCEND
	.EXPORT		_OS_EXIT,ENTRY

_OS_CREAT 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 8  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_CREAT,ENTRY 

 


_OS_UNLINK 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 10  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_UNLINK,ENTRY 

 


_OS_FCHMOD 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 124  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_FCHMOD,ENTRY 

 


_OS_CHDIR 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 12  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_CHDIR,ENTRY 

 


_OS_CHMOD 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 15  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_CHMOD,ENTRY 

 


_OS_CHOWN 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 16  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_CHOWN,ENTRY 

 


_OS_RECVMSG 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 284  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_RECVMSG,ENTRY 

 


_OS_SENDMSG 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 286  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_SENDMSG,ENTRY 

 


_OS_SHUTDOWN 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 289  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_SHUTDOWN,ENTRY 

 


_OS_RENAME 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 128  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_RENAME,ENTRY 

 


_OS_LINK 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 9  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_LINK,ENTRY 

 


_OS_STAT 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 38  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_STAT,ENTRY 

 


_OS_FCHOWN 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 123  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_FCHOWN,ENTRY 

 


_OS_READ 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1
	ble 4(%sr7,%r1)				  					
	ldi 3  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_READ,ENTRY 

 


_OS_WRITE 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 4  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_WRITE,ENTRY 

 


_OS_OPEN 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 5  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_OPEN,ENTRY 

 


_OS_CLOSE 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 6  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_CLOSE,ENTRY 

 


_OS_LSEEK 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 19  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_LSEEK,ENTRY 

 


_OS_FCNTL 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 62  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_FCNTL,ENTRY 

 


_OS_DUP2 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 90  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_DUP2,ENTRY 

 


_OS_READV 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 120  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_READV,ENTRY 

 


_OS_WRITEV 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 121  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_WRITEV,ENTRY 

 


_OS_FSTAT 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 92  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_FSTAT,ENTRY 

 


_OS_PIPE 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 42  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_PIPE,ENTRY 

 


_OS_DUP 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 41  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_DUP,ENTRY 

 


_OS_FORK														
																		
	.PROC															
	.CALLINFO 	NO_CALLS,FRAME=0	 									
																		
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 2 ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 		
	or,= %r29,%r0,%r0				 				
	copy %r0,%r28				 	
	bv,n %r0(%r2)				 							
																		
	.PROCEND															
	.EXPORT		_OS_FORK,ENTRY

 


_OS_EXECVE 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 59  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_EXECVE,ENTRY 

 


_OS_WAIT3 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 84  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_WAIT3,ENTRY 

 


_OS_WAITPID 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 200  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_WAITPID,ENTRY 

 




_OS_SELECT 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 93  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_SELECT,ENTRY  

 


_OS_GETDIRENTRIES 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 195  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_GETDIRENTRIES,ENTRY 

 
 


_OS_ACCEPT 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 275  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_ACCEPT,ENTRY 

 


_OS_BIND 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 276  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_BIND,ENTRY 

 


_OS_CONNECT 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 277  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_CONNECT,ENTRY 

 


_OS_GETPEERNAME 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 278  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_GETPEERNAME,ENTRY 

 


_OS_GETSOCKOPT 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 280  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_GETSOCKOPT,ENTRY 

 


_OS_LISTEN 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 281  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_LISTEN,ENTRY 

 


_OS_SOCKET 														
	
	.PROC
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 290  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_SOCKET,ENTRY 

 


_OS_SEND 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 285  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_SEND,ENTRY 

 


_OS_SENDTO 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 287  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_SENDTO,ENTRY 

 


_OS_RECV 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 282  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_RECV,ENTRY 

 


_OS_RECVFROM 														
	
	.PROC																
	.CALLINFO 	NO_CALLS,FRAME=0	 									
	
	ldil -0x80000,%r1			 	
	ble 4(%sr7,%r1)				  					
	ldi 283  ,%r22			  		
	or,= %r0,%r22,%r0			 						
	b,n machdep_error			 			
	bv,n %r0(%r2)				 							
	
	.PROCEND															
	.EXPORT		_OS_RECVFROM,ENTRY 

_OS_SETSOCKOPT

	.PROC
	.CALLINFO	NO_CALLS,FRAME=0

	ldil -0x80000,%r1
	ble 4(%sr7,%r1)
	ldi 288,%r22
	or,= %r0,%r22,%r0
	b,n machdep_error
	bv,n %r0(%r2)

	.PROCEND
	.EXPORT		_OS_SETSOCKOPT,ENTRY


_OS_IOCTL

	.PROC
	.CALLINFO	NO_CALLS,FRAME=0

	ldil -0x80000,%r1
	ble 4(%sr7,%r1)
	ldi 54,%r22
	or,= %r0,%r22,%r0
	b,n machdep_error
	bv,n %r0(%r2)

	.PROCEND
	.EXPORT		_OS_IOCTL,ENTRY

_OS_SIGPROCMASK

	.PROC
	.CALLINFO   NO_CALLS,FRAME=0

	ldil -0x80000,%r1
	ble 4(%sr7,%r1)
	ldi 185,%r22
	or,= %r0,%r22,%r0
	b,n machdep_error
	bv,n %r0(%r2)

	.PROCEND
	.EXPORT     _OS_SIGPROCMASK,ENTRY

	.END
