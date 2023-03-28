set args -geom +0+70 -xrm '*useStderrDialog:false' -xrm '*useStdoutDialog:false'
set height 0

handle SIGPIPE nostop pass noprint

#set env HOME /u/jwz/tmp
set env NS_MSG_DISPLAY_HOOK

dir ../../lib/libmime
dir ../../lib/libmsg
dir ../../lib/libnet

dir ../../security/lib/cert
dir ../../security/lib/crypto
dir ../../security/lib/hash
dir ../../security/lib/key
dir ../../security/lib/nav
dir ../../security/lib/pkcs11
dir ../../security/lib/pkcs12
dir ../../security/lib/pkcs7
dir ../../security/lib/ssl
dir ../../security/lib/util

b exit
#b abort
b x_error_handler
#b purify_stop_here

# bad places to be in nspr's malloc
b wrtwarning
b malloc_dump


#b XtAppError
#b XtAppErrorMsg
#b XtAppWarning
#b XtAppWarningMsg
#b XtDisplayStringConversionWarning
#b XtError
#b XtErrorMsg
#b XtStringConversionWarning
#b XtWarning
#b XtWarningMsg
#b _XmWarning
#b _XtAllocError
#b _XtDefaultError
#b _XtDefaultErrorMsg
#b _XtDefaultWarning
#b _XtDefaultWarningMsg



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
