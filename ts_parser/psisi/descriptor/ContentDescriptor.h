#ifndef _CONTENT_DESCRIPTOR_H_
#define _CONTENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsCommon.h"
#include "Descriptor.h"


class CContentDescriptor : public CDescriptor
{
public:
	class CContent {
	public:
		CContent (void)
			:content_nibble_level_1 (0)
			,content_nibble_level_2 (0)
			,user_nibble_1 (0)
			,user_nibble_2 (0)
		{}
		virtual ~CContent (void) {}

		uint8_t content_nibble_level_1;
		uint8_t content_nibble_level_2;
		uint8_t user_nibble_1;
		uint8_t user_nibble_2;
	};
public:
	explicit CContentDescriptor (const CDescriptor &obj);
	virtual ~CContentDescriptor (void);

	void dump (void) const override;

	std::vector <CContent> contents;

private:
	bool parse (void);

};

#endif
