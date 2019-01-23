#!/bin/bash

#BASE_DIR=${HOME}/prog/atpp
BASE_DIR=./


export LD_LIBRARY_PATH=${BASE_DIR}/lib:

if [ "$1" = "gdb" ]; then
	gdb ${BASE_DIR}/atpp
else
	${BASE_DIR}/bin/atpp
fi
