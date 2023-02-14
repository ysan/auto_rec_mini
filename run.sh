#!/bin/bash

# debug build command
#   make DEBUG_BUILD=1
#   make DEBUG_BUILD=1 install

# *** run this script with sudo ***

ulimit -c unlimited

export LD_LIBRARY_PATH=./local_build/lib:

if [ $# -eq 1 ] && [ $1 = "local" ]; then
	echo run in local
	./local_build/bin/auto_rec_mini --conf=./settings.json
elif [ -f /opt/auto_rec_mini/settings.json ]; then
	echo "run in /opt/auto_rec_mini..."
	./local_build/bin/auto_rec_mini -c /opt/auto_rec_mini/settings.json -d /opt/auto_rec_mini
else
	echo run in local
	./local_build/bin/auto_rec_mini --conf=./settings.json
fi


# core file analysis after reproduction
#   export LD_LIBRARY_PATH=./local_build/lib
#   gdb ./local_build/bin/auto_rec_mini core
#     gdb > bt
