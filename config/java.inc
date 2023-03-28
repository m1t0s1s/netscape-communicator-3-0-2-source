.SILENT:
#//------------------------------------------------------------------------
#//
#// This makefile contains all of the rules necessary to build Java Header
#// and Stub files
#//
#//------------------------------------------------------------------------
!if !defined(CONFIG_JAVA_INC)
CONFIG_JAVA_INC=1

!ifdef JAVA_HEADER_FILES
#//------------------------------------------------------------------------
#//
#// Rule to generate Java header files using javah.
#//
#//------------------------------------------------------------------------
$(JAVA_HEADER_FILES):: display_java_header_msg
    !$(MAKE_JAVA_HEADERS) -d _gen $@

display_java_header_msg:
	echo +++ MAKE: Generating java header files...

!endif

!ifdef JAVA_STUB_FILES
#//------------------------------------------------------------------------
#//
#// Rules to generate Java stub files using javah.
#//
#//------------------------------------------------------------------------
$(JAVA_STUB_FILES)::display_java_stub_msg
    !$(MAKE_JAVA_STUBS) -d _stubs $@

display_java_stub_msg:
	echo +++ MAKE: Generating java stub files...

!endif

!ifdef JAVA_LIBSTUB_FILES
#//------------------------------------------------------------------------
#//
#// Rules to generate libstubs.c using javah.
#//
#//------------------------------------------------------------------------
libstubs.c::$(JAVA_LIBSTUB_FILES)
	echo +++ MAKE: Generating libstubs.c...
    $(MAKE_JAVA_LIBSTUBS) $**

!endif

!ifdef JRI_HEADER_FILES
#//------------------------------------------------------------------------
#//
#// Rule to generate JRI header files using javah.
#//
#//------------------------------------------------------------------------
$(JRI_HEADER_FILES):: display_jri_header_msg
    !$(MAKE_JAVA_HEADERS) -jri -d _gen $@

display_jri_header_msg:
	echo +++ MAKE: Generating JRI header files...

!endif

!ifdef JRI_STUB_FILES
#//------------------------------------------------------------------------
#//
#// Rules to generate JRI stub files using javah.
#//
#//------------------------------------------------------------------------
$(JRI_STUB_FILES)::display_jri_stub_msg
    !$(MAKE_JAVA_STUBS) -jri -d _stubs $@

display_jri_stub_msg:
	echo +++ MAKE: Generating JRI stub files...

!endif

!endif # CONFIG_JAVA_INC
