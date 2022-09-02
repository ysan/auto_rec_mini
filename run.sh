#!/bin/bash

ulimit -c unlimited

export LD_LIBRARY_PATH=./local_build/lib:

#	./local_build/bin/auto_rec_mini --conf=./settings.json
	./local_build/bin/auto_rec_mini -c /opt/auto_rec_mini/settings.json -d /opt/auto_rec_mini
