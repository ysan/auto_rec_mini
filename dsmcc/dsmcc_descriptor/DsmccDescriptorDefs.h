#ifndef _DSMCC_DESCRIPTOR_DEFS_H_
#define _DSMCC_DESCRIPTOR_DEFS_H_


#include "Utils.h"

/*
#include "ShortEventDescriptor.h"
#include "ExtendedEventDescriptor.h"
#include "ComponentDescriptor.h"
#include "AudioComponentDescriptor.h"
#include "SeriesDescriptor.h"
#include "ContentDescriptor.h"
#include "ServiceListDescriptor.h"
#include "SatelliteDeliverySystemDescriptor.h"
#include "TerrestrialDeliverySystemDescriptor.h"
#include "TSInformationDescriptor.h"
#include "ServiceDescriptor.h"
#include "SIParameterDescriptor.h"
#include "NetworkNameDescriptor.h"
#include "CAIdentifierDescriptor.h"
#include "ConditionalAccessDescriptor.h"
#include "StreamIdentifierDescriptor.h"


#define DESC_TAG__NETWORK_NAME_DESCRIPTOR					(0x40) // ネットワーク名記述子
#define DESC_TAG__SERVICE_LIST_DESCRIPTOR					(0x41) // サービスリスト記述子
#define DESC_TAG__STUFFING_DESCRIPTOR						(0x42) // スタッフ記述子
#define DESC_TAG__SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR		(0x43) // 衛星分配システム記述子
#define DESC_TAG__CABLE_DELIVERY_SYSTEM_DESCRIPTOR			(0x44) // 有線分配システム記述子
#define DESC_TAG__BOUQET_NAME_DESCRIPTOR					(0x47) // ブーケ名記述子
#define DESC_TAG__SERVICE_DESCRIPTOR						(0x48) // サービス記述子
#define DESC_TAG__COUNTRY_AVAILABILITY_DESCRIPTOR			(0x49) // 国別受信可否記述子
#define DESC_TAG__LINKAGE_DESCRIPTOR						(0x4a) // リンク記述子
#define DESC_TAG__NVOD_REFERENCE_DESCRIPTOR					(0x4b) // NVOD基準サービス記述子
#define DESC_TAG__TIME_SHIFTED_SERVICE_DESCRIPTOR			(0x4c) // タイムシフトサービス記述子
#define DESC_TAG__SHORT_EVENT_DESCRIPTOR					(0x4d) // 短形式イベント記述子
#define DESC_TAG__EXTENDED_EVENT_DESCRIPTOR					(0x4e) // 拡張形式イベント記述子
#define DESC_TAG__TIME_SHIFTED_EVENT_DESCRIPTOR				(0x4f) // タイムシフトイベント記述子

#define DESC_TAG__COMPONENT_DESCRIPTOR						(0x50) // コンポーネント記述子
#define DESC_TAG__MOSAIC_DESCRIPTOR							(0x51) // モザイク記述子
#define DESC_TAG__STREAM_IDENTIFIER_DESCRIPTOR				(0x52) // ストリーム識別記述子
#define DESC_TAG__CA_IDENTIFIER_DESCRIPTOR					(0x53) // CA識別記述子
#define DESC_TAG__CONTENT_DESCRIPTOR						(0x54) // コンテント記述子
#define DESC_TAG__PARENTAL_RATING_DESCRIPTOR				(0x55) // パレンタルレート記述子
#define DESC_TAG__TELETEXT_DESCRIPTOR						(0x56) // 
#define DESC_TAG__TELEPHONE_DESCRIPTOR						(0x57) // 
#define DESC_TAG__LOCAL_TIME_OFFSET_DESCRIPTOR				(0x58) // ローカル時間オフセット記述子
#define DESC_TAG__SUBTITLING_DESCRIPTOR						(0x59) // 
#define DESC_TAG__TERRESTRIAL_DLIVRERY_SYSTEM_DESCRIPTOR	(0x5a) // 
#define DESC_TAG__MULTILINGUAL_NETWORK_NAME_DESCRIPTOR		(0x5b) // 
#define DESC_TAG__MULTILINGUAL_BOUQET_NAME_DESCRIPTOR		(0x5c) // 
#define DESC_TAG__MULTILINGUAL_SERVICE_NAME_DESCRIPTOR		(0x5d) // 
#define DESC_TAG__MULTILINGUAL_COMPONENT_DESCRIPTOR			(0x5e) // 
#define DESC_TAG__PRIVATE_DATA_SPECIFIER_DESCRIPTOR			(0x5f) // 

#define DESC_TAG__SERVICE_MOVE_DESCRIPTOR					(0x60) // 
#define DESC_TAG__SHORT_SMOOTHING_BUFFER_DESCRIPTOR			(0x61) // 
#define DESC_TAG__FREQUENCY_LIST_DESCRIPTOR					(0x62) // 
#define DESC_TAG__PARTIAL_TRANSPORT_STREAM_DESCRIPTOR		(0x63) // パーシャルTS記述子
#define DESC_TAG__DATA_BROADCAST_DESCRIPTOR					(0x64) // 
#define DESC_TAG__CA_SYSTEM_DESCRIPTOR						(0x65) // 
#define DESC_TAG__DATA_BROADCAST_ID_DESCRIPTOR				(0x66) // 

#define DESC_TAG__TRANSCODE_MODE_REGISTRATION_DESCRIPTOR	(0x05) // 
#define DESC_TAG__REGISTRATION_DESCRIPTOR					(0x05) // 
#define DESC_TAG__DTCP_DESCRIPTOR							(0x88) // 

#define DESC_TAG__CONDITIONAL_ACCESS_DESCRIPTOR				(0x09) // 限定受信方式記述子
#define DESC_TAG__COPYRIGHT_DESCRIPTOR						(0x0d) // 著作権記述子

#define DESC_TAG__DM_OFFSET_TIME_DESCRIPTOR					(0x80) // 
#define DESC_TAG__DM_MESSAGE_DESCRIPTOR						(0x81) // 
#define DESC_TAG__DM_NAME_DESCRIPTOR						(0x82) // 
#define DESC_TAG__BROADCASTER_IDENTIFIER_DESCRIPTOR			(0x85) // 
#define DESC_TAG__DM_COPY_MANAGEMENT_DESCRIPTOR				(0x8b) // 

#define DESC_TAG__DM_JP_SI_SYSTEM_DESCRIPTOR				(0x99) // 
#define DESC_TAG__DM_JP_NETWORK_LIST_DESCRIPTOR				(0x9a) // 
#define DESC_TAG__SP_POWER_CONTROL_DESCRIPTOR				(0x9c) // 

#define DESC_TAG__HIERARCHICAL_TRANSMISSON_DESCRIPTOR		(0xc0) // 階層伝送記述子
#define DESC_TAG__DIGITAL_COPY_CONTROL_DESCRIPTOR			(0xc1) // デジタルコピー制御記述子
#define DESC_TAG__NETWORK_ID_DESCRIPTOR						(0xc2) // ネットワーク識別記述子
#define DESC_TAG__PARTIAL_TRANSPORT_TIME_DESCRIPTOR			(0xc3) // パーシャルTSタイム記述子
#define DESC_TAG__AUDIO_COMPONENT_DESCRIPTOR				(0xc4) // 音声コンポーネント記述子
#define DESC_TAG__HYPERLINK_DESCRIPTOR						(0xc5) // ハイパーリンク記述子
#define DESC_TAG__TARGER_REGION_DESCRIPTOR					(0xc6) // 対象地域記述子
#define DESC_TAG__DATA_CONTENTS_DESCRIPTOR					(0xc7) // データコンテント記述子
#define DESC_TAG__VIDEO_DECODE_CONTROL_DESCRIPTOR			(0xc8) // ビデオデコードコントロール記述子
#define DESC_TAG__DOWNLOAD_CONTENT_DESCRIPTOR				(0xc9) // ダウンロードコンテンツ記述子
#define DESC_TAG__CA_EMM_TS_DESCRIPTOR						(0xca) // CA_EMM_TS記述子
#define DESC_TAG__CA_CONTRACT_DESCRIPTOR					(0xcb) // CA契約情報記述子
#define DESC_TAG__CA_SERVICE_DESCRIPTOR						(0xcc) // CAサービス記述子
#define DESC_TAG__TS_INFORMATION_DESCRIPTOR					(0xcd) // TS情報記述子
#define DESC_TAG__EXTENDED_BROADCASTER_DESCRIPTOR			(0xce) // 拡張ブロードキャスタ記述子
#define DESC_TAG__LOGO_TRANSMISSION_DESCRIPTOR				(0xcf) // ロゴ伝送記述子

#define DESC_TAG__BASIC_LOCAL_EVENT_DESCRIPTOR				(0xd0) // 基本ローカルイベント記述子
#define DESC_TAG__REFERENCE_DESCRIPTOR						(0xd1) // リファレンス記述子
#define DESC_TAG__NODE_RELATION_DESCRIPTOR					(0xd2) // ノード関係記述子
#define DESC_TAG__SHORT_NODE_INFORMATION_DESCRIPTOR			(0xd3) // 短形式ノード情報記述子
#define DESC_TAG__STC_REFERENCE_DESCRIPTOR					(0xd4) // STC参照記述子
#define DESC_TAG__SERIES_DESCRIPTOR							(0xd5) // シリーズ記述子
#define DESC_TAG__EVENT_GROUP_DESCRIPTOR					(0xd6) // イベントグループ記述子
#define DESC_TAG__SI_PARAMETER_DESCRIPTOR					(0xd7) // SI伝送パラメータ記述子
#define DESC_TAG__BROADCASTER_NAME_DESCRIPTOR				(0xd8) // ブロードキャスタ名記述子
#define DESC_TAG__COMPONENT_GROUP_DESCRIPTOR				(0xd9) // コンポーネントグループ記述子
#define DESC_TAG__SI_PRIME_TS_DESCRIPTOR					(0xda) // SIプライムTS記述子
#define DESC_TAG__BOARD_INFORMATION_DESCRIPTOR				(0xdb) // 掲示板情報記述子
#define DESC_TAG__LDT_LINKAGE_DESCRIPTOR					(0xdc) // LDTリンク記述子
#define DESC_TAG__CONNECT_SENDING_DESCRIPTOR				(0xdd) // 連結送信記述子
#define DESC_TAG__CONTENT_AVAILABILITY_DESCRIPTOR			(0xde) // コンテント利用記述子

#define DESC_TAG__CABLE_TS_DEVISION_SYSTEM_DESCRIPTOR		(0xf9) // 有線TS分割システム記述子
#define DESC_TAG__TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR	(0xfa) // 地上分配システム記述子
#define DESC_TAG__PARTIAL_RECEPTION_DESCRIPTOR				(0xfb) // 部分受信記述子
#define DESC_TAG__EMERGENCY_INFORMATION_DESCRIPTOR			(0xfc) // 緊急情報記述子
#define DESC_TAG__DATA_COMPONENT_DESCRIPTOR					(0xfd) // データ符号化方式記述子
#define DESC_TAG__SYSTEM_MANAGEMENT_DESCRIPTOR				(0xfe) // システム管理記述子


class CDescriptorCommon {
public:
	static void dump (uint8_t tag, const CDescriptor &desc) {
		switch (tag) {
		case DESC_TAG__SHORT_EVENT_DESCRIPTOR:
			{
				CShortEventDescriptor sed (desc);
				if (sed.isValid) {
					sed.dump();
				} else {
					_UTL_LOG_W ("invalid ShortEventDescriptor\n");
				}
			}
			break;

		case DESC_TAG__EXTENDED_EVENT_DESCRIPTOR:
			{
				CExtendedEventDescriptor eed (desc);
				if (eed.isValid) {
					eed.dump();
				} else {
					_UTL_LOG_W ("invalid ExtendedEventDescriptor\n");
				}
			}
			break;

		case DESC_TAG__COMPONENT_DESCRIPTOR:
			{
				CComponentDescriptor cd (desc);
				if (cd.isValid) {
					cd.dump();
				} else {
					_UTL_LOG_W ("invalid ComponentDescriptor\n");
				}
			}
			break;

		case DESC_TAG__AUDIO_COMPONENT_DESCRIPTOR:
			{
				CAudioComponentDescriptor acd (desc);
				if (acd.isValid) {
					acd.dump();
				} else {
					_UTL_LOG_W ("invalid AudioComponentDescriptor\n");
				}
			}
			break;

		case DESC_TAG__SERIES_DESCRIPTOR:
			{
				CSeriesDescriptor sd (desc);
				if (sd.isValid) {
					sd.dump();
				} else {
					_UTL_LOG_W ("invalid SeriesDescriptor\n");
				}
			}
			break;

		case DESC_TAG__CONTENT_DESCRIPTOR:
			{
				CContentDescriptor cd (desc);
				if (cd.isValid) {
					cd.dump();
				} else {
					_UTL_LOG_W ("invalid ContentDescriptor\n");
				}
			}
			break;

		case DESC_TAG__SERVICE_LIST_DESCRIPTOR:
			{
				CServiceListDescriptor sld (desc);
				if (sld.isValid) {
					sld.dump();
				} else {
					_UTL_LOG_W ("invalid ServiceListDescriptor\n");
				}
			}
			break;

		case DESC_TAG__SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR:
			{
				CSatelliteDeliverySystemDescriptor sdsd (desc);
				if (sdsd.isValid) {
					sdsd.dump();
				} else {
					_UTL_LOG_W ("invalid SatelliteDeliverySystemDescriptor\n");
				}
			}
			break;

		case DESC_TAG__TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR:
			{
				CTerrestrialDeliverySystemDescriptor tdsd (desc);
				if (tdsd.isValid) {
					tdsd.dump();
				} else {
					_UTL_LOG_W ("invalid TerrestrialDeliverySystemDescriptor\n");
				}
			}
			break;

		case DESC_TAG__TS_INFORMATION_DESCRIPTOR:
			{
				CTSInformationDescriptor tsid (desc);
				if (tsid.isValid) {
					tsid.dump();
				} else {
					_UTL_LOG_W ("invalid TSInformationDescriptor\n");
				}
			}
			break;

		case DESC_TAG__SERVICE_DESCRIPTOR:
			{
				CServiceDescriptor sd (desc);
				if (sd.isValid) {
					sd.dump();
				} else {
					_UTL_LOG_W ("invalid ServiceDescriptor\n");
				}
			}
			break;

		case DESC_TAG__SI_PARAMETER_DESCRIPTOR:
			{
				CSIParameterDescriptor spd (desc);
				if (spd.isValid) {
					spd.dump();
				} else {
					_UTL_LOG_W ("invalid SIParameterDescriptor\n");
				}
			}
			break;

		case DESC_TAG__NETWORK_NAME_DESCRIPTOR:
			{
				CNetworkNameDescriptor nnd (desc);
				if (nnd.isValid) {
					nnd.dump();
				} else {
					_UTL_LOG_W ("invalid NetworkNameDescriptor\n");
				}
			}
			break;

		case DESC_TAG__CA_IDENTIFIER_DESCRIPTOR:
			{
				CCAIdentifierDescriptor caid (desc);
				if (caid.isValid) {
					caid.dump();
				} else {
					_UTL_LOG_W ("invalid CAIdentifierDescriptor\n");
				}
			}
			break;

		case DESC_TAG__CONDITIONAL_ACCESS_DESCRIPTOR:
			{
				CConditionalAccessDescriptor cad (desc);
				if (cad.isValid) {
					cad.dump();
				} else {
					_UTL_LOG_W ("invalid ConditionalAccessDescriptor\n");
				}
			}
			break;

		case DESC_TAG__STREAM_IDENTIFIER_DESCRIPTOR:
			{
				CStreamIdentifierDescriptor sid (desc);
				if (sid.isValid) {
					sid.dump();
				} else {
					_UTL_LOG_W ("invalid StreamIdentifierDescriptor\n");
				}
			}
			break;

		default:
			desc.dump();
			break;
		}
	};
};
*/
#endif
