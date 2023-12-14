#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <memory>

#include "Defs.h"

#include "TsAribCommon.h"


class CTsParser
{
public:
	class IParserListener {
	public:
		virtual ~IParserListener (void) {};
		virtual bool on_ts_packet_available (TS_HEADER *p_header, uint8_t *p_payload, size_t payload_size) = 0;
	};

public:
	static constexpr uint32_t INNER_BUFF_SIZE = 65535*5;

	enum class buff_state : int {
		CACHING = 0,
		FULL,
	};

	CTsParser (void);
	explicit CTsParser (IParserListener *p_listener);
	virtual ~CTsParser (void);

	void run (uint8_t *buff, size_t size);

private:
	bool allocate_to_inner_buffer (uint8_t *p_buff, size_t size);
	buff_state copy_to_inner_buffer (uint8_t *p_buff, size_t size);
	int get_unit_size (void) const;
	uint8_t *get_sync_top_addr (uint8_t *p_top, uint8_t *p_bottom, size_t unit_size) const;
	int check_unit_size (void);
	bool parse (void);


	uint8_t *mp_top ;
	uint8_t *mp_current ;
	uint8_t *mp_bottom ;
	size_t m_buff_size ;
	int m_parse_remain_len;

	IParserListener *mp_listener;

	std::unique_ptr<uint8_t[]> m_inner_buff;
};

#endif
