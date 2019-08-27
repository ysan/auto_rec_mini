[Japanese](/README_ja.md)

atpp
===============

ARIB TS Parser and Processer.  
  
Like a minimum TV recorder middleware.  
Small, light and simple.  
feasibility study.  
under development, in progress.


Features
------------ 
* Tuning. (one tuner. only terrestrial digital.)
* Channel scan.
* Recording and recording reservation. (keyword search)
* EPG.
* Command line interface. (connect via telnet. port 20001)
  

How to use
----------
### System requirements ###

##### Tuner #####
Target tuner is KTV-FSUSB2/V3. (S/N: K1212 later)  
one tuner. only terrestrial digital.

##### Dependencies #####
Depends on the following libraries.  
Please install as appropriate. I will omit it here.

* `libarib25`
* `libpcsclite`

##### Platforms #####
Generic Linux will be ok. (confirmed worked on `Ubuntu`, `Fedora`, `Raspbian`)  
  
Raspberry pi model B (Raspbian) is used in the main,  
Packet loss occurs, block noise and image skipping are likely to occur.  
There was a case that ts could not be taken because of insufficient power at the start of tuning.

### Build and install ###
`clone` and `make`.

	$ git clone https://github.com/ysan/atpp
	$ cd atpp
	$ make

When installing in a working directory.

	$ mkdir -p ~/work/data
	$ make INSTALLDIR=~/work install
	$ cp -p ./data/settings.json ~/work/data
	$ cd ~/work
	$ tree
	.
	├── bin
	│   └── atpp
	├── data
	│   └── settings.json
	└── lib
	   	└── atpp
	        ├── libchannelmanager.so
	        ├── libcommandserver.so
	        ├── libcommon.so
	        ├── libdsmccparser.so
	        ├── libeventschedulemanager.so
	        ├── libeventsearch.so
	        ├── libit9175.so
	        ├── libparser.so
	        ├── libpsisimanager.so
	        ├── libpsisiparser.so
	        ├── librecmanager.so
	        ├── libthreadmgr.so
	        ├── libthreadmgrpp.so
	        ├── libtunercontrol.so
	        └── libtunethread.so
	
	4 directories, 17 files

### settings.json ###

`settings.json` is a configuration file.  
Read at `atpp` process startup.  
Please you set according to the environment.

| item | description |
|:-----|:------------|
| `m_is_syslog_output` | switch whether to output log to `syslog`. `syslog facility` is `user`. |
| `m_command_server_port` | `command server` listen port. |
| `m_channels_json_path` | save/load destination of channel scan result. |
| `m_rec_reserves_json_path` | save/load destination of recording reservation. |
| `m_rec_results_json_path` | save/load destination of recording result. |
| `m_rec_ts_path` | save destination of recording stream. |
| `m_dummy_tuner_ts_path` | unused |
| `m_event_schedule_cache_is_enable` | switch to enable EPG. |
| `m_event_schedule_cache_start_interval_day` | EPG cache execution interval date. |
| `m_event_schedule_cache_start_hour` | EPG cache start time. (hour) |
| `m_event_schedule_cache_start_min` | EPG cache start time. (minute) |
| `m_event_schedule_cache_timeout_min` | EPG cache timeout minute. |
| `m_event_schedule_cache_histories_json_path` | save/load destination of EPG cache history. |
| `m_event_name_keywords_json_path` | load destination of keywords for `event name` search. |
| `m_extended_event_keywords_json_path` | load destination of keywords for `extended event` search. |

### event_name_keywords.json ###

### extended_event_keywords.json ###

### How to run ###

When excuting in a working directory at `Build and install`.  
(You need root to run it.)

	$ cd ~/work
	$ export LD_LIBRARY_PATH=./lib/atpp
	$ sudo ./bin/atpp ./data/settings.json &

`atpp` process is up and ready to use the tuner.  
`command server` is listening for connections on port 20001.

### How to use CLI ###

##### Connect to command server #####
At a new terminal with run `netcat` , `telnet`.  
`localhost` or external address.
`command server` is single client.

	$ nc -C localhost 20001
	
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

##### channel scan #####

	/ > cm
	
	  ------ channel manager ------
	  scan                 -- channel scan                  
	  tr                   -- tune by remote_control_key_id (usage: tr {remote_control_key_id} )
	  ds                   -- dump channel scan results     
	
	/channel manager > 
	/channel manager > scan

channel scan is start.


Component diagram
------------
...


Others
------------
Setting value etc. Have static data by using [`cereal`](https://github.com/USCiLab/cereal) json serializer.  
(no use DB.)
  
The following repositories are referred to the parser.  
* [`libarib25`](https://github.com/stz2012/libarib25)
* [`epgdump`](https://github.com/Piro77/epgdump)
* [`ariblib`](https://github.com/youzaka/ariblib)
* [`eit_txtout_mod`](https://github.com/arairait/eit_txtout_mod)
  
What is being used:  
* aribstr from [`epgdump`](https://github.com/Piro77/epgdump)
* [`recfsusb2i`](https://github.com/jeeb/recfsusb2i) (tuner<->USB control)


LICENSE
------------
It becomes GPL depending on the license of the source code imported from the outside.

