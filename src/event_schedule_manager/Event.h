#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <map>
#include <string>

#include "Utils.h"
#include "TsAribCommon.h"
#include "EventScheduleManagerIf.h"


typedef struct _service_key {
public:
	_service_key (void) {
		clear ();
    }

	_service_key (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	) {
		clear ();
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
    }

	_service_key (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint8_t _service_type,
		const char* _p_service_name_char
	) {
		clear ();
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->service_type = _service_type;
		this->service_name = _p_service_name_char;
    }

	explicit _service_key (CEventScheduleManagerIf::service_key_t &_key) {
		clear ();
		this->transport_stream_id = _key.transport_stream_id;
		this->original_network_id = _key.original_network_id;
		this->service_id = _key.service_id;
    }

	virtual ~_service_key (void) {
		clear ();
	}


	bool operator == (const _service_key &obj) const {
		if (
			this->transport_stream_id == obj.transport_stream_id &&
			this->original_network_id == obj.original_network_id &&
			this->service_id == obj.service_id
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator != (const _service_key &obj) const {
		if (
			this->transport_stream_id != obj.transport_stream_id ||
			this->original_network_id != obj.original_network_id ||
			this->service_id != obj.service_id
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator < (const _service_key &obj) const {
		// map の二分木探査用
		// networkが一番優先高く 次に内包されるts_id
		// 更に内包される serviceという観点で比較します

		if (this->original_network_id < obj.original_network_id) {
			return true;
		} else if (this->original_network_id == obj.original_network_id) {
			if (this->transport_stream_id < obj.transport_stream_id) {
				return true;
			} else if (this->transport_stream_id == obj.transport_stream_id) {
				if (this->service_id < obj.service_id) {
					return true;
				} else if (this->service_id == obj.service_id) {
					return false;
				} else {
					return false;
				}
			} else {
				return false;
			}
		} else {
			return false;
		}
	}


	//// keys ////
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;
	//////////////


	// additional
	uint8_t service_type;
	std::string service_name;


	void clear (void) {
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		service_type = 0;
		service_name.clear();
	}

	void dump (void) const {
		_UTL_LOG_I (
			"#_service_key#  0x%04x 0x%04x 0x%04x  (svctype:0x%02x,%s)",
			transport_stream_id,
			original_network_id,
			service_id,
			service_type,
			service_name.c_str()
		);
	}

} service_key_t;


class CEvent {
public:
	class CExtendedInfo {
	public:
		CExtendedInfo (void) {
			item_description.clear();
			item.clear();
		}
		virtual ~CExtendedInfo (void) {
			item_description.clear();
			item.clear();
		}
		std::string item_description;
		std::string item;
	};

	class CGenre {
	public:
		CGenre (void) {
		}
		virtual ~CGenre (void) {
		}
		uint8_t content_nibble_level_1;
		uint8_t content_nibble_level_2;
	};

public:
	CEvent (void) {
		clear ();
	}
	virtual ~CEvent (void) {
		clear ();
	}

	bool operator == (const CEvent &obj) const {
		if (
//			this->table_id == obj.table_id &&
			this->transport_stream_id == obj.transport_stream_id &&
			this->original_network_id == obj.original_network_id &&
			this->service_id == obj.service_id &&
			this->event_id == obj.event_id
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator != (const CEvent &obj) const {
		if (
//			this->table_id != obj.table_id ||
			this->transport_stream_id != obj.transport_stream_id ||
			this->original_network_id != obj.original_network_id ||
			this->service_id != obj.service_id ||
			this->event_id != obj.event_id
		) {
			return true;
		} else {
			return false;
		}
	}


	uint8_t table_id; // sortするために必要
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint8_t section_number; // sortするために必要

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	std::string event_name;
	std::string text;

	uint8_t component_type;
	uint8_t component_tag;

	uint8_t audio_component_type;
	uint8_t audio_component_tag;
	uint8_t ES_multi_lingual_flag;
	uint8_t main_component_flag;
	uint8_t quality_indicator;
	uint8_t sampling_rate;

	std::vector <CGenre> genres;

	// table_id 0x58〜0x5fから取得します
	std::vector <CExtendedInfo> extended_infos;



	void clear (void) {
		table_id = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		section_number = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();	
		event_name.clear();
		text.clear();
		component_type = 0;
		component_tag = 0;
		audio_component_type = 0;
		audio_component_tag = 0;
		ES_multi_lingual_flag = 0;
		main_component_flag = 0;
		quality_indicator = 0;
		sampling_rate = 0;
		genres.clear();
		extended_infos.clear();
	}

	void dump (void) const {
		_UTL_LOG_I (
			"tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] num:[0x%02x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			section_number,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s]",
			start_time.toString(),
			end_time.toString()
		);

		_UTL_LOG_I ("event_name:[%s]", event_name.c_str());
		_UTL_LOG_I ("text:[%s]", text.c_str());

	}

	void dump_detail (void) const {
		std::vector<CExtendedInfo>::const_iterator iter_ex = extended_infos.begin();
		for (; iter_ex != extended_infos.end(); ++ iter_ex) {
			_UTL_LOG_I ("item_description:[%s]", iter_ex->item_description.c_str());
			_UTL_LOG_I ("item:[%s]", iter_ex->item.c_str());
		}

		_UTL_LOG_I (
			"component_type:[%s][%s]",
			CTsAribCommon::getVideoComponentType(component_type),
			CTsAribCommon::getVideoRatio(component_type)
		);

		_UTL_LOG_I ("audio_component_type:[%s]", CTsAribCommon::getAudioComponentType(audio_component_type));
		_UTL_LOG_I ("ES_multi_lingual_flag:[%s]", ES_multi_lingual_flag ? "二ヶ国語" : "-");
		_UTL_LOG_I ("main_component_flag:[%s]", main_component_flag ? "主" : "副");
		_UTL_LOG_I ("quality_indicator:[%s]", CTsAribCommon::getAudioQuality(quality_indicator));
		_UTL_LOG_I ("sampling_rate:[%s]\n", CTsAribCommon::getAudioSamplingRate(sampling_rate));

		std::vector<CGenre>::const_iterator iter_gr = genres.begin();
		for (; iter_gr != genres.end(); ++ iter_gr) {
			_UTL_LOG_I (
				"genre:[%s][%s]",
				CTsAribCommon::getGenre_lvl1(iter_gr->content_nibble_level_1),
				CTsAribCommon::getGenre_lvl1(iter_gr->content_nibble_level_2)
			);
		}
	}

};

#endif
