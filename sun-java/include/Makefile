#! gmake

DEPTH = ../..

include $(DEPTH)/sun-java/include/classfiles.mk

include $(DEPTH)/sun-java/config/rules.mk

# XXX: Fix to put the generated files into a $(OBJDIR) somewhere so that
# XXX: concurrent makes don't step on each other

GENFILES =			 \
	$(DESTDIR)opcodes.h	 \
	$(DESTDIR)opcodes.init	 \
	$(DESTDIR)opcodes.length \
	$(DESTDIR)opcodes.in_out

all:: generated

mac:: mac_headers $(GENFILES)
	cp $(GENFILES) $(DEPTH)/lib/mac/Java/_gen
	cp _gen/java_lang_String.h $(DEPTH)/lib/mac/Java/_gen/jdk_java_lang_String.h
	cp _gen/java_lang_Object.h $(DEPTH)/lib/mac/Java/_gen/jdk_java_lang_Object.h
	cp _gen/java_io_InputStream.h $(DEPTH)/lib/mac/Java/_gen/jdk_java_io_InputStream.h

export:: generated

generated:: headers $(GENFILES)

opcodes:: $(GENFILES)

JAVAH_OPTS = -d _gen

GARBAGE += $(GENFILES)

clobber::
	rm -rf _gen

OPCODES = $(DEPTH)/sun-java/include/opcodes.list

$(DESTDIR)opcodes.h: $(OPCODES) Makefile
	@$(BSDECHO) -n "Creating $@ ... "
	@rm -f $@
	@awk 'BEGIN    {print "typedef enum {" } \
	      /^[a-z]/ {printf("  opc_%s = %d,\n", $$1, i++); } \
	      END   {printf("  opc_first_unused_index = %d\n} opcode_type;\n",\
	                         i++) }' \
	   $(OPCODES)  > $@
	@chmod 444 $@
	@echo done.

$(DESTDIR)opcodes.init: $(OPCODES) Makefile
	@$(BSDECHO) -n "Creating $@ ... "
	@rm -f $@
	@awk 'BEGIN     { print "char *opnames[256] = {" } \
	      /^[a-z]/ { printf("   \"%s\",\n", $$1); i++; } \
	      END       { while(i++ < 256) print "   \"??\","; print "};" }' \
	   $(OPCODES)  > $@
	@chmod 444 $@
	@echo done.

$(DESTDIR)opcodes.length: $(OPCODES) Makefile
	@$(BSDECHO) -n "Creating $@ ... "
	@rm -f $@
	@awk 'BEGIN     { print "short opcode_length[256] = {" } \
	      /^[a-z]/  { printf("   %s,\t\t/* %s */\n", $$2, $$1); i++; } \
	      END       { while(i++ < 256) print "   -1,"; print "};" }' \
	  $(OPCODES)  > $@
	@chmod 444 $@
	@echo done.

$(DESTDIR)opcodes.in_out: $(OPCODES) Makefile
	@$(BSDECHO) -n "Creating $@ ... "
	@rm -f $@
	@awk 'BEGIN     { print "char *opcode_in_out[][2] = {" } \
	      /^[a-z]/  { if ($$3 == "-") $$3 = ""; \
	                  if ($$4 == "-") $$4 = ""; \
	                  printf("   {\"%s\", \"%s\"}, \t\t/* %s */\n", \
	                            $$3, $$4, $$1); }  \
	      END       { print "};" }' \
	  $(OPCODES)  > $@
	@chmod 444 $@
	@echo done.
