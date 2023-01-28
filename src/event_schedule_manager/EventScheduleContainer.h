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
#include <memory>

#include "Settings.h"
#include "Utils.h"
#include "TsAribCommon.h"

#include "EventScheduleManagerIf.h"
#include "Event.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/memory.hpp"


class CEventScheduleContainer
{
public:
	CEventScheduleContainer (void);
	virtual ~CEventScheduleContainer (void);

	void set_schedule_map_json_path (std::string sched_map_json_path);

	bool add_schedule_map (const service_key_t &key, std::vector <CEvent*> *p_sched);
	void delete_schedule_map (const service_key_t &key);
	bool has_schedule_map (const service_key_t &key) const;
	void dump_schedule_map (void) const;
	void dump_schedule_map (const service_key_t &key) const;

	const CEvent *get_event (const service_key_t &key, uint16_t event_id) const;
	const CEvent *get_event (const service_key_t &key, int index) const;
	int get_events (
		const char *p_keyword,
		CEventScheduleManagerIf::event_t *p_out_event,
		int out_array_num,
		bool is_check_extended_event
	) const;

	void clear (void);

	void save_schedule_map (void);
	void load_schedule_map (void);

private:
//	void clear_schedule (std::vector <CEvent*> *p_sched);
	void clear_schedule (std::vector<std::unique_ptr<CEvent>> &sched);
//	void dump_schedule (const std::vector <CEvent*> *p_sched) const;
	void dump_schedule (const std::vector<std::unique_ptr<CEvent>> &sched) const;

	CSettings *mp_settings;
	std::string m_sched_map_json_path;

//	std::map <service_key_t, std::vector <CEvent*> *> m_sched_map;
	std::map <service_key_t, std::unique_ptr<std::vector<std::unique_ptr<CEvent>>>> m_sched_map;

};

#endif
