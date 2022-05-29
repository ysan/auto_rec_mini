#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TsAribCommon.h"


static const uint32_t g_crc32table [256] = {
	0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
	0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
	0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 
	0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
	
	0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9,
	0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 
	0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
	0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
	
	0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
	0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
	0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 
	0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
	
	0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49,
	0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
	0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 
	0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,

	0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE,
	0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 
	0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
	0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,

	0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 
	0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
	0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
	0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
	
	0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E,
	0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
	0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 
	0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,

	0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
	0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 
	0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686,
	0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,

	0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 
	0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
	0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F,
	0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
	
	0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
	0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
	0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
	0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,

	0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7,
	0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 
	0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F,
	0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
	
	0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 
	0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
	0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F,
	0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
	
	0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
	0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
	0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
	0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,

	0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30,
	0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
	0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088,
	0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,

	0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
	0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
	0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
	0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 

	0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0,
	0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
	0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 
	0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4,
};

static const char *g_pszGenre_lvl1 [] = {
	"ニュース／報道",			// 0x0
	"スポーツ",					// 0x1
	"情報／ワイドショー",		// 0x2
	"ドラマ",					// 0x3
	"音楽",						// 0x4
	"バラエティ",				// 0x5
	"映画",						// 0x6
	"アニメ／特撮",				// 0x7
	"ドキュメンタリー／教養",	// 0x8
	"劇場／公演",				// 0x9
	"趣味／教育",				// 0xa
	"福祉",						// 0xb
	"予備",						// 0xc
	"予備",						// 0xd
	"拡張",						// 0xe
	"その他 ",					// 0xf
};

