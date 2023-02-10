#ifndef _ARIB_B25_PROCESS_H_
#define _ARIB_B25_PROCESS_H_

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include <arib25/arib_std_b25.h>
#include <arib25/b_cas_card.h>
#include <utility>

#include "BufferedProcess.h"
#include "Utils.h"
#include "TsAribCommon.h"
#include "tssplitter_lite.h"

class CAribB25Process : public CBufferedProcess
{
public:
	CAribB25Process (size_t size, uint16_t service_id, bool use_splitter, std::function<int (const void *buff, size_t size)> b25_processed_handler)
		: CBufferedProcess (size > TS_PACKET_LEN * 512 ? size : TS_PACKET_LEN * 512)
		, mp_b25 (NULL)
		, mp_b25cas (NULL)
		, m_service_id (service_id)
		, m_use_splitter(use_splitter)
		, mp_splitter(NULL)
		, m_split_select_state(TSS_ERROR)
		, m_b25_processed_handler(std::move(b25_processed_handler))
	{
		init();
		init_b25();

		std::string sid = std::to_string(m_service_id);
		memset (m_service_id_string, 0x00, sizeof(m_service_id_string));
		strncpy (m_service_id_string, sid.c_str(), sid.length());

		if (m_use_splitter) {
			// init tssplitter_lite
			_UTL_LOG_I ("split_startup %d (0x%04x) -> [%s]", m_service_id, m_service_id, m_service_id_string);
			mp_splitter = split_startup(m_service_id_string);
			if (mp_splitter->sid_list == NULL) {
				_UTL_LOG_W("Cannot start TS splitter");
			}
			m_splitbuf.buffer = NULL;
			m_splitbuf.buffer_size = 0;
		}
	}
	
	virtual ~CAribB25Process (void) {
		finaliz_b25();

		if (m_use_splitter) {
			// finaliz tssplitter_lite
			if (m_splitbuf.buffer) {
				free (m_splitbuf.buffer);
				m_splitbuf.buffer = NULL;
				m_splitbuf.buffer_size = 0;
			}
			if (mp_splitter) {
				split_shutdown(mp_splitter);
				mp_splitter = NULL;
			}
		}
	}

	void set_service_id (uint16_t _id) {
		m_service_id = _id;
	}

	void init () {

		std::function<int(bool, uint8_t*)> _process = [this] (bool need_proc_inner_buff, uint8_t* p_buffer) {
			int r = 0;
			uint8_t* p = need_proc_inner_buff ? get_buffer() : p_buffer;
			size_t len = need_proc_inner_buff ? get_buffered_position() : get_buffer_size();
			if (len == 0) {
				return 0;
			}

			if (mp_b25) {
				ARIB_STD_B25_BUFFER b25_sbuf;
				ARIB_STD_B25_BUFFER b25_dbuf;
				b25_sbuf.data = p;
				b25_sbuf.size = len;
				if ((r = mp_b25->put(mp_b25, &b25_sbuf) ) < 0) {
					_UTL_LOG_W ("StdB25.put failed");
				}
				if ((r = mp_b25->get(mp_b25, &b25_dbuf) ) < 0) {
					_UTL_LOG_W ("StdB25.get failed");
				} else {
					p = b25_dbuf.data;
					len = b25_dbuf.size;
				}
			}
			
			if (m_use_splitter) {
				int code = 0;
				ARIB_STD_B25_BUFFER buf;
				buf.data = p;
				buf.size = len;
				m_splitbuf.buffer_filled = 0;

				/* allocate split buffer */
				if (m_splitbuf.buffer_size < buf.size) {
					if (m_splitbuf.buffer) {
						free(m_splitbuf.buffer);
						m_splitbuf.buffer = NULL;
						m_splitbuf.buffer_size = 0;
					}
					m_splitbuf.buffer = (uint8_t*)malloc(buf.size);
					if(m_splitbuf.buffer == NULL) {
						_UTL_LOG_E("split buffer allocation failed");
						// next
						return 0;
					}
					m_splitbuf.buffer_size = buf.size;
				}

				if (m_split_select_state != TSS_SUCCESS) {
					m_split_select_state = split_select(mp_splitter, &buf);
					if (m_split_select_state == TSS_SUCCESS) {
						_UTL_LOG_I ("m_split_select_state ==> TSS_SUCCESS");

					} else if (m_split_select_state == TSS_NULL) {
						_UTL_LOG_E("split_select inner malloc failed");
						// next
						return 0;

					} else {
						// TSS_ERROR
#if 0
						time_t cur_time;
						time(&cur_time);
						if(cur_time - tdata->start_time > 4) {
							goto fin;
						}
						break;
#endif
						// next
						return 0;
					}
				}

				code = split_ts(mp_splitter, &buf, &m_splitbuf);
				if (code == TSS_NULL) {
					_UTL_LOG_W("PMT reading..");
				} else if(code != TSS_SUCCESS) {
					_UTL_LOG_W("split_ts failed");
				}

				p = m_splitbuf.buffer;
				len = m_splitbuf.buffer_filled;
			}

			if (m_b25_processed_handler) {
				if (m_b25_processed_handler (p, len) < 0) {
					return -1;
				}
			}

			return 0;
		};

		std::function<void(void)> _finalize = [this] (void) {
			finaliz_b25();

			if (m_use_splitter) {
				// finaliz tssplitter_lite
				if (m_splitbuf.buffer) {
					free (m_splitbuf.buffer);
					m_splitbuf.buffer = NULL;
					m_splitbuf.buffer_size = 0;
				}
				if (mp_splitter) {
					split_shutdown(mp_splitter);
					mp_splitter = NULL;
				}
			}
		};

		set_process_handler (_process);
		set_finalize_handler (_finalize);
	}

