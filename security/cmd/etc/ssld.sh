#! /bin/sh
# 
# This shell is intended to be exec'd by ssld to pass "HTML" to a client
# that is connecting to ssld (see ssld.conf)
read i
echo "HTTP/1.0 200 OK"

echo "Server: ssld-test-hack"

echo "Content-Type: text/html"

echo

echo "<HR>"
echo "<P>You have reached the test ssld server on host: "
echo "<pre>"
hostname
echo "</pre>"
echo "<HR>"
echo "The current date is: "
echo "<pre>"
date
echo "</pre>"
echo "<HR>"
echo "<P>Have a nice day."