static const char *g_pszGenre_lvl2 [] = {
	"定時・総合",						// 0x0 0x0
	"天気",								// 0x0 0x1
	"特集・ドキュメント",				// 0x0 0x2
	"政治・国会",						// 0x0 0x3
	"経済・市況",						// 0x0 0x4
	"海外・国際",						// 0x0 0x5
	"解説",								// 0x0 0x6
	"討論・会談",						// 0x0 0x7
	"報道特番",							// 0x0 0x8
	"ローカル・地域",					// 0x0 0x9
	"交通",								// 0x0 0xa
	"",									// 0x0 0xb
	"",									// 0x0 0xc
	"",									// 0x0 0xd
	"",									// 0x0 0xe
	"その他",							// 0x0 0xf

	"スポーツニュース",					// 0x1 0x0
	"野球",								// 0x1 0x1
	"サッカー",							// 0x1 0x2
	"ゴルフ",							// 0x1 0x3
	"その他の球技",						// 0x1 0x4
	"相撲・格闘技",						// 0x1 0x5
	"オリンピック・国際大会",			// 0x1 0x6
	"マラソン・陸上・水泳",				// 0x1 0x7
	"モータースポーツ",					// 0x1 0x8
	"マリン・ウィンタースポーツ",		// 0x1 0x9
	"競馬・公営競技",					// 0x1 0xa
	"",									// 0x1 0xb
	"",									// 0x1 0xc
	"",									// 0x1 0xd
	"",									// 0x1 0xe
	"その他",							// 0x1 0xf

	"芸能・ワイドショー",				// 0x2 0x0
	"ファッション",						// 0x2 0x1
	"暮らし・住まい",					// 0x2 0x2
	"健康・医療",						// 0x2 0x3
	"ショッピング・通販",				// 0x2 0x4
	"グルメ・料理",						// 0x2 0x5
	"イベント",							// 0x2 0x6
	"番組紹介・お知らせ",				// 0x2 0x7
	"",									// 0x2 0x8
	"",									// 0x2 0x9
	"",									// 0x2 0xa
	"",									// 0x2 0xb
	"",									// 0x2 0xc
	"",									// 0x2 0xd
	"",									// 0x2 0xe
	"その他",							// 0x2 0xf

	"国内ドラマ",						// 0x3 0x0
	"海外ドラマ",						// 0x3 0x1
	"時代劇",							// 0x3 0x2
	"",									// 0x3 0x3
	"",									// 0x3 0x4
	"",									// 0x3 0x5
	"",									// 0x3 0x6
	"",									// 0x3 0x7
	"",									// 0x3 0x8
	"",									// 0x3 0x9
	"",									// 0x3 0xa
	"",									// 0x3 0xb
	"",									// 0x3 0xc
	"",									// 0x3 0xd
	"",									// 0x3 0xe
	"その他",							// 0x3 0xf

	"国内ロック・ポップス",				// 0x4 0x0
	"海外ロック・ポップス",				// 0x4 0x1
	"クラシック・オペラ",				// 0x4 0x2
	"ジャズ・フュージョン",				// 0x4 0x3
	"歌謡曲・演歌",						// 0x4 0x4
	"ライブ・コンサート",				// 0x4 0x5
	"ランキング・リクエスト",			// 0x4 0x6
	"カラオケ・のど自慢",				// 0x4 0x7
	"民謡・邦楽",						// 0x4 0x8
	"童謡・キッズ",						// 0x4 0x9
	"民族音楽・ワールドミュージック",	// 0x4 0xa
	"",									// 0x4 0xb
	"",									// 0x4 0xc
	"",									// 0x4 0xd
	"",									// 0x4 0xe
	"その他",							// 0x4 0xf

	"クイズ",							// 0x5 0x0
	"ゲーム",							// 0x5 0x1
	"トークバラエティ",					// 0x5 0x2
	"お笑い・コメディ",					// 0x5 0x3
	"音楽バラエティ",					// 0x5 0x4
	"旅バラエティ",						// 0x5 0x5
	"料理バラエティ",					// 0x5 0x6
	"",									// 0x5 0x7
	"",									// 0x5 0x8
	"",									// 0x5 0x9
	"",									// 0x5 0xA
	"",									// 0x5 0xb
	"",									// 0x5 0xc
	"",									// 0x5 0xd
	"",									// 0x5 0xe
	"その他",							// 0x5 0xf

	"洋画",								// 0x6 0x0
	"邦画",								// 0x6 0x1
	"アニメ",							// 0x6 0x2
	"",									// 0x6 0x3
	"",									// 0x6 0x4
	"",									// 0x6 0x5
	"",									// 0x6 0x6
	"",									// 0x6 0x7
	"",									// 0x6 0x8
	"",									// 0x6 0x9
	"",									// 0x6 0xa
	"",									// 0x6 0xb
	"",									// 0x6 0xc
	"",									// 0x6 0xd
	"",									// 0x6 0xe
	"その他",							// 0x6 0xf

	"国内アニメ",						// 0x7 0x0
	"海外アニメ",						// 0x7 0x1
	"特撮",								// 0x7 0x2
	"",									// 0x7 0x3
	"",									// 0x7 0x4
	"",									// 0x7 0x5
	"",									// 0x7 0x6
	"",									// 0x7 0x7
	"",									// 0x7 0x8
	"",									// 0x7 0x9
	"",									// 0x7 0xa
	"",									// 0x7 0xb
	"",									// 0x7 0xc
	"",									// 0x7 0xd
	"",									// 0x7 0xe
	"その他",							// 0x7 0xf

	"社会・時事",						// 0x8 0x0
	"歴史・紀行",						// 0x8 0x1
	"自然・動物・環境",					// 0x8 0x2
	"宇宙・科学・医学",					// 0x8 0x3
	"カルチャー・伝統文化",				// 0x8 0x4
	"文学・文芸",						// 0x8 0x5
	"スポーツ",							// 0x8 0x6
	"ドキュメンタリー全般",				// 0x8 0x7
	"インタビュー・討論",				// 0x8 0x8
	"",									// 0x8 0x9
	"",									// 0x8 0xa
	"",									// 0x8 0xb
	"",									// 0x8 0xc
	"",									// 0x8 0xd
	"",									// 0x8 0xe
	"その他",							// 0x8 0xf

	"現代劇・新劇",						// 0x9 0x0
	"ミュージカル",						// 0x9 0x1
	"ダンス・バレエ",					// 0x9 0x2
	"落語・演芸",						// 0x9 0x3
	"歌舞伎・古典",						// 0x9 0x4
	"",									// 0x9 0x5
	"",									// 0x9 0x6
	"",									// 0x9 0x7
	"",									// 0x9 0x8
	"",									// 0x9 0x9
	"",									// 0x9 0xa
	"",									// 0x9 0xb
	"",									// 0x9 0xc
	"",									// 0x9 0xd
	"",									// 0x9 0xe
	"その他",							// 0x9 0xf

	"旅・釣り・アウトドア",				// 0xa 0x0
	"園芸・ペット・手芸",				// 0xa 0x1
	"音楽・美術・工芸",					// 0xa 0x2
	"囲碁・将棋",						// 0xa 0x3
	"麻雀・パチンコ",					// 0xa 0x4
	"車・オートバイ",					// 0xa 0x5
	"コンピュータ・ＴＶゲーム",			// 0xa 0x6
	"会話・語学",						// 0xa 0x7
	"幼児・小学生",						// 0xa 0x8
	"中学生・高校生",					// 0xa 0x9
	"大学生・受験",						// 0xa 0xa
	"生涯教育・資格",					// 0xa 0xb
	"教育問題",							// 0xa 0xc
	"",									// 0xa 0xd
	"",									// 0xa 0xe
	"その他",							// 0xa 0xf

	"高齢者",							// 0xb 0x0
	"障害者",							// 0xb 0x1
	"社会福祉",							// 0xb 0x2
	"ボランティア",						// 0xb 0x3
	"手話",								// 0xb 0x4
	"文字（字幕）",						// 0xb 0x5
	"音声解説",							// 0xb 0x6
	"",									// 0xb 0x7
	"",									// 0xb 0x8
	"",									// 0xb 0x9
	"",									// 0xb 0xa
	"",									// 0xb 0xb
	"",									// 0xb 0xc
	"",									// 0xb 0xd
	"",									// 0xb 0xe
	"その他",							// 0xb 0xf

	"",									// 0xc 0x0
	"",									// 0xc 0x1
	"",									// 0xc 0x2
	"",									// 0xc 0x3
	"",									// 0xc 0x4
	"",									// 0xc 0x5
	"",									// 0xc 0x6
	"",									// 0xc 0x7
	"",									// 0xc 0x8
	"",									// 0xc 0x9
	"",									// 0xc 0xa
	"",									// 0xc 0xb
	"",									// 0xc 0xc
	"",									// 0xc 0xd
	"",									// 0xc 0xe
	"",									// 0xc 0xf

	"",									// 0xd 0x0
	"",									// 0xd 0x1
	"",									// 0xd 0x2
	"",									// 0xd 0x3
	"",									// 0xd 0x4
	"",									// 0xd 0x5
	"",									// 0xd 0x6
	"",									// 0xd 0x7
	"",									// 0xd 0x8
	"",									// 0xd 0x9
	"",									// 0xd 0xa
	"",									// 0xd 0xb
	"",									// 0xd 0xc
	"",									// 0xd 0xd
	"",									// 0xd 0xe
	"",									// 0xd 0xf

	"BS/地上デジタル放送用番組付属情報",// 0xe 0x0
	"広帯域 CSデジタル放送用拡張",		// 0xe 0x1
	"衛星デジタル音声放送用拡張",		// 0xe 0x2
	"サーバー型番組付属情報",			// 0xe 0x3
	"IP放送用番組付属情報",				// 0xe 0x4
	"",									// 0xe 0x5
	"",									// 0xe 0x6
	"",									// 0xe 0x7
	"",									// 0xe 0x8
	"",									// 0xe 0x9
	"",									// 0xe 0xa
	"",									// 0xe 0xb
	"",									// 0xe 0xc
	"",									// 0xe 0xd
	"",									// 0xe 0xe
	"",									// 0xe 0xf

	"",									// 0xf 0x0
	"",									// 0xf 0x1
	"",									// 0xf 0x2
	"",									// 0xf 0x3
	"",									// 0xf 0x4
	"",									// 0xf 0x5
	"",									// 0xf 0x6
	"",									// 0xf 0x7
	"",									// 0xf 0x8
	"",									// 0xf 0x9
	"",									// 0xf 0xa
	"",									// 0xf 0xb
	"",									// 0xf 0xc
	"",									// 0xF 0xd
	"",									// 0xF 0xe
	"その他",							// 0xf 0xf
};

