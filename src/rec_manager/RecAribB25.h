#ifndef _ARIB_B25_H_
#define _ARIB_B25_H_

#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arib25/arib_std_b25.h>
#include <arib25/b_cas_card.h>

#include "BufferedWriter.h"
#include "Utils.h"
#include "FileBufferedWriter.h"
#include "TsAribCommon.h"
#include "tssplitter_lite.h"

class CRecAribB25 : public CBufferedWriter
{
public:
	CRecAribB25 (size_t size, std::string output_name) 
		: CBufferedWriter (size > TS_PACKET_LEN * 512 ? size : TS_PACKET_LEN * 512)
		, mp_b25 (NULL)
		, mp_b25cas (NULL)
		, m_writer (768 * 1024, output_name)
	{
		init();
		init_b25();
	}
	
	virtual ~CRecAribB25 (void) {
		if (mp_b25) {
			mp_b25->release (mp_b25);
			mp_b25 = NULL;
		}
		if (mp_b25cas) {
			mp_b25cas->release (mp_b25cas);
			mp_b25cas = NULL;
		}
	}

	void set_service_id (uint16_t _id) {
		m_service_id = _id;
	}

	void init () {

		std::function<int(bool, uint8_t*)> _process = [this] (bool is_proc_inner_buff, uint8_t* p_buffer) {
			int r = 0;
			uint8_t* p = is_proc_inner_buff ? getBuffer() : p_buffer;
			size_t len = is_proc_inner_buff ? getWritedPosition() : getBufferSize();
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
			
			r = m_writer.put (p, len);
			if (0 < r) {
				return r;
			}

			return 0;
		};

		std::function<void(void)> _release = [this] (void) {
			if (mp_b25) {
				mp_b25->release (mp_b25);
				mp_b25 = NULL;
			}
			if (mp_b25cas) {
				mp_b25cas->release (mp_b25cas);
				mp_b25cas = NULL;
			}
		};

		set_process_handler (_process);
		set_release_handler (_release);
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

	int flush (void) override {
		int r = 0;
		r = CBufferedWriter::flush();
		if (r < 0) {
			return r;
		}
		r = m_writer.flush();
		if (r < 0) {
			return r;
		}
		return 0;
	}

	void release (void) override {
		m_writer.release();
		CBufferedWriter::release();
	}
	
private:
	ARIB_STD_B25 *mp_b25;
	B_CAS_CARD *mp_b25cas;
	CFileBufferedWriter m_writer;
	uint16_t m_service_id;
};

#endif
