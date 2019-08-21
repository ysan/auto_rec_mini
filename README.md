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
  

Build and install
------------
...


How to use
----------
### System requirements ###

##### Tuner #####
Target tuner is KTV-FSUSB2/V3. (S/N: K1212 later)
one tuner. only terrestrial digital.

##### Dependencies #####
* libarib25
* libpcsclite

##### Platforms #####
Generic Linux will be ok. (confirmed worked on Ubuntu, Fedora, Raspbian)

Raspberry pi model B (Raspbian) is used in the main,
Packet loss occurs, block noise and image skipping are likely to occur.
There was a case that ts could not be taken because of insufficient power at the start of tuning.

### How to run ###
...


Component diagram
------------
...


Others
------------
Setting value etc. Have static data by using cereal json serializer.  
https://github.com/USCiLab/cereal  
(no use DB.)
  
The following repositories are referred to the parser.  
* https://github.com/stz2012/libarib25  
* https://github.com/Piro77/epgdump  
* https://github.com/youzaka/ariblib  
* https://github.com/arairait/eit_txtout_mod  
  
What is being used:  
* aribstr  https://github.com/Piro77/epgdump
* recfsusb2i (tuner<->USB control) https://github.com/jeeb/recfsusb2i  

LICENSE
------------
It becomes GPL depending on the license of the source code imported from the outside.

