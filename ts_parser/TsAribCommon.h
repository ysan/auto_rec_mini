#ifndef _TS_ARIB_COMMON_H_
#define _TS_ARIB_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "Defs.h"
#include "Utils.h"


#define TS_PACKET_LEN					(188)
#define SYNC_BYTE						(0x47)
#define TS_HEADER_LEN					(4)
#define SECTION_HEADER_LEN				(8)
#define SECTION_HEADER_FIX_LEN			(5) // after section_length
#define SECTION_SHORT_HEADER_LEN		(3) // section_length include, before
#define SECTION_CRC32_LEN				(4)

#define MAXSECLEN						(4096)

// pid
#define PID_PAT							(0x0000)
#define PID_CAT							(0x0001)
#define PID_TSDT						(0x0002)
#define PID_NIT							(0x0010)
#define PID_SDT							(0x0011)
#define PID_BAT							(0x0011)
#define PID_EIT_H						(0x0012)
#define PID_RST							(0x0013)
#define PID_TOT							(0x0014)
#define PID_TDT							(0x0014)
#define PID_DCT							(0x0017)
#define PID_DIT							(0x001e)
#define PID_SIT							(0x001f)
#define PID_PCAT						(0x0022)
#define PID_SDTT						(0x0023)
#define PID_W_SDTT						(0x0023)
#define PID_BIT							(0x0024)
#define PID_NBIT						(0x0025)
#define PID_LDT							(0x0025)
#define PID_EIT_M						(0x0026)
#define PID_EIT_L						(0x0027)
#define PID_S_SDTT						(0x0028)
#define PID_CDT							(0x0029)
#define PID_ETT							(0x0030)
#define PID_EMT							(0x0030)
#define PID_SLDT						(0x0036)
#define PID_NULL						(0x1fff)

// table_id
#define TBLID_PAT						(0x00)
#define TBLID_CAT						(0x01)
#define TBLID_PMT						(0x02)
#define TBLID_TSDT						(0x03)
#define TBLID_NIT_A						(0x40) // actual
#define TBLID_NIT_O						(0x41) // other
#define TBLID_SDT_A						(0x42) // actual
#define TBLID_SDT_O						(0x46) // other
#define TBLID_BAT						(0x4a)
#define TBLID_EIT_PF_A					(0x4e) // actual
#define TBLID_EIT_PF_O					(0x4f) // other
#define TBLID_EIT_SCH_A					(0x50) // actual
#define TBLID_EIT_SCH_A_EXT				(0x58) // actual extend
#define TBLID_EIT_SCH_O					(0x60) // other
#define TBLID_EIT_SCH_O_EXT				(0x68) // other extend
#define TBLID_TDT						(0x70)
#define TBLID_RST						(0x71)
#define TBLID_ST						(0x72)
#define TBLID_TOT						(0x73)
#define TBLID_DIT						(0x7e)
#define TBLID_SIT						(0x7f)
#define TBLID_ECM						(0x82)
#define TBLID_EMM_NARROW				(0x84)
#define TBLID_EMM_INDV_CMN				(0x85)
#define TBLID_SLDT_A					(0xb2) // actual
#define TBLID_SLDT_O					(0xb6) // other
#define TBLID_ETT_A						(0xa2) // actual
#define TBLID_ETT_O						(0xa3) // other
#define TBLID_EMT_G						(0xa4)
#define TBLID_EMT_N						(0xa5)
#define TBLID_EMT_D						(0xa7)
#define TBLID_SDTT						(0xc3)
#define TBLID_BIT						(0xc4)
#define TBLID_NBIT_MSG					(0xc5)
#define TBLID_NBIT_REF					(0xc6)
#define TBLID_LDT						(0xc7)
#define TBLID_CDT						(0xc8)

#define UHF_PHYSICAL_CHANNEL_MIN		(13)	// UHFの最小物理CH
#define UHF_PHYSICAL_CHANNEL_MAX		(62)	// UHFの最大物理CH