static const char *g_pszVideoComponentType [] = {
	"480i",		// 0x0
	"予約",		// 0x1
	"予約",		// 0x2
	"予約",		// 0x3
	"予約",		// 0x4
	"予約",		// 0x5
	"予約",		// 0x6
	"予約",		// 0x7
	"予約",		// 0x8
	"2160p",	// 0x9
	"480p",		// 0xa
	"1080i",	// 0xb
	"720p",		// 0xc
	"240p",		// 0xd
	"1080p",	// 0xe
};

static const char *g_pszVideoRatio [] = {
	"予約",							// 0x0
	"4:3",							// 0x1
	"16:9 (with panorama vector)",	// 0x2
	"16:9",							// 0x3
	"16:9 wider",					// 0x4
};

static const char *g_pszAudioComponentType [] = {
	"予約",										// 0x0
	"シングルモノ",								// 0x1
	"デュアルモノ",								// 0x2
	"ステレオ",									// 0x3
	"2/1モード",								// 0x4
	"3/0モード",								// 0x5
	"2/2モード",								// 0x6
	"3/1モード",								// 0x7
	"3/2モード (5.1ch)",						// 0x8
	"3/2+LFE (5.1ch + low frequency effect)",	// 0x9
};

static const char *g_pszAudioSamplingRate [] = {
	"予約",		// 0x0
	"16kHz",	// 0x1
	"22kHz",	// 0x2
	"24kHz",	// 0x3
	"予約",		// 0x4
	"32kHz",	// 0x5
	"44kHz",	// 0x6
	"48kHz",	// 0x7
	"96kHz",	// 0x8
	"192kHz",	// 0x9
};

