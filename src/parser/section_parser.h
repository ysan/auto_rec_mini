#ifndef _SECTION_PARSER_H_
#define _SECTION_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"

namespace parser {



typedef enum {
	EN_SECTION_STATE__INIT = 0,
	EN_SECTION_STATE__RECEIVING,
	EN_SECTION_STATE__RECEIVED,
	EN_SECTION_STATE__COMPLETE,

	EN_SECTION_STATE_NUM,
} EN_SECTION_STATE;

typedef enum {
	EN_SECTION_TYPE__PSISI = 0,
	EN_SECTION_TYPE__DSMCC,

	EN_SECTION_TYPE_NUM,
} EN_SECTION_TYPE;


class section_info {
public:
	friend class section_parser;

	section_info (uint8_t *p_buff, size_t size, EN_SECTION_TYPE en_type=EN_SECTION_TYPE__PSISI);
	section_info (section_info *p_obj);
	~section_info (void);

	bool operator==(const section_info &obj) const {
		if (!raw_ || !tail_) {
			return false;
		}

		if (!obj.raw_ || !obj.tail_) {
			return false;
		}

		size_t na_len = obj.tail_ - obj.raw_;
		size_t nb_len = tail_ - raw_;
		if (na_len != nb_len) {
			return false;
		}

//		if (memcmp (obj.raw_, raw_, TS_PACKET_LEN - (TS_HEADER_LEN +1)) != 0) { // +1はpointer_field分
		if (memcmp (obj.raw_, raw_, na_len) != 0) {
			return false;
		}

		return true;
	}


	uint8_t *get_header_addr (void) const;
	const ST_SECTION_HEADER *get_header (void) const;
	uint8_t *get_data_part_addr (void) const;
	uint16_t get_data_part_len (void) const;

	static uint8_t * parse_header (ST_SECTION_HEADER *p_hdr_out, uint8_t *p_start, uint8_t *p_end, EN_SECTION_TYPE en_type=EN_SECTION_TYPE__PSISI) ;
	static void dump_header (const ST_SECTION_HEADER *p_hdr, bool is_short_format) ;


private:
	void parse_header (void);
	void dump_header (void) const;
	bool is_receive_all (void) const;
	size_t truncate (void);
	void clear (void);
	bool check_crc32 (void) const;


	ST_SECTION_HEADER sect_hdr_;
	uint8_t *raw_; // section header
	uint8_t *data_; // section data head
	uint8_t *tail_; // payload tail
	EN_SECTION_STATE state_;
	EN_SECTION_TYPE type_;


	// link-list
	section_info *next_;
};



typedef enum {
	EN_CHECK_SECTION__COMPLETED = 0,
	EN_CHECK_SECTION__COMPLETED_ALREADY,
	EN_CHECK_SECTION__RECEIVING,
	EN_CHECK_SECTION__CANCELED,
	EN_CHECK_SECTION__CRC32_ERR,
	EN_CHECK_SECTION__IGNORE,
	EN_CHECK_SECTION__INVALID,
} EN_CHECK_SECTION;

class section_parser
{
public:
    static const uint32_t POOL_SIZE;
	
public:
	section_parser (EN_SECTION_TYPE en_type=EN_SECTION_TYPE__PSISI);
	section_parser (int fifo_num, EN_SECTION_TYPE en_type=EN_SECTION_TYPE__PSISI);
	section_parser (size_t pool_size, EN_SECTION_TYPE en_type=EN_SECTION_TYPE__PSISI);
	section_parser (size_t pool_size, int fifo_num, EN_SECTION_TYPE en_type=EN_SECTION_TYPE__PSISI);
	virtual ~section_parser (void);


	EN_CHECK_SECTION check_section (const TS_HEADER *p_ts_hdr, uint8_t *p_payload, size_t payload_size);
	void check_async_delete (void);
	void dump_section_list (void) const;


protected:
	section_info *get_latest_complete_section (void) const;
	void detach_section_list (const section_info *p_sect_info);
	void detach_all_section_list (void);
	section_info *search_section_list (const section_info &sect_info) const;
	int get_section_list_num (void) const;
	uint16_t get_pid (void) const;

	virtual bool on_section_started (const section_info *p_section);
	virtual void on_section_completed (const section_info *p_comp_section);

	void async_delete (void);

private:
//	void convert_raw2_section_info (section_info *p_out, uint8_t *p_buff, size_t size);

	section_info* attach_section_list (uint8_t *p_buff, size_t size);
	section_info* add_section_list (section_info *p_sect_info);

	void delete_section_list (const section_info &sect_info, bool is_delete=true);
	void delete_section_list (const section_info *p_sect_info, bool is_delete=true);
	void delete_all_section_list (void);
	void shift_all_section_list (size_t shift_size);

	void check_detach_fifo_section_list (void);
	section_info* check_delete_fifo_section_list (void);

	// pool full 時に最古(先頭)の COMPLETE セクションを1つ退避して領域を空ける
	bool evict_oldest_section (void);


	EN_CHECK_SECTION check_section_first (uint8_t *p_payload, size_t payload_size);
	EN_CHECK_SECTION check_section_follow (uint8_t *p_payload, size_t payload_size);

	void truncate (size_t truncate_len);


	uint16_t pid_;

	section_info *sect_list_top_;
	section_info *sect_list_bottom_;

	bool list_is_fifo_;
	int fifo_num_; // limited num of section_list

	section_info *work_sect_info_;

	uint8_t *pool_;
	size_t pool_ind_;
	size_t pool_size_;

	EN_SECTION_TYPE type_;

	bool is_async_delete_;
	bool is_ignore_section_;
};


} // namespace parser

#endif