typedef enum {
	EN_AREA_CODE_COMMON				= 0x34d,
	EN_AREA_CODE_KANTO				= 0x5a5,
	EN_AREA_CODE_CHUKYO				= 0x72a,
	EN_AREA_CODE_KINKI				= 0x8d5,
	EN_AREA_CODE_TOTTORI_SHIMANE	= 0x699,
	EN_AREA_CODE_OKAYAMA_KAGAWA		= 0x553,
	EN_AREA_CODE_HOKKAIDO			= 0x16b,
	EN_AREA_CODE_AOMORI				= 0x467,
	EN_AREA_CODE_IWATE				= 0x5d4,
	EN_AREA_CODE_MIYAGI				= 0x758,
	EN_AREA_CODE_AKITA				= 0xac6,
	EN_AREA_CODE_YAMAGATA			= 0xe4c,
	EN_AREA_CODE_FUKUSHIMA			= 0x1ae,
	EN_AREA_CODE_IBARAKI			= 0xc69,
	EN_AREA_CODE_TOCHIGI			= 0xe38,
	EN_AREA_CODE_GUNMA				= 0x98b,
	EN_AREA_CODE_SAITAMA			= 0x64b,
	EN_AREA_CODE_CHIBA				= 0x1c7,
	EN_AREA_CODE_TOKYO				= 0xaac,
	EN_AREA_CODE_KANAGAWA			= 0x56c,
	EN_AREA_CODE_NIIGATA			= 0x4ce,
	EN_AREA_CODE_TOYAMA				= 0x539,
	EN_AREA_CODE_ISHIKAWA			= 0x6a6,
	EN_AREA_CODE_FUKUI				= 0x92d,
	EN_AREA_CODE_YAMANASHI			= 0xd4a,
	EN_AREA_CODE_NAGANO				= 0x9d2,
	EN_AREA_CODE_GIFU				= 0xa65,
	EN_AREA_CODE_SHIZUOKA			= 0xa5a,
	EN_AREA_CODE_AICHI				= 0x966,
	EN_AREA_CODE_MIE				= 0x2dc,
	EN_AREA_CODE_SHIGA				= 0xce4,
	EN_AREA_CODE_KYOTO				= 0x59a,
	EN_AREA_CODE_OSAKA				= 0xcb2,
	EN_AREA_CODE_HYOGO				= 0x674,
	EN_AREA_CODE_NARA				= 0xa93,
	EN_AREA_CODE_WAKAYAMA			= 0x396,
	EN_AREA_CODE_TOTTORI			= 0xd23,
	EN_AREA_CODE_SHIMANE			= 0x31b,
	EN_AREA_CODE_OKAYAMA			= 0x2b5,
	EN_AREA_CODE_HIROSHIMA			= 0xb31,
	EN_AREA_CODE_YAMAGUCHI			= 0xb98,
	EN_AREA_CODE_TOKUSHIMA			= 0xe62,
	EN_AREA_CODE_KAGAWA				= 0x9b4,
	EN_AREA_CODE_EHIME				= 0x19d,
	EN_AREA_CODE_KOCHI				= 0x2e3,
	EN_AREA_CODE_FUKUOKA			= 0x62d,
	EN_AREA_CODE_SAGA				= 0x959,
	EN_AREA_CODE_NAGASAKI			= 0xa2b,
	EN_AREA_CODE_KUMAMOTO			= 0x8a7,
	EN_AREA_CODE_OITA				= 0xc8d,
	EN_AREA_CODE_MIYAZAKI			= 0xd1c,
	EN_AREA_CODE_KAGOSHIMA			= 0xd45,
	EN_AREA_CODE_OKINAWA			= 0x372,
	EN_AREA_CODE_UNKNOWN			= 0xffff
} EN_AREA_CODE;


typedef enum
{
	EN_SERVICE_TYPE_DIGITAL_TV				= 0x01, // デジタルTV放送
	EN_SERVICE_TYPE_DIGITAL_RADIO			= 0x02, // デジタルラジオ放送

	EN_SERVICE_TYPE_SPECIAL_TV				= 0xA1, // デジタル放送（臨時）
	EN_SERVICE_TYPE_SPECIAL_RADIO			= 0xA2, // デジタルラジオ放送（臨時）
	EN_SERVICE_TYPE_SPECIAL_DATA			= 0xA3, // データ放送（臨時)

	EN_SERVICE_TYPE_ENGINEERING_DOWNLOAD	= 0xA4, // エンジニアリングダウンロード

	EN_SERVICE_TYPE_PROMOTION_TV			= 0xA5, // プロモーション映像サービス
	EN_SERVICE_TYPE_PROMOTION_RADIO			= 0xA6, // プロモーション音声サービス
	EN_SERVICE_TYPE_PROMOTION_DATA			= 0xA7, // プロモーションデータサービス

	EN_SERVICE_TYPE_PRE_ACCUMULATED			= 0xA8, // 事前蓄積サービス
	EN_SERVICE_TYPE_ACCUMULATED				= 0xA9, // 蓄積サービス
	EN_SERVICE_TYPE_DATA					= 0xC0, // データ放送
	EN_SERVICE_TYPE_BOOKMARK				= 0xAA, // ブックマーク一覧データサービス
	EN_SERVICE_TYPE_4K_TV					= 0XAD, // 4KTV放送

} EN_SERVICE_TYPE;


