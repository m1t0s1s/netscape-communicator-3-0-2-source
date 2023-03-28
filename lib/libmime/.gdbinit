
set height 0

handle SIGPIPE nostop pass noprint

set env HOME /u/jwz/tmp
set env NS_MSG_DISPLAY_HOOK

dir ../libmsg
dir ../libnet
#dir ../libsec
dir ../../security/lib/pkcs7
dir ../../security/lib/pkcs11
dir ../../security/lib/cert
dir ../../security/lib/key
dir ../../security/lib/util

#b exit
#b abort
#b purify_stop_here

# bad places to be in nspr's malloc
b wrtwarning
b malloc_dump

# From Erik / Kipp for getting stack traces out of NSPR and Java:

define mozbt
set $mySavedSP=$sp
set $mySavedPC=$pc
set $sp=((PRThread *) mozilla_thread)->context.uc_mcontext.gregs[29]
set $pc=((PRThread *) mozilla_thread)->context.uc_mcontext.gregs[31]
bt
set $sp=$mySavedSP
set $pc=$mySavedPC
end

define awtbt
set $mySavedSP=$sp
set $mySavedPC=$pc
set $sp=((PRThread *) awt_thread)->context.uc_mcontext.gregs[29]
set $pc=((PRThread *) awt_thread)->context.uc_mcontext.gregs[31]
bt
set $sp=$mySavedSP
set $pc=$mySavedPC
end
