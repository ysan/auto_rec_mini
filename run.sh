#!/bin/bash

ulimit -c unlimited


#BASE_DIR=${HOME}/prog/auto_rec_mini
BASE_DIR=./


export LD_LIBRARY_PATH=${BASE_DIR}/local_build/lib/auto_rec_mini:


if [ "$1" = "gdb" ]; then
	gdb ${BASE_DIR}/local_build/bin/auto_rec_mini
else
#	${BASE_DIR}/local_build/bin/auto_rec_mini --conf=./data/settings.json >/dev/null 2>&1
	${BASE_DIR}/local_build/bin/auto_rec_mini --conf=./data/settings.json
fi