	void init_b25 (void) {
		int r = 0;
		const unsigned MULTI2_round = 4;
		ARIB_STD_B25* const b25 = create_arib_std_b25();
		if (b25) {
			_UTL_LOG_I ("# StdB25 interface : ");
		} else {
			_UTL_LOG_E ("Create StdB25 interface failed");
			mp_b25 = NULL;
			mp_b25cas = NULL;
			return;
		}
		r = b25->set_multi2_round(b25, MULTI2_round);
		if (r < 0) {
			_UTL_LOG_E ("StdB25.MULTI2 round=%u failed", MULTI2_round);
			b25->release(b25);
			mp_b25 = NULL;
			mp_b25cas = NULL;
			return;
		}
		b25->set_strip(b25, 1);
		b25->set_emm_proc(b25, 0);

		B_CAS_CARD* const bc = create_b_cas_card();
		if (!bc) {
			_UTL_LOG_E ("Create CAS interface failed");
			b25->release(b25);
			mp_b25 = NULL;
			mp_b25cas = NULL;
			return;
		}
		r = bc->init(bc);
		if(r < 0) {
			_UTL_LOG_E ("Init CAS failed");
			bc->release(bc);
			b25->release(b25);
			mp_b25 = NULL;
			mp_b25cas = NULL;
			return;
		}
		r = b25->set_b_cas_card(b25, bc);
		if(r < 0) {
			_UTL_LOG_E ("StdB25.SetCAS failed");
			bc->release(bc);
			b25->release(b25);
			mp_b25 = NULL;
			mp_b25cas = NULL;
			return;
		}
		mp_b25 = b25;
		mp_b25cas = bc;

		B_CAS_ID casID;
		r = bc->get_id(bc, &casID);
		if (r < 0) {
			_UTL_LOG_E ("Get CAS ID failed");
		} else {
			for(r = 0; r < casID.count; r++) {
				const uint64_t  val = casID.data[r];
				_UTL_LOG_I ("%u%014llu, ", (val >> 45) & 0x7,val & ((1ULL << 45) - 1));
			}
		}
		_UTL_LOG_I ("done.");
	}

	void finaliz_b25 (void) {
		if (mp_b25) {
			mp_b25->release (mp_b25);
			mp_b25 = NULL;
		}
		if (mp_b25cas) {
			mp_b25cas->release (mp_b25cas);
			mp_b25cas = NULL;
		}
	}

	int process_remaining (void) override {
		int r = 0;
		r = CBufferedProcess::process_remaining();
		if (r < 0) {
			return r;
		}
		return 0;
	}

	void finalize (void) override {
		CBufferedProcess::finalize();
	}
	
private:
	ARIB_STD_B25 *mp_b25;
	B_CAS_CARD *mp_b25cas;

	uint16_t m_service_id;
	char m_service_id_string [64];

	bool m_use_splitter;
	splitter *mp_splitter;
	splitbuf_t m_splitbuf;
	int m_split_select_state;
	std::function<int (void *buff, size_t size)> m_b25_processed_handler;
};

#endif
