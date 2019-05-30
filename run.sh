#!/bin/bash

#BASE_DIR=${HOME}/prog/atpp
BASE_DIR=./


export LD_LIBRARY_PATH=${BASE_DIR}/local_build/lib/atpp:


if [ "$1" = "gdb" ]; then
	gdb ${BASE_DIR}/local_build/bin/atpp
else
#	${BASE_DIR}/local_build/bin/atpp ./data/settings.json >/dev/null 2>&1
	${BASE_DIR}/local_build/bin/atpp ./data/settings.json
fi
