#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsCommon.h"


class CDescriptor {
public:
	explicit CDescriptor (const uint8_t *pDesc);
	CDescriptor (const CDescriptor &obj);
	~CDescriptor (void);

	virtual void dump (void) const;


	uint8_t tag;
	uint8_t length;
	uint8_t data [0xff];

	bool isValid;

protected:
	bool parse (const uint8_t *pDesc);
	void dump (bool isDataDump) const;

};

#endif
