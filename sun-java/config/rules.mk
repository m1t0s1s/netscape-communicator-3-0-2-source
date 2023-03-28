#! gmake

include $(DEPTH)/sun-java/config/config.mk

include $(DEPTH)/config/rules.mk

ifdef GENERATED_HEADERS_CLASSFILES

GENERATED_HEADERS_CLASSES = $(patsubst %,$(DEPTH)/sun-java/classsrc/%.class,$(subst .,/,$(GENERATED_HEADERS_CLASSFILES)))
GENERATED_HEADERS_CFILES = $(patsubst %,_gen/%.c,$(subst .,_,$(GENERATED_HEADERS_CLASSFILES)))

$(GENERATED_HEADERS_CFILES): $(GENERATED_HEADERS_CLASSES)

mac_headers:
	@echo Generating/Updating headers for the Mac
	$(JVH) -d $(DEPTH)/lib/mac/Java/_gen -mac $(GENERATED_HEADERS_CLASSFILES)

headers:
	@echo Generating/Updating headers
	$(JVH) -d _gen $(GENERATED_HEADERS_CLASSFILES)

.PHONY: headers

endif

ifdef GENERATED_STUBS_CLASSFILES

GENERATED_STUBS_CLASSES = $(patsubst %,$(DEPTH)/sun-java/classsrc/%.class,$(subst .,/,$(GENERATED_STUBS_CLASSFILES)))
GENERATED_STUBS_CFILES = $(patsubst %,_stubs/%.c,$(subst .,_,$(GENERATED_STUBS_CLASSFILES)))

$(GENERATED_STUBS_CFILES): $(GENERATED_STUBS_CLASSES)

mac_stubs:
	@echo Generating/Updating stubs for the Mac
	$(JVH) -stubs -d $(DEPTH)/lib/mac/Java/_stubs -mac $(GENERATED_STUBS_CLASSFILES)

stubs:
	@echo Generating/Updating stubs
	$(JVH) -stubs -d _stubs $(GENERATED_STUBS_CLASSFILES)

.PHONY: stubs

endif
