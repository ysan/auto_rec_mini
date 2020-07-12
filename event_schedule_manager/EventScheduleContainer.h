#ifndef _EVENT_SCHEDULE_CONTAINER_H_
#define _EVENT_SCHEDULE_CONTAINER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <map>
#include <string>

#include "Settings.h"
#include "Utils.h"
#include "TsAribCommon.h"

#include "EventScheduleManagerIf.h"
#include "Event.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/vector.hpp"


class CEventScheduleContainer
{
public:
	CEventScheduleContainer (void);
	virtual ~CEventScheduleContainer (void);

	bool addScheduleMap (const SERVICE_KEY_t &key, std::vector <CEvent*> *p_sched);
	void deleteScheduleMap (const SERVICE_KEY_t &key);
	bool hasScheduleMap (const SERVICE_KEY_t &key) const;
	void dumpScheduleMap (void) const;
	void dumpScheduleMap (const SERVICE_KEY_t &key) const;

	const CEvent *getEvent (const CEventScheduleManagerIf::EVENT_KEY_t &key) const;
	const CEvent *getEvent (const SERVICE_KEY_t &key, int index) const;
	int getEvents (
		const char *p_keyword,
		CEventScheduleManagerIf::EVENT_t *p_out_event,
		int out_array_num,
		bool is_check_extendedEvent
	) const;

	void clear (void);

private:
	void clearSchedule (std::vector <CEvent*> *p_sched);
	void dumpSchedule (const std::vector <CEvent*> *p_sched) const;

	CSettings *mp_settings;

	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> m_sched_map;

};

#endif
