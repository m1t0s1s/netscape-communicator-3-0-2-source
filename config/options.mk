# Example myconfig.mk prototype -- look for lines beginning with ## to find
# option settings to uncomment.

# Define pervasive debugging, instrumenting, etc. macros for your builds here.
##DEFINES += -DARENAMETER

# Set ETAGS to the name of the Emacs TAGS file generator command if you have
# use Emacs and have etags.
##ETAGS = etags

# Build Netscape Gold (AKA "the editor").
##BUILD_EDITOR = 1

# Build Java with garbage collector debugging enabled.
##BUILD_DEBUG_GC = 1

# Build without NSPR.
##NO_NSPR = 1

# Build without Java.
##NO_JAVA = 1

# Build without Mocha.
##NO_MOCHA = 1
