#!/bin/bash

BASE_DIR=${HOME}/prog/atpp


export LD_LIBRARY_PATH=\
${BASE_DIR}/threadmgr:\
${BASE_DIR}/threadmgrpp:\
${BASE_DIR}/ts_parser/psisi:\
${BASE_DIR}/ts_parser/dsmcc:\
${BASE_DIR}/ts_parser:\
${BASE_DIR}/common:\
${BASE_DIR}/command_server:\



#if [ $# -ne 1 ]; then
#	echo "specify 1 argument."
#	exit 1
#fi

#if [ ! -e $1 -o ! -f $1 ]; then
#	echo "not existed [$1]"
#	exit 1
#fi

${BASE_DIR}/atpp
#gdb ${BASE_DIR}/atpp

