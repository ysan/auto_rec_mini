#!/bin/bash

ulimit -c unlimited

export LD_LIBRARY_PATH=./local_build/lib:

if [ -f /opt/auto_rec_mini/settings.json ]; then
	echo "run in /opt/auto_rec_mini..."
	./local_build/bin/auto_rec_mini -c /opt/auto_rec_mini/settings.json -d /opt/auto_rec_mini
else
	./local_build/bin/auto_rec_mini --conf=./settings.json
fi
