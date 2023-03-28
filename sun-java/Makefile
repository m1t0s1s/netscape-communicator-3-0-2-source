#! gmake

DEPTH = ..

DIRS =	classsrc				     \
	javah include runtime md awt mmedia netscape \
	java

OTHER_DIRS = javac javadoc doc javap

include $(DEPTH)/sun-java/config/rules.mk

LOOP_OVER_OTHER_DIRS =						\
	@for d in $(OTHER_DIRS); do				\
		if test -d $$d; then				\
			set -e;					\
			echo "cd $$d; $(MAKE) $@";		\
			cd $$d; $(MAKE) $@; cd ..;		\
			set +e;					\
		else						\
			echo "Skipping non-directory $$d...";	\
		fi;						\
	done

.PHONY: $(OTHER_DIRS)

$(OTHER_DIRS)::
	@if test -d $@; then				\
		set -e;					\
		echo "cd $@; $(MAKE)";			\
		cd $@; $(MAKE);				\
		set +e;					\
	else						\
		echo "Skipping non-directory $@...";	\
	fi

other_dirs::
	+$(LOOP_OVER_OTHER_DIRS)