static const char *g_pszAudioQuality [] = {
	"予約",		// 0x0
	"モード1",	// 0x1
	"モード2",	// 0x2
	"モード3",	// 0x3
};

const uint8_t g_commonPaletCLUT [128][4] = { // extern
	// CLUT共通固定色 64色 + alpha入りも同じように並べたもの
	{0  , 0  , 0  , 255}, // 0  黒
	{255, 0  , 0  , 255}, // 1  赤
	{0  , 255, 0  , 255}, // 2  緑 
	{255, 255, 0  , 255}, // 3  黄
	{0  , 0  , 255, 255}, // 4  青
	{255, 0  , 255, 255}, // 5  マゼ 
	{0  , 255, 255, 255}, // 6  シア 
	{255, 255, 255, 255}, // 7  白
	{0  , 0  , 0  , 0  }, // 8  透明
	{170, 0  , 0  , 255},
	{0  , 170, 0  , 255},
	{170, 170, 0  , 255},
	{0  , 0  , 170, 255},
	{170, 0  , 170, 255},
	{0  , 170, 170, 255},
	{170, 170, 170, 255},
	{0  , 0  , 85 , 255},
	{0  , 85 , 0  , 255},
	{0  , 85 , 85 , 255},
	{0  , 85 , 170, 255},
	{0  , 85 , 255, 255},
	{0  , 170, 85 , 255},
	{0  , 170, 255, 255},
	{0  , 255, 85 , 255},
	{0  , 255, 170, 255},
	{85 , 0  , 0  , 255},
	{85 , 0  , 85 , 255},
	{85 , 0  , 170, 255},
	{85 , 0  , 255, 255},
	{85 , 85 , 0  , 255},
	{85 , 85 , 85 , 255},
	{85 , 85 , 170, 255},
	{85 , 85 , 255, 255},
	{85 , 170, 0  , 255},
	{85 , 170, 85 , 255},
	{85 , 170, 170, 255},
	{85 , 170, 255, 255},
	{85 , 255, 0  , 255},
	{85 , 255, 85 , 255},
	{85 , 255, 170, 255},
	{85 , 255, 255, 255},
	{170, 0  , 85 , 255},
	{170, 0  , 255, 255},
	{170, 85 , 0  , 255},
	{170, 85 , 85 , 255},
	{170, 85 , 170, 255},
	{170, 85 , 255, 255},
	{170, 170, 85 , 255},
	{170, 170, 255, 255},
	{170, 255, 0  , 255},
	{170, 255, 85 , 255},
	{170, 255, 170, 255},
	{170, 255, 255, 255},
	{255, 0  , 85 , 255},
	{255, 0  , 255, 255},
	{255, 85 , 0  , 255},
	{255, 85 , 85 , 255},
	{255, 85 , 170, 255},
	{255, 85 , 255, 255},
	{255, 170, 0  , 255},
	{255, 170, 85 , 255},
	{255, 170, 170, 255},
	{255, 170, 255, 255},
	{255, 255, 85 , 255},
	{255, 255, 255, 255},
	{0  , 0  , 0  , 128}, // 64  黒   以下alpha入り
	{255, 0  , 0  , 128}, // 65  赤
	{0  , 255, 0  , 128}, // 66  緑
	{255, 255, 0  , 128}, // 67  黄
	{0  , 0  , 255, 128}, // 68  青
	{255, 0  , 255, 128}, // 69  マゼンタ
	{0  , 255, 255, 128}, // 70  シアン
	{255, 255, 255, 128}, // 71  白
	{170, 0  , 0  , 128},
	{0  , 170, 0  , 128},
	{170, 170, 0  , 128},
	{0  , 0  , 170, 128},
	{170, 0  , 170, 128},
	{0  , 170, 170, 128},
	{170, 170, 170, 128},
	{0  , 0  , 85 , 128},
	{0  , 85 , 0  , 128},
	{0  , 85 , 85 , 128},
	{0  , 85 , 170, 128},
	{0  , 85 , 255, 128},
	{0  , 170, 85 , 128},
	{0  , 170, 255, 128},
	{0  , 255, 85 , 128},
	{0  , 255, 170, 128},
	{85 , 0  , 0  , 128},
	{85 , 0  , 85 , 128},
	{85 , 0  , 170, 128},
	{85 , 0  , 255, 128},
	{85 , 85 , 0  , 128},
	{85 , 85 , 85 , 128},
	{85 , 85 , 170, 128},
	{85 , 85 , 255, 128},
	{85 , 170, 0  , 128},
	{85 , 170, 85 , 128},
	{85 , 170, 170, 128},
	{85 , 170, 255, 128},
	{85 , 255, 0  , 128},
	{85 , 255, 85 , 128},
	{85 , 255, 170, 128},
	{85 , 255, 255, 128},
	{170, 0  , 85 , 128},
	{170, 0  , 255, 128},
	{170, 85 , 0  , 128},
	{170, 85 , 85 , 128},
	{170, 85 , 170, 128},
	{170, 85 , 255, 128},
	{170, 170, 85 , 128},
	{170, 170, 255, 128},
	{170, 255, 0  , 128},
	{170, 255, 85 , 128},
	{170, 255, 170, 128},
	{170, 255, 255, 128},
	{255, 0  , 85 , 128},
	{255, 0  , 255, 128},
	{255, 85 , 0  , 128},
	{255, 85 , 85 , 128},
	{255, 85 , 170, 128},
	{255, 85 , 255, 128},
	{255, 170, 0  , 128},
	{255, 170, 85 , 128},
	{255, 170, 170, 128},
	{255, 170, 255, 128},
	{255, 255, 85 , 128}, // 127
};

