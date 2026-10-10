#ifndef _GROUP_H_
#define _GROUP_H_

#include "utils.h"

namespace arec {

class group {
public:
	static const uint8_t GROUP_MAX = 4;
protected:
	explicit group (uint8_t id=0) {
		if (id >= GROUP_MAX) {
			_UTL_LOG_E("invalid group:[%d]  -> force id=0", id);
			id = 0;
		}
		id_ = id;
	}

	virtual ~group (void) {}

	uint8_t get_group_id (void) const {
		return id_;
	}

private:
	uint8_t id_;
};

} // namespace arec

#endif
