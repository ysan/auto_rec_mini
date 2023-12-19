[Japanese](/README_ja.md)

auto_rec_mini
===============

[![Build Status](https://github.com/ysan/auto_rec_mini/actions/workflows/ci.yml/badge.svg)](https://github.com/ysan/auto_rec_mini/actions/workflows/ci.yml)

Digital broadcast recording system for Japan. This middleware is a tiny and minimal implementation.  
Recording, EPG acquisition, keyword search recording reservation are the main functions.  
(+ Real-time viewing function (in browser by HLS).)  
Simple to build and use.  
  
For self studying MPEG-2 TS and ARIB.  

<!--
![demo](https://github.com/ysan/auto_rec_mini/blob/master/etc/demo.gif)
-->

Features
-------------
* Tuning
  * Terrestrial digital only.
  * Simultaneous selection by multiple tuners.
* Channel scan
* Record and schedule recording
  * Simultaneous recording with multiple tuners.
  * Event tracking.
  * After obtaining the EPG, search for the registered keyword and schedule recording.
  * Introduced tssplitter_lite.
* EPG
  * Perform scheduled EPG acquisition.
* Viewing (*Platform dependent)
  * View broadcasts in real time on your browser using ffmpeg HLS.
  * Simultaneous viewing with multiple tuners.
  * Introduced tssplitter_lite.
* Broadcaster logo download
  * Save what fell during song selection in PNG format.
  * Since palette information (CLUT common fixed color) is added, it can be viewed as it is. (It should be possible.)
* Command Line Interface (CLI)
  * Connect to the `command server` with `telnet` or `netcat`, etc., and execute commands that access each function.

Future tasks
------------
* BS/CS compatible.
* BML-related functions for data broadcasting. (Implementation of dsmcc)


System requirements
----------

### Tuners ###
The supported tuners are as follows. only terrestrial digitl.
* [`KTV-FSUSB2/V3`](http://www.keian.co.jp/products/ktv-fsusb2v3/#spec-table) (S/N: K1212 later)  
* [`PX-S1UD V2.0`](http://www.plex-net.co.jp/product/px-s1udv2/)

Anything that works with other PLEX tuners or `recdvb` should work.

### Platforms ###
Will work on generic linux destributions. (confirmed worked on `Ubuntu`, `Fedora`, `Raspberry Pi OS`)  
  
I usually use `Raspberry Pi3 model B` to check the operation,  
If the back EPG acquisition runs during recording, or if multiple simultaneous recordings are performed,  
packet loss may occur, causing block noise and image skipping..  
~~There was a case that ts could not be taken because of insufficient power at the start of tuning.~~  
As a remedy, using a 5V4A power adapter has improved somewhat.
These will also be improved on Pi4.
  
Regarding the viewing function, we use ffmpeg to encode in H.264 and view it in a browser using the HLS streaming method.  
H.264 assumes HW encoding on Raspberry Pi4 (h264_omx), but encoding could not keep up with the playback speed on Pi3.  
It also depends on the version of ffmpeg, and operation has been confirmed with versions below 4.1.  
(For versions 4.2 and above, an error will occur due to the subtitle stream.)  
(On Raspberry Pi OS, when using Buster, ffmpeg 4.1 series is installed by default.)  
  
Also using B-CAS card with IC card reader.

### Dependencies ###
Depends on the following libraries.  
Please install as appropriate.  
  
*libpcsclite*

	$ sudo apt-get install pcscd libpcsclite-dev libccid pcsc-tools

*libarib25*

	$ sudo apt-get install cmake g++
	$ git clone https://github.com/stz2012/libarib25.git
	$ cd libarib25
	$ mkdir build
	$ cd build
	$ cmake ..
	$ make
	$ sudo make install

*Install firmware for PLEX tuners. (isdbt_rio.inp)*

	$ wget http://plex-net.co.jp/plex/px-s1ud/PX-S1UD_driver_Ver.1.0.1.zip
	$ unzip PX-S1UD_driver_Ver.1.0.1.zip
	$ sudo cp PX-S1UD_driver_Ver.1.0.1/x64/amd64/isdbt_rio.inp /lib/firmware/
	$ sync
	$ sudo reboot

Build and install
------------
### Build and install ###
The code depends on several git submodules.  
You need cloned the repository with --recursive.

	$ git clone --recursive https://github.com/ysan/auto_rec_mini.git
	$ cd auto_rec_mini
	$ make
	$ sudo make INSTALLDIR=/opt/auto_rec_mini install

Installation files:

	/opt/auto_rec_mini/
	├── bin
	│   ├── auto_rec_mini
	│   ├── recfsusb2i
	│   └── recdvb
	└── lib
	    ├── libchannelmanager.so
	    ├── libcommandserver.so
	    ├── libcommon.so
	    ├── libdsmccparser.so
	    ├── libeventschedulemanager.so
	    ├── libeventsearch.so
	    ├── libparser.so
	    ├── libpsisimanager.so
	    ├── libpsisiparser.so
	    ├── librecmanager.so
	    ├── libstreamhandler.so
	    ├── libthreadmgr.so
	    ├── libthreadmgrpp.so
	    ├── libtunercontrol.so
	    ├── libtunerservice.so
	    ├── libtunethread.so
	    └── libviewingmanager.so

### Running as a Linux service (use systemd) ###
Copy the configuration file(`settings.json`) to /opt/auto_rec_mini/ .

	$ sudo cp settings.json /opt/auto_rec_mini

Copy the systemd unit file. (`auto_rec_mini.service`)

	$ sudo cp auto_rec_mini.service /etc/systemd/system

To start the service for the first time, do the usual systemctl dance.

	$ sudo systemctl daemon-reload
	$ sudo systemctl enable auto_rec_mini.service
	$ sudo systemctl start auto_rec_mini.service

`auto_rec_mini` process is up and ready to use the tuner.  
`command server` is listening for connections on port 20001.  
First, please execute the `channel scan` CLI command.  

### Syslog configuration and rotation ###
Copy the syslog configuration file. (`30-auto_rec_mini.conf`) and restart syslog service.  
If you set `is_syslog_output` to `true` in `settings.json`,  
you can output to `/var/log/auto_rec_mini/auto_rec_mini.log`.

	$ sudo cp 30-auto_rec_mini.conf /etc/rsyslog.d
	$ sudo systemctl restart rsyslog.service

Copy the rotation configuration file. (`logrotate-auto_rec_mini`)  

	$ sudo cp logrotate-auto_rec_mini /etc/logrotate.d
	$ sudo chown root:root /etc/logrotate.d/logrotate-auto_rec_mini

### Uninstall ###
	$ sudo systemctl stop auto_rec_mini.service
	$ sudo systemctl disable auto_rec_mini.service
	$ sudo rm /etc/systemd/system/auto_rec_mini.service
	$ sudo systemctl daemon-reload
	$ sudo rm /etc/rsyslog.d/30-auto_rec_mini.conf
	$ sudo rm /etc/logrotate.d/logrotate-auto_rec_mini
	$ sudo systemctl restart rsyslog.service
	$ sudo make INSTALLDIR=/opt/auto_rec_mini clean
	$ make clean


settings.json
------------

`settings.json` is a configuration file.  
Read at `auto_rec_mini` process startup.  
There are no default values, and settings are made by reading this file.  
Please you set according to the environment.

| items | descriptions |
|:------|:-------------|
| `is_syslog_output` | Switch whether to output log to `syslog`. |
| `command_server_port` | Listening port of `command server`. |
| `tuner_hal_allocates` | Assign the tuner hal command. |
| `channels_json_path` | Save/load destination of channel scan result. |
| `rec_reserves_json_path` | Save/load destination of recording reservation. |
| `rec_results_json_path` | Save/load destination of recording result. |
| `rec_ts_path` | Save destination of recording stream. |
| `rec_disk_space_low_limit_MB` | Threshold of the remaining capacity of the file system for recording. (MBytes) |
| `rec_use_splitter` | Toggles whether to apply tssplitter_lite to the recording stream. |
| `dummy_tuner_ts_path` | unused |
| `event_schedule_cache_is_enable` | Switch to enable EPG. |
| `event_schedule_cache_start_interval_day` | EPG cache execution interval date. |
| `event_schedule_cache_start_time` | EPG cache start time. (HH:mm) |
| `event_schedule_cache_timeout_min` | EPG cache timeout minute. |
| `event_schedule_cache_retry_interval_min` | EPG cache retry interval minute. |
| `event_schedule_cache_data_json_path` | Save/load EPG cached json data. |
| `event_schedule_cache_histories_json_path` | Save/load destination of EPG cache history. |
| `event_name_keywords_json_path` | Save/load destination of keywords for `event name` search. |
| `extended_event_keywords_json_path` | Save/load destination of keywords for `extended event` search. |
| `event_name_search_histories_json_path` | Save/load destination of `event name` search history. |
| `extended_event_search_histories_json_path` | Save/load destination of `extended event` search history. |
| `viewing_stream_command_format` | Write the viewing stream command. |
| `viewing_stream_data_path` | Path to write/read the viewing stream file. |
| `viewing_use_splitter` | Toggle whether to apply tssplitter_lite to the viewing stream. |
| `http_server_port` | listening port of the HTTP server. |
| `http_server_static_contents_path` | Location where Web UI content is placed and corresponds to the document root. |
| `logo_path` | save/load destination of broadcaster logo png. |

### tuner_hal_allocates ###
Assign commands according to the tuner hardware you can use.

	"tuner_hal_allocates": [
		"./bin/recdvb %d - -",
		"./bin/recfsusb2i --np %d - -"
	],

### event_name_keywords_json_path ###

By creating json of the following format in `event_name_keywords_json_path`  
Search for programs whose keywords are included in the program name after EPG acquisition and  
make a recording reservation.  
No limit to the number of keywords.

	{
        "m_event_name_keywords": [
	        "ＸＸＸニュース",
	        "ＸＸＸスポーツ",
	        "ＸＸＸ報道"
	    ]
	}

### extended_event_keywords_json_path ###

By creating json of the following format in `extended_event_keywords_json_path`  
Search for programs whose keywords are included in the program name after EPG acquisition and  
make a recording reservation.  
No limit to the number of keywords.

	{
	    "m_extended_event_keywords": [
	        "ワールドカップ",
	        "オリンピック",
	        "野球"
	    ]
	}

### viewing_stream_command_format ###

As an initial value, we have included a command to encode in H264 using ffmpeg's HLS method.  


Client app
------------
### Build and install ###
Using Create React App.

	$ cd client
	$ npm ci
	$ npm run build

Please place the index.html and static directory generated in the build directory under `http_server_static_contents_path` described in `settings.json`.


How to use CLI
------------

### Connect to command server ###
At a new terminal with run `netcat` , `telnet`.

	$ nc -C 127.0.0.1 20001
	
	###  command line  begin. ###
	
	  ------ root tables ------
	  s                    -- system commands               
	  tc                   -- tuner control                 
	  pm                   -- psisi manager                 
	  rec                  -- rec manager                   
	  cm                   -- channel manager               
	  em                   -- event schedule manager        
	  es                   -- event search                  
	
	/ >
	/ >

### channel scan ###

	/ > cm
	
	  ------ channel manager ------
	  scan                 -- channel scan                  
	  tr                   -- tune by remote_control_key_id (usage: tr {remote_control_key_id} )
	  d                    -- dump channels
	
	/channel manager > 
	/channel manager > scan

Channel scan will start. Please wait a moment.

### dump recording reserve ###
...

### dump recording result ###
...

### run EPG cache (manually) ###
...

### dump EPG cache result ###
...


Component diagram
------------
<!--
![component diagram](https://github.com/ysan/auto_rec_mini/blob/master/etc/component_diagram.png)
-->
<img src="https://github.com/ysan/auto_rec_mini/blob/master/etc/component_diagram.png" width="75%" height="75%">

Others
------------
Using [`cpp-httplib`](https://github.com/yhirose/cpp-httplib) for the internal HTTP server.  
Using [`nlohmann/json`](https://github.com/nlohmann/json) for json SerDes.  
Setting value etc. Have static data by using [`cereal`](https://github.com/USCiLab/cereal) json SerDes. (no use DB.)  
Experimental implementation using [`thread_manger`](https://github.com/ysan/thread_manager).  
  
I have been influenced and referred to by the following repositories.
* [`libarib25`](https://github.com/stz2012/libarib25)
* [`epgdump`](https://github.com/Piro77/epgdump)
* [`ariblib`](https://github.com/youzaka/ariblib)
* [`eit_txtout_mod`](https://github.com/arairait/eit_txtout_mod)
* [`node-aribts`](https://github.com/rndomhack/node-aribts)
* [`TVRemotePlus`](https://github.com/tsukumijima/TVRemotePlus)
* [`KonomiTV`](https://github.com/tsukumijima/KonomiTV)
  
LICENSE
------------
MIT