void CTsAribCommon::getStrEpoch (time_t tx, const char *format, char *pszout, int outsize)
{
	struct tm *tl;
	struct tm stm;
	tl = localtime_r (&tx, &stm); 
	strftime(pszout, outsize - 1, format, tl);
}

void CTsAribCommon::getStrSecond (int second, char *pszout, int outsize)
{
	int hh = second / 3600;
	int mm = (second % 3600) / 60;
	int ss = (second % 3600) % 60;
	snprintf (pszout, outsize, "%02d:%02d:%02d", hh, mm, ss);
}

uint32_t CTsAribCommon::crc32(const uint8_t *buff, size_t length)
{
	uint32_t crc = 0xffffffff;
	uint8_t *p = (uint8_t*)buff;

	while (p < (buff + length)) {
		crc = (crc << 8) ^ g_crc32table [((crc >> 24) ^ p[0]) & 0xff];
		p += 1;
	}

	return crc ;
}

time_t CTsAribCommon::getEpochFromMJD (const uint8_t *mjd)
{
	if (!mjd) {
		return 0;
	}

	int tnum,yy,mm,dd;
	char buf[10];
	time_t l_time ;
	time_t end_time ;
	struct tm tl ;
	struct tm *endtl ;
	char cendtime[32];
	char cmjd[32];

	tnum = (mjd[0] & 0xFF) << 8 | (mjd[1] & 0xFF);

	yy = (tnum - 15078.2) / 365.25;
	mm = ((tnum - 14956.1) - (int)(yy * 365.25)) / 30.6001;
	dd = (tnum - 14956) - (int)(yy * 365.25) - (int)(mm * 30.6001);

	if (mm == 14 || mm == 15) {
		yy += 1;
		mm = mm - 1 - (1 * 12);
	} else {
		mm = mm - 1;
	}

	tl.tm_year = yy;
	tl.tm_mon = mm - 1;
	tl.tm_mday = dd;
	memset(buf, '\0', sizeof(buf));
	sprintf(buf, "%x", mjd[2]);
	tl.tm_hour = atoi(buf);
	memset(buf, '\0', sizeof(buf));
	sprintf(buf, "%x", mjd[3]);
	tl.tm_min = atoi(buf);
	memset(buf, '\0', sizeof(buf));
	sprintf(buf, "%x", mjd[4]);
	tl.tm_sec = atoi(buf);

	tl.tm_wday = 0;
	tl.tm_isdst = 0;
	tl.tm_yday = 0;

	l_time = mktime(&tl);
	return l_time;
}

