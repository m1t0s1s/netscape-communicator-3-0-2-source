#!/bin/sh

DATATYPE="$1"
INFILE="$2"

echo "${DATATYPE} RCDATA"
sed 's/"/""/g' ${INFILE} | awk 'BEGIN { printf("BEGIN\n") } { printf("\"%s\\r\\n\",\n", $0) } END { printf("\"\\0\"\nEND\n") }'

exit 0
