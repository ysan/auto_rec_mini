[Japanese](/README_ja.md)

auto_rec_mini
===============

[![Build Status](https://travis-ci.org/ysan/auto_rec_mini.svg?branch=master)](https://travis-ci.org/ysan/auto_rec_mini)

Digital broadcast recording system for Japan. The middleware daemon with minimal functions.  
Keyword search recording reservation is the main function.  
  
For studying MPEG-2 TS and ARIB.  
The operation may not be stable depending on the environment.  

<!--
![demo](https://github.com/ysan/auto_rec_mini/blob/master/etc/demo.gif)
-->

Features
------------ 
* Tuning. (only terrestrial digital.)
* Channel scan.
* Recording and recording reservation. (keyword search)
* EPG.
* Download broadcaster Logo.
* Command line interface. (connect via telnet. port 20001)
  

System requirements
----------

### Tuner ###
The supported tuners are as follows. only terrestrial digitl. orz
* [`KTV-FSUSB2/V3`](http://www.keian.co.jp/products/ktv-fsusb2v3/#spec-table) (S/N: K1212 later)  
* [`PX-S1UD V2.0`](http://www.plex-net.co.jp/product/px-s1udv2/)

### Platforms ###
Will work on generic linux destributions. (confirmed worked on `Ubuntu`, `Fedora`, `Raspbian`)  
  
I usually use `Raspberry pi model B` to check the operation,  
If the back EPG acquisition runs during recording, or if multiple simultaneous recordings are performed,  
Packet loss occurs, block noise and image skipping are likely to occur.  
~~There was a case that ts could not be taken because of insufficient power at the start of tuning.~~
  
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

Build and install
------------
### Build and install ###

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
	    ├── libthreadmgr.so
	    ├── libthreadmgrpp.so
	    ├── libtunercontrol.so
	    ├── libtunerservice.so
	    └── libtunethread.so

### Running as a Linux service (use systemd) ###
Copy the config file(`settings.json`) to /opt/auto_rec_mini/ .

	$ sudo mkdir -p /opt/auto_rec_mini/data
	$ sudo cp settings.json /opt/auto_rec_mini

Copy the systemd unit file. (`auto_rec_mini.service`)

	$ sudo cp auto_rec_mini.service /etc/systemd/system

To start the service for the first time, do the usual systemctl dance.

	$ sudo systemctl daemon-reload
	$ sudo systemctl enable auto_rec_mini
	$ sudo systemctl start auto_rec_mini

`auto_rec_mini` process is up and ready to use the tuner.  
`command server` is listening for connections on port 20001.  
First, please execute the `channel scan` CLI command.  
  
### Clean ###  
	$ sudo systemctl stop auto_rec_mini
	$ sudo systemctl disable auto_rec_mini
	$ sudo rm /etc/systemd/system/auto_rec_mini.service
	$ sudo systemctl daemon-reload
	$ sudo make INSTALLDIR=/opt/auto_rec_mini clean
	$ make clean


settings.json
------------

`settings.json` is a configuration file.  
Read at `auto_rec_mini` process startup.  
Please you set according to the environment.

| item | description |
|:-----|:------------|
| `is_syslog_output` | switch whether to output log to `syslog`. |
| `command_server_port` | `command server` listen port. |
| `tuner_hal_allocates` | assign the tuner hal command. |
| `channels_json_path` | save/load destination of channel scan result. |
| `rec_reserves_json_path` | save/load destination of recording reservation. |
| `rec_results_json_path` | save/load destination of recording result. |
| `rec_ts_path` | save destination of recording stream. |
| `dummy_tuner_ts_path` | unused |
| `event_schedule_cache_is_enable` | switch to enable EPG. |
| `event_schedule_cache_start_interval_day` | EPG cache execution interval date. |
| `event_schedule_cache_start_time` | EPG cache start time. (HH:mm) |
| `event_schedule_cache_timeout_min` | EPG cache timeout minute. |
| `event_schedule_cache_retry_interval_min` | EPG cache retry interval minute. |
| `event_schedule_cache_data_json_path` | save/load EPG cached json data. |
| `event_schedule_cache_histories_json_path` | save/load destination of EPG cache history. |
| `event_name_keywords_json_path` | save/load destination of keywords for `event name` search. |
| `extended_event_keywords_json_path` | save/load destination of keywords for `extended event` search. |
| `event_name_search_histories_json_path` | save/load destination of `event name` search history. |
| `extended_event_search_histories_json_path` | save/load destination of `extended event` search history. |
| `logo_path` | save/load destination of broadcaster logo png. |

### is_syslog_output ###
Logs can be output to `/var/log/user.log` by setting the `syslog facility` is `user`.  
You need modify `/etc/rsyslog.d/50-default.conf`. (case `ubuntu16.04`)

	9c9
	< *.*;auth,authpriv.none		-/var/log/syslog
	---
	> *.*;auth,authpriv.none,user.none		-/var/log/syslog
	15c15
	< #user.*				-/var/log/user.log
	---
	> user.*				-/var/log/user.log

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
Setting value etc. Have static data by using [`cereal`](https://github.com/USCiLab/cereal) json serializer.  
(no use DB.)
  
The following repositories are referred to the parser.  
* [`libarib25`](https://github.com/stz2012/libarib25)
* [`epgdump`](https://github.com/Piro77/epgdump)
* [`ariblib`](https://github.com/youzaka/ariblib)
* [`eit_txtout_mod`](https://github.com/arairait/eit_txtout_mod)
  
LICENSE
------------
MIT

Feedback
------------
Please give feedback.