int CTsAribCommon::getSecFromBCD (const uint8_t *bcd)
{
	if (!bcd) {
		return -1;
	}

	int hh,mm,ss;
	char buf[24];

	if ((bcd[0] == 0xFF) && (bcd[1] == 0xFF) && (bcd[2] == 0xFF)) {
		// 終了未定
		hh = mm = ss = 0;
		ss = -1;
	} else {
		memset(buf, '\0', sizeof(buf));
		sprintf(buf, "%x", bcd[0]);
		hh = atoi(buf)*3600;
		memset(buf, '\0', sizeof(buf));
		sprintf(buf, "%x", bcd[1]);
		mm = atoi(buf)*60;
		memset(buf, '\0', sizeof(buf));
		sprintf(buf, "%x", bcd[2]);
		ss = atoi(buf);
	}

	return hh+mm+ss;
}

const char* CTsAribCommon::getGenre_lvl1 (uint8_t genre)
{
	if (genre < 0x00 || genre > 0x0f) {
		return "ジャンル指定なし";
	} else {
		return g_pszGenre_lvl1 [genre];
	}
}

const char* CTsAribCommon::getGenre_lvl2 (uint8_t genre)
{
	// 0x00 - 0xff
	const char *str = g_pszGenre_lvl2 [genre];
	if (!str || (int)strlen(str) == 0) {
		return "未定義";
	} else {
		return str;
	}
}

const char* CTsAribCommon::getVideoComponentType (uint8_t type)
{
	uint8_t t = (type >> 4) & 0x0f;
	if (t < 0x00 || t > 0x0e) {
		return "未定義";
	} else {
		return g_pszVideoComponentType [t];
	}
}

const char* CTsAribCommon::getVideoRatio (uint8_t type)
{
	uint8_t r = type & 0x0f;
	if (r < 0x00 || r > 0x04) {
		return "未定義";
	} else {
		return g_pszVideoRatio [r];
	}
}

const char* CTsAribCommon::getAudioComponentType (uint8_t type)
{
	if (type < 0x00 || type > 0x09) {
		return "未定義";
	} else {
		return g_pszAudioComponentType [type];
	}
}

const char* CTsAribCommon::getAudioSamplingRate (uint8_t samplingRate)
{
	if (samplingRate < 0x00 || samplingRate > 0x09) {
		return "未定義";
	} else {
		return g_pszAudioSamplingRate [samplingRate];
	}
}

const char* CTsAribCommon::getAudioQuality (uint8_t quality)
{
	if (quality < 0x00 || quality > 0x03) {
		return "未定義";
	} else {
		return g_pszAudioQuality [quality];
	}
}

uint16_t CTsAribCommon::freqKHz2pysicalCh (uint32_t freqKHz)
{
	return ((freqKHz - 473143) / 6000) + 13;
}

uint32_t CTsAribCommon::pysicalCh2freqKHz (uint16_t ch)
{
	return ((ch - 13) * 6000) + 473143;
}
