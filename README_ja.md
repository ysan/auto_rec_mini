[Japanese](/README.md)

atpp
===============

ARIB TS Parser and Processer.

最小機能のテレビ録画ミドルウェア。
小規模、軽量、簡易。

いまさらながら MPEG-2TSやARIBの勉強のため。
開発進行中です。


Features
------------ 
* 選局
  * 1チューナーで地デジのみとなります。
* 地デジ チャンネルスキャン
* 録画 及び 録画予約
  * 予約は50件まで。
  * 予約時間がかぶっている場合は先に録画していたものが
    終わり次第、次の録画を行います。
  * EPG取得後に指定したキーワードを検索し自動予約が可能です。
* EPG
  * 定時取得を行います。
* コマンドライン インターフェース
  * ポート20001 に```telnet```や```netcat```等で接続可能です。
    各機能にアクセスするコマンドを実行できます。(デバッグ目的)

Upcomings
------------
* データ放送のBMLの取得 (dsmccの実装)
* libarib25で行っているPMT周りの実装の勉強
* 複数チューナー、BS/CS対応


Build and install
------------
...


How to use
------------
#### System requirements ####

##### Tuner #####
対象のチューナーは KTV-FSUSB2/V3 です。 (S/N: K1212 later)
1チューナーで地デジのみとなります。
（他のチューナーにも対応してきたい。。）

##### Dependencies #####
* libarib25
* libpcsclite

##### Platforms #####
一般的なLinuxなら問題なく動作すると思います。(Ubuntu, Fedora, Raspbianで確認済。)

メインで使用しているのはRaspberry pi model B (Raspbian)ですが、
パケットロスが起こり、ブロックノイズ、画飛びが起きやすいです。
また選局開始時に電力が足りないせいかtsが取れないケースがありました。

#### How to run ####
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
* aribstr  https://github.com/Piro77/epgdump  
* recfsusb2i (tuner<->USB control) https://github.com/jeeb/recfsusb2i  


LICENSE
------------
流用しているコードがGPLとなっていましたので、それに準じています。

