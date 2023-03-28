#! /bin/sh

JAVA_HOME=/usr/local/netscape/java

if test "$CLASSPATH"x = "x"; then
  CLASSPATH=.:$JAVA_HOME/lib/javac.zip
  export CLASSPATH
fi

exec java java.tools.javadoc.Main $*
