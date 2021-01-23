#ifndef _GROUP_H_
#define _GROUP_H_

#include <cassert>
#include "Utils.h"

#define GROUP_MAX	(3)

class CGroup {
protected:
	explicit CGroup (uint8_t id=0) {
		if (id >= GROUP_MAX) {
			_UTL_LOG_E("invalid group:[%d]  -> force id=0", id);
			id = 0;
		}
		m_id = id;
	}

	virtual ~CGroup (void) {}

	uint8_t getGroupId (void) const {
		return m_id;
	}

private:
	uint8_t m_id;
};

#endif
