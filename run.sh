#!/bin/bash

#BASE_DIR=${HOME}/prog/atpp
BASE_DIR=./


export LD_LIBRARY_PATH=${BASE_DIR}/lib:

${BASE_DIR}/bin/atpp
#gdb ${BASE_DIR}/atpp

