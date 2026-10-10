#ifndef _CONTENT_DESCRIPTOR_H_
#define _CONTENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class content_descriptor : public descriptor
{
public:
	class content {
	public:
		content (void)
			:content_nibble_level_1 (0)
			,content_nibble_level_2 (0)
			,user_nibble_1 (0)
			,user_nibble_2 (0)
		{}
		virtual ~content (void) {}

		uint8_t content_nibble_level_1;
		uint8_t content_nibble_level_2;
		uint8_t user_nibble_1;
		uint8_t user_nibble_2;
	};
public:
	explicit content_descriptor (const descriptor &obj);
	virtual ~content_descriptor (void);

	void dump (void) const override;

	std::vector <content> contents;

private:
	bool parse (void);

};


} // namespace parser

#endif
