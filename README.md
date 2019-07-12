atpp
===============

Arib TS Parser and Processer. Like a minimum TV middleware.  
feasibility study.  
under development, in progress.  


Platforms
------------
Generic Linux will be ok. (confirmed worked on Ubuntu, Fedora, raspbian)  
Require is libarib25, libpcsclite.  

etc
------------
Target tuner is KTV-FSUSB2/V3. (S/N: K1212 later)  
one tuner. only terrestrial digital.  
  
functions:  
&nbsp;&nbsp;&nbsp;&nbsp;Tuning. (one tuner. only terrestrial digital.)  
&nbsp;&nbsp;&nbsp;&nbsp;Terrestrial digital channel scan.  
&nbsp;&nbsp;&nbsp;&nbsp;Recording and recording reservation.  
&nbsp;&nbsp;&nbsp;&nbsp;Command line interface. (connect via telnet. port 20001)  
&nbsp;&nbsp;&nbsp;&nbsp;EPG. (still in implementation...)  
  
Setting value etc. Have static data by using cereal json serializer.  
&nbsp;&nbsp;&nbsp;&nbsp;https://github.com/USCiLab/cereal  
  
The following repositories are referred to the parser.  
&nbsp;&nbsp;&nbsp;&nbsp;https://github.com/stz2012/libarib25  
&nbsp;&nbsp;&nbsp;&nbsp;https://github.com/Piro77/epgdump  
&nbsp;&nbsp;&nbsp;&nbsp;https://github.com/youzaka/ariblib  
&nbsp;&nbsp;&nbsp;&nbsp;https://github.com/arairait/eit_txtout_mod  
  
What is being used:  
&nbsp;&nbsp;&nbsp;&nbsp;aribstr of epgdump.  
&nbsp;&nbsp;&nbsp;&nbsp;recfsusb2i (tuner<->USB control) https://github.com/jeeb/recfsusb2i  
