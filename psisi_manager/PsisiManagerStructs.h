#ifndef _PSISI_MANAGER_STRUCTS_H_
#define _PSISI_MANAGER_STRUCTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Utils.h"


//---------------------------------------------------
typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;

	uint16_t program_number;
	uint16_t program_map_PID;

	void dump (void) {
		_UTL_LOG_I (
			"tsid:[0x%04x] pgm_num:[0x%04x] pmt_pid:[0x%04x]",
			transport_stream_id,
			program_number,
			program_map_PID
		);
	}

} PSISI_PROGRAM_INFO;


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

} PSISI_SERVICE_INFO;

typedef struct {
	PSISI_SERVICE_INFO *p_out_serviceInfos;
	int array_max_num;
} REQ_SERVICE_INFOS_PARAM;


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

} PSISI_EVENT_INFO;

typedef struct {
	PSISI_SERVICE_INFO key;
	PSISI_EVENT_INFO *p_out_eventInfo;
} REQ_EVENT_INFO_PARAM;

typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;

	void dump (void) {
		_UTL_LOG_I (
			"tblid:[0x%2x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
	}

} PSISI_NOTIFY_EVENT_INFO;


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

} PSISI_NETWORK_INFO;

typedef struct {
	PSISI_NETWORK_INFO *p_out_networkInfo;
} REQ_NETWORK_INFO_PARAM;


#endif
