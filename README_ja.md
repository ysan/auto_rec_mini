[English](https://github.com/ysan/auto_rec_mini/)

auto_rec_mini
===============

[![Build Status](https://travis-ci.org/ysan/auto_rec_mini.svg?branch=master)](https://travis-ci.org/ysan/auto_rec_mini)

最小機能のTV録画ミドルウェア。  
主にキーワード検索録画予約機能がメインになります。  
  
MPEG-2TSやARIBの勉強のため。  
環境によっては動作が安定しない可能性があります。

<!--
![demo](https://github.com/ysan/auto_rec_mini/blob/master/etc/demo.gif)
-->

Features
------------ 
* 選局
  * 1チューナー、地デジのみ。
* チャンネルスキャン
* 録画 及び 録画予約
  * 予約は50件まで。
  * イベント追従。
  * 予約時間が被っている場合は先に録画していたものが終わり次第、次の録画を行います。
  * EPG取得後に登録したキーワードを検索し録画予約を行います。
* EPG
  * 定時EPG取得を行います。
* 放送局ロゴダウンロード
  * 選曲中に降ってきたものをPNG形式で保存します。
  * パレット情報（CLUT共通固定色）を付加するのでそのまま閲覧できます。(暫定)
* コマンドラインインターフェース (CLI)
  * `telnet`や`netcat`等で`command server`に接続して、各機能にアクセスするコマンドの実行等を行います。

Future tasks
------------
* libarib25で行っているPMT周りの実装の勉強、と実装を取り込みたい。
* 複数チューナー、BS/CS対応したい。
* データ放送のBMLの取得してみたい。 (dsmccの実装)


System requirements
------------

### Tuner ###
対応しているチューナーは以下のものです。今のところ1チューナー、地デジのみとなります。orz
* [`KTV-FSUSB2/V3`](http://www.keian.co.jp/products/ktv-fsusb2v3/#spec-table) (S/N: K1212 以降)  
* [`PX-S1UD V2.0`](http://www.plex-net.co.jp/product/px-s1udv2/)

### Platforms ###
一般的なLinuxであれば問題なく動作すると思います。(`Ubuntu`, `Fedora`, `Raspbian`で確認済。)  
  
メインで使用しているのは Raspberry pi model B ですが、  
パケットロスが起こり、ブロックノイズ、画飛びが起きやすいです。  
~~また選局開始時に電力が足りないせいかtsが取れないことがあります。~~
  
B-CASカードは別途USB接続のICカードリーダを用意して使用しています。

### Dependencies ###
下記のライブラリに依存します。  
適宜インストールをお願いします。  
  
*libpcsclite

	$ sudo apt-get install pcscd libpcsclite-dev libccid pcsc-tools

*libarib25*

	$ sudo apt-get install cmake
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

インストールファイル：

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
	    └── libtunethread.so

### Running as a Linux service (use systemd) ###
設定ファイル(`settings.json`) を /opt/auto_rec_mini/ にコピーします。

	$ sudo mkdir -p /opt/auto_rec_mini/data
	$ sudo cp settings.json /opt/auto_rec_mini

systemdユニット設定ファイルをコピーします。(`auto_rec_mini.service`)

	$ sudo cp auto_rec_mini.service /etc/systemd/system

systemctlコマンドでサービスを開始します。

	$ sudo systemctl daemon-reload
	$ sudo systemctl enable auto_rec_mini
	$ sudo systemctl start auto_rec_mini

`auto_rec_mini` プロセスは起動し、チューナーを使用する準備ができた状態になります。  
`command server` はポート20001で接続を待ち受けています。
最初にCLIコマンドより `channel scan` を行う必要があります。  

### Clean ###
	$ sudo systemctl stop auto_rec_mini
	$ sudo systemctl disable auto_rec_mini
	$ sudo rm /etc/systemd/system/auto_rec_mini.service
	$ sudo systemctl daemon-reload
	$ sudo make INSTALLDIR=/opt/auto_rec_mini clean
	$ make clean


settings.json
------------

`settings.json`は設定ファイルです。  
`auto_rec_mini` プロセスの起動時に読み込みます。  
環境に合わせてお好みで設定できます。

| item | description |
|:-----|:------------|
| `is_syslog_output` | ログを`syslog`に出力するかどうかを切り替えます。 |
| `command_server_port` | `command server` の待受ポートです。 |
| `channels_json_path` | チャンネルスキャン結果の書き込み/読み込み先パスです。 |
| `rec_reserves_json_path` | 録画予約リストの書き込み/読み込み先パスです。 |
| `rec_results_json_path` | 録画結果リストの書き込み/読み込み先パスです。 |
| `rec_ts_path` | 録画ストリームの保存先パスです。(.m2ts) |
| `dummy_tuner_ts_path` | unused |
| `event_schedule_cache_is_enable` | EPGを有効にするスイッチ。 |
| `event_schedule_cache_start_interval_day` | EPG取得の間隔日。 |
| `event_schedule_cache_start_time` | EPG取得の開始時間。(HH:mm) |
| `event_schedule_cache_timeout_min` | EPG取得タイムアウト時間。(分) |
| `event_schedule_cache_retry_interval_min` | EPG取得リトライ間隔。(分) |
| `event_schedule_cache_data_json_path` | 取得したEPGデータの書き込み/読み込み先パスです。 | 
| `event_schedule_cache_histories_json_path` | EPG取得履歴の書き込み/読み込み先パスです。 |
| `event_name_keywords_json_path` | `event name` 検索のキーワードリストの書き込み/読み込み先パスです。 |
| `extended_event_keywords_json_path` | `extended event` 検索のキーワードリストの書き込み/読み込み先パスです。 |
| `event_name_search_histories_json_path` | `event name` 検索履歴の書き込み/読み込み先パスです。 |
| `extended_event_search_histories_json_path` | `extended event` 検索履歴の書き込み/読み込み先パスです。 |
| `logo_path` | 放送局ロゴの書き込み/読み込み先パスです。 |

### is_syslog_output ###
`syslog facirity` を `user` に設定することで、ログを `/var/log/user.log` に出力できます。  
以下 `/etc/rsyslog.d/50-default.conf` を編集する必要があります。(`ubuntu16.04`の場合)

	9c9
	< *.*;auth,authpriv.none        -/var/log/syslog
	---
	> *.*;auth,authpriv.none,user.none      -/var/log/syslog
	15c15
	< #user.*               -/var/log/user.log
	---
	> user.*                -/var/log/user.log

### event_name_keywords_json_path ###

`m_extended_event_keywords_json_path` に下記形式の json を作成することにより   
EPG取得後に番組名にキーワードが含まれる番組を検索して録画予約を入れます。  
キーワードの数に制限はありません。  

	{
	    "m_event_name_keywords": [
	        "ＸＸＸニュース",
	        "ＸＸＸスポーツ",
	        "ＸＸＸ報道"
	    ]
	}

### extended_event_keywords_json_path ###

`m_extended_event_keywords_json_path` に下記形式の json を作成することにより  
EPG取得後に番組詳細にキーワードが含まれる番組を検索して録画予約を入れます。  
キーワードの数に制限はありません。  

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
端末から`netcat` や `telnet` で接続できます。

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

チャンネルスキャンが開始します。しばらく時間がかかります。

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
設定値や保持するデータの読み込み/書き込みは [`cereal`](https://github.com/USCiLab/cereal) のjsonシリアライザを使用しています。  
(現状DBは使用していません。)


パーサー周りは以下のレポジトリ様を参考にさせていただいています:
* [`libarib25`](https://github.com/stz2012/libarib25)
* [`epgdump`](https://github.com/Piro77/epgdump)
* [`ariblib`](https://github.com/youzaka/ariblib)
* [`eit_txtout_mod`](https://github.com/arairait/eit_txtout_mod)
 
Contribute
------------
This project is still at its beginning. If you can offer suggestions, additional information or help please contact me.

LICENSE
------------
MIT

