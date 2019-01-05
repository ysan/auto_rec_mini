#!/bin/bash

BASE_DIR=${HOME}/prog/atpp/ts_parser_test


export LD_LIBRARY_PATH=\
${BASE_DIR}/../ts_parser/aribstr:\
${BASE_DIR}/../ts_parser/psisi:\
${BASE_DIR}/../ts_parser/dsmcc:\
${BASE_DIR}/../ts_parser:\
${BASE_DIR}/../common:\



if [ $# -ne 1 ]; then
	echo "specify 1 argument."
	exit 1
fi

if [ ! -e $1 -o ! -f $1 ]; then
	echo "not existed [$1]"
	exit 1
fi

${BASE_DIR}/ts_parser -f $1
#gdb ${BASE_DIR}/ts_parser

