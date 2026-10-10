#ifndef _DSMCC_DESCRIPTOR_H_
#define _DSMCC_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"

namespace parser {



class dsmcc_descriptor {
public:
	explicit dsmcc_descriptor (const uint8_t *p_desc);
	dsmcc_descriptor (const dsmcc_descriptor &obj);
	~dsmcc_descriptor (void);

	virtual void dump (void) const;


	uint8_t tag;
	uint8_t length;
	uint8_t data [0xff];

	bool is_valid;

protected:
	bool parse (const uint8_t *p_desc);
	void dump (bool is_data_dump) const;

};


} // namespace parser

#endif
