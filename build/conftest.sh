#!/bin/sh
"$1" "$2" -o conftest "$3"/conftest.c >/dev/null 2>&1 && ./conftest && echo "$2"
rm -f conftest.o conftest core