typedef struct _ts_header {

	void parse (const uint8_t* p_src) {
		if (!p_src) {
			return;
		}
		sync                         =   *(p_src+0);
		transport_error_indicator    =  (*(p_src+1) >> 7) & 0x01;
		payload_unit_start_indicator =  (*(p_src+1) >> 6) & 0x01;
		transport_priority           =  (*(p_src+1) >> 5) & 0x01;
		pid                          = ((*(p_src+1) & 0x1f) << 8) | *(p_src+2);
		transport_scrambling_control =  (*(p_src+3) >> 6) & 0x03;
		adaptation_field_control     =  (*(p_src+3) >> 4) & 0x03;
		continuity_counter           =   *(p_src+3)       & 0x0f;
	}

	void dump (void) const {
		_UTL_LOG_I (
			"TsHeader: sync:[0x%02x] trans_err:[0x%02x] start_ind:[0x%02x] prio:[0x%02x] pid:[0x%04x] scram:[0x%02x] adap:[0x%02x] cont:[0x%02x]\n",
			sync,
			transport_error_indicator,
			payload_unit_start_indicator,
			transport_priority,
			pid,
			transport_scrambling_control,
			adaptation_field_control,
			continuity_counter
		);
	}

	uint8_t sync;							//  0- 7 :  8 bits
	uint8_t transport_error_indicator;		//  8- 8 :  1 bit
	uint8_t payload_unit_start_indicator;	//  9- 9 :  1 bit
	uint8_t transport_priority;				// 10-10 :  1 bits
	uint16_t pid;							// 11-23 : 13 bits
	uint8_t transport_scrambling_control;	// 24-25 :  2 bits
	uint8_t adaptation_field_control;		// 26-27 :  2 bits
	uint8_t continuity_counter;				// 28-31 :  4 bits

} TS_HEADER;

typedef struct _section_header {

	void dump (void) const {
		if (section_syntax_indicator == 0) {
			_UTL_LOG_I (
				"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d]\n",
				table_id,
				section_syntax_indicator,
				private_indicator,
				section_length
			);
		} else {
			_UTL_LOG_I (
				"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d] tbl_ext:[0x%04x] ver:[0x%02x] next:[0x%02x] num:[0x%02x] last:[0x%02x]\n",
				table_id,
				section_syntax_indicator,
				private_indicator,
				section_length,
				table_id_extension,
				version_number,
				current_next_indicator,
				section_number,
				last_section_number
			);
		}
	}

	uint8_t table_id;						//  0- 7 :  8 bits
	uint8_t section_syntax_indicator;		//  8- 8 :  1 bit
	uint8_t private_indicator;				//  9- 9 :  1 bit
	uint8_t reserved_1;						// 10-11 :  2 bits
	uint16_t section_length;				// 12-23 : 12 bits
	uint16_t table_id_extension;			// 24-39 : 16 bits
	uint8_t reserved_2;						// 40-41 :  2 bits
	uint8_t version_number;					// 42-46 :  5 bits
	uint8_t current_next_indicator;			// 47-47 :  1 bit
	uint8_t section_number;					// 48-55 :  8 bits
	uint8_t last_section_number;			// 56-63 :  8 bits

} ST_SECTION_HEADER;


class CTsAribCommon {
public:
	static void getStrEpoch (time_t tx, const char *format, char *pszout, int outsize);
	static void getStrSecond (int second, char *pszout, int outsize);


	// ARIB specific
	static time_t getEpochFromMJD (const uint8_t *mjd);
	static int getSecFromBCD (const uint8_t *bcd);
	static const char* getGenre_lvl1 (uint8_t genre);
	static const char* getGenre_lvl2 (uint8_t genre);
	static const char* getVideoComponentType (uint8_t type);
	static const char* getVideoRatio (uint8_t type);
	static const char* getAudioComponentType (uint8_t type);
	static const char* getAudioSamplingRate (uint8_t samplingRate);
	static const char* getAudioQuality (uint8_t quality);
	static uint16_t freqKHz2ch (uint32_t freqKHz);
	static uint32_t ch2freqKHz (uint16_t ch);


};


#endif
