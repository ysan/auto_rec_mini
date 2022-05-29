#ifndef _PNG_CRC_H_
#define _PNG_CRC_H_

#include <iostream>
#include <memory>
#include "Defs.h"

/* from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html */

/* The following sample code represents a practical implementation 
   of the CRC (Cyclic Redundancy Check) employed in PNG chunks.
   (See also ISO 3309 [ISO-3309] or ITU-T V.42 [ITU-T-V42] for a
   formal specification.) */

class CPngcrc
{
public:
	CPngcrc (void) {
		m_crc_table = make_crc_table();
	}
	virtual ~CPngcrc (void) {
		m_crc_table = nullptr;
	}


	/* Return the CRC of the bytes buf[0..len-1]. */
	uint32_t crc(uint8_t *buf, int len) {
		return update_crc(0xffffffff, buf, len) ^ 0xffffffff;
	}

private:
	/* Make the table for a fast CRC. */
	std::shared_ptr<uint32_t> make_crc_table(void) {
		std::shared_ptr<uint32_t> sp(new uint32_t[256], std::default_delete<uint32_t[]>());
	
		for (int n = 0; n < 256; n++) {
			uint32_t c = (uint32_t) n;
			for (int k = 0; k < 8; k++) {
				if (c & 1)
					c = 0xedb88320 ^ (c >> 1);
				else
					c = c >> 1;
			}
			sp.get()[n] = c;
		}

		return sp;
	}

	/* Update a running CRC with the bytes buf[0..len-1]--the CRC
	   should be initialized to all 1's, and the transmitted value
	   is the 1's complement of the final running CRC (see the
	   crc() routine below). */
	uint32_t update_crc(uint32_t crc, uint8_t *buf, int len) {
		uint32_t c = crc;
	
		for (int n = 0; n < len; n++) {
			c = m_crc_table.get()[(c ^ buf[n]) & 0xff] ^ (c >> 8);
		}
		return c;
	}

	/* Table of CRCs of all 8-bit messages. */
	std::shared_ptr<uint32_t> m_crc_table;
};

#endif
