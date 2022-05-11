#!/bin/bash

# build command
#   make DEBUG_BUILD=1
#   make DEBUG_BUILD=1 install


# *** run this script with sudo ***

ulimit -c unlimited

export LD_LIBRARY_PATH=./local_build/lib

./local_build/bin/auto_rec_mini -c /opt/auto_rec_mini/settings.json -d /opt/auto_rec_mini


# core file analysis after reproduction
#   export LD_LIBRARY_PATH=./local_build/lib
#   gdb ./local_build/bin/auto_rec_mini core
#     gdb > bt
