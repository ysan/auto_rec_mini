atpp
===============

Arib TS Parser and Processer. Like a minimum TV.  
under development, in progress.  


Platforms
------------
Generic Linux will be ok. (confirmed worked on Ubuntu, Fedora, raspbian)  
Require is libarib25, libpcsclite.  

etc
------------
parser周りは以下のレポジトリ様を参考にさせていただいています。  
https://github.com/stz2012/libarib25  
https://github.com/Piro77/epgdump  
https://github.com/youzaka/ariblib  
https://github.com/arairait/eit_txtout_mod  
  
流用しているもの:  
&nbsp;&nbsp;&nbsp;&nbsp;epgdump の aribstr。  
&nbsp;&nbsp;&nbsp;&nbsp;recfsusb2i&nbsp;チューナー制御。&nbsp;https://github.com/jeeb/recfsusb2i  
  
チューナーは KTV-FSUSB2/V3 を使用しています。(S/N: K1212以降)
