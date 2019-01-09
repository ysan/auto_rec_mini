#!/bin/bash

BASE_DIR=${HOME}/prog/atpp


export LD_LIBRARY_PATH=\
${BASE_DIR}/lib:\


#if [ $# -ne 1 ]; then
#	echo "specify 1 argument."
#	exit 1
#fi

#if [ ! -e $1 -o ! -f $1 ]; then
#	echo "not existed [$1]"
#	exit 1
#fi

${BASE_DIR}/bin/atpp
#gdb ${BASE_DIR}/atpp

