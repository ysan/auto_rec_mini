#ifndef _PSISI_MANAGER_STRUCTS_H_
#define _PSISI_MANAGER_STRUCTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Utils.h"
#include "TsAribCommon.h"


namespace psisi_structs {

//---------------------------------------------------
typedef struct {
	uint8_t type;
	uint16_t pid; // pes pid
} stream_info_t;

typedef struct {
	uint16_t program_number; // service_id
	EN_STREAM_TYPE type;
	stream_info_t *p_out_stream_infos;
	int array_max_num;
} request_stream_infos_param_t;


//---------------------------------------------------
typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint8_t service_type;
	const char *p_service_name_char;

	void dump (void) {
		_UTL_LOG_I (
			"tblid:[0x%2x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] svctype:[0x%02x] [%s]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			service_type,
			p_service_name_char
		);
	}

} service_info_t;

typedef struct {
	service_info_t *p_out_service_infos;
	int array_max_num;
} request_service_infos_param_t;


//---------------------------------------------------
typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	char event_name_char [1024];

	void clear (void) {
		table_id = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();
		memset (event_name_char, 0x00, sizeof(event_name_char));
	}

	void dump (void) const {
		_UTL_LOG_I (
			"tblid:[0x%2x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s]",
			start_time.toString(),
			end_time.toString()
		);
		_UTL_LOG_I ("event_name:[%s]", event_name_char);
	}

} event_info_t;

typedef struct {
	service_info_t key;
	event_info_t *p_out_event_info;
} request_event_info_param_t;

typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;

	void dump (void) {
		_UTL_LOG_I (
			"psisi_notify event_info: tblid:[0x%2x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
	}

} notify_event_info_t;


//---------------------------------------------------
typedef struct {
public:
	uint8_t table_id;
	uint16_t network_id;
	char network_name_char [64];

	uint16_t transport_stream_id;
	uint16_t original_network_id;

	struct service {
		uint16_t service_id;
		uint8_t service_type;
	} services [16];
	int services_num;

	uint16_t area_code;
	uint8_t guard_interval;
	uint8_t transmission_mode;
	uint8_t remote_control_key_id;
	char ts_name_char  [64];


	void dump (void) {
		_UTL_LOG_I (
			"tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id
		);
		_UTL_LOG_I ("network_name:[%s]", network_name_char);
		_UTL_LOG_I ("----------");
		for (int i = 0; i < services_num; ++ i) {
			_UTL_LOG_I (
				"svcid:[0x%04x] svctype:[0x%02x]",
				services[i].service_id,
				services[i].service_type
			);
		}
		_UTL_LOG_I ("----------");
		_UTL_LOG_I (
			"area_code:[0x%04x] guard:[0x%02x] trans_mode:[0x%02x]",
			area_code,
			guard_interval,
			transmission_mode
		);
		_UTL_LOG_I (
			"remote_control_key_id:[0x%04x] ts_name:[%s]",
			remote_control_key_id,
			ts_name_char
		);
	}

} network_info_t;

typedef struct {
	network_info_t *p_out_network_info;
} request_network_info_param_t;

} // psisi_structs

#endif
