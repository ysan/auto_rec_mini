#!/bin/bash

BASE_DIR=${HOME}/prog/atpp


export LD_LIBRARY_PATH=\
${BASE_DIR}/aribstr:\
${BASE_DIR}/psisi:\
${BASE_DIR}/dsmcc\



if [ $# -ne 1 ]; then
	echo "specify 1 argument."
	exit 1
fi

if [ ! -e $1 -o ! -f $1 ]; then
	echo "not existed [$1]"
	exit 1
fi

${BASE_DIR}/atpp -f $1
#gdb ${BASE_DIR}/atpp

