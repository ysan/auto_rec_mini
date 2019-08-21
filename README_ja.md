[English](/README.md)

atpp
===============

ARIB TS Parser and Processer.

最小機能のテレビ録画ミドルウェア。  
小規模、軽量、簡易。  
  
いまさらながら MPEG-2TSやARIBの勉強のため。 
開発中のため環境によっては動作が安定しない可能性があります。

Features
------------ 
* 選局
  * 1チューナー、地デジのみとなります。(チューナー：KTV-FSUSB2/V3)
* 地デジ チャンネルスキャン
* 録画 及び 録画予約
  * 予約は50件まで。
  * 予約時間が被っている場合は先に録画していたものが終わり次第、次の録画を行います。
  * EPG取得後に指定したキーワードを検索し自動予約を行います。
* EPG
  * 定時EPG取得を行います。
* コマンドライン インターフェース
  * ポート20001 に```telnet```や```netcat```等で接続できます。各機能にアクセスするコマンドの実行等を行います。(デバッグ用途でもあります。)

Upcomings
------------
* 現状は録画、EPG、等チューナーを使う機能間の排他ができてない。その実装を対応する。
* データ放送のBMLの取得を行う。 (dsmccの実装)
* libarib25で行っているPMT周りの実装の勉強、と実装を取り込みたい。
* 複数チューナー、BS/CS対応したい。


Build and install
------------
...


How to use
------------
### System requirements ###

##### Tuner #####
対象のチューナーは KTV-FSUSB2/V3 です。 (S/N: K1212 以降)  
1チューナーで地デジのみとなります。  
（他のチューナーにも対応してきたい。）

##### Dependencies #####
* libarib25
* libpcsclite

##### Platforms #####
一般的なLinuxなら問題なく動作すると思います。(Ubuntu, Fedora, Raspbianで確認済。)  
  
メインで使用しているのはRaspberry pi model B (Raspbian)ですが、  
パケットロスが起こり、ブロックノイズ、画飛びが起きやすいです。  
また選局開始時に電力が足りないせいかtsが取れないケースがありました。

### How to run ###
...


Component diagram
------------
...


Others
------------
設定値などの静的データは cereal を使用してjsonに落としています。  
https://github.com/USCiLab/cereal  
(現状DBは使用していません。)


パーサー周りは以下のレポジトリ様を参考にさせていただいています:
* https://github.com/stz2012/libarib25  
* https://github.com/Piro77/epgdump  
* https://github.com/youzaka/ariblib  
* https://github.com/arairait/eit_txtout_mod  

 
流用させていただいているもの:
* aribstr (ARIB外字)  https://github.com/Piro77/epgdump  
* recfsusb2i (tuner<->USB control)  https://github.com/jeeb/recfsusb2i  


LICENSE
------------
流用しているコードがGPLとなっていますので、それに準じています。

