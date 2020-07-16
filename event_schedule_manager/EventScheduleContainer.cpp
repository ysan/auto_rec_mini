#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "EventScheduleContainer.h"


CEventScheduleContainer::CEventScheduleContainer (void)
	:mp_settings (NULL)
{
	mp_settings = CSettings::getInstance();
	m_sched_map.clear ();
}

CEventScheduleContainer::~CEventScheduleContainer (void)
{
	m_sched_map.clear ();
}


bool CEventScheduleContainer::addScheduleMap (const SERVICE_KEY_t &key, std::vector <CEvent*> *p_sched)
{
	if (!p_sched || p_sched->size() == 0) {
		return false;
	}

//	m_sched_map.insert (pair<SERVICE_KEY_t, std::vector <CEvent*> *>(key, p_sched));
	std::vector<std::unique_ptr<CEvent>> *_p_sched = new std::vector<std::unique_ptr<CEvent>>;
	for (const auto e : *p_sched) {
		std::unique_ptr<CEvent> up_e (e);
		_p_sched->push_back (std::move(up_e));
	}
	std::unique_ptr<std::vector<std::unique_ptr<CEvent>>> _up_sched(_p_sched);
	m_sched_map.insert (std::make_pair(key, std::move(_up_sched)));

	return true;
}

void CEventScheduleContainer::deleteScheduleMap (const SERVICE_KEY_t &key)
{
//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
	const auto iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return ;
	}

//	std::vector <CEvent*> *p_sched = iter->second;
	std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();

//	clearSchedule (p_sched);
	clearSchedule (*p_sched);

//	delete p_sched;

	m_sched_map.erase (iter);
}

bool CEventScheduleContainer::hasScheduleMap (const SERVICE_KEY_t &key) const
{
//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
	const auto iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return false;

	} else {
//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (!p_sched || (p_sched->size() == 0)) {
			return false;
		}
	}

	return true;
}

void CEventScheduleContainer::dumpScheduleMap (void) const
{
//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.begin ();
	auto iter = m_sched_map.cbegin ();

	if (iter == m_sched_map.end()) {
		return ;
	}

	for (; iter != m_sched_map.end(); ++ iter) {
		SERVICE_KEY_t key = iter->first;
		key.dump();

//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (p_sched) {
			_UTL_LOG_I ("  [%d] items", p_sched->size());
		}
	}
}

void CEventScheduleContainer::dumpScheduleMap (const SERVICE_KEY_t &key) const
{
	if (!hasScheduleMap (key)) {
		_UTL_LOG_I ("not hasScheduleMap...");
		return ;
	}

//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
	const auto iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return ;

	} else {
//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (!p_sched || (p_sched->size() == 0)) {
			return ;

		} else {
//			dumpSchedule (p_sched);
			dumpSchedule (*p_sched);
		}
	}
}

const CEvent *CEventScheduleContainer::getEvent (const SERVICE_KEY_t &key, uint16_t event_id) const
{
	if (!hasScheduleMap (key)) {
		_UTL_LOG_I ("not hasScheduleMap...");
		return NULL;
	}


	CEvent *r = NULL;
//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
	const auto iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return NULL;

	} else {
//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (p_sched->size() == 0) {
			return NULL;

		} else {
//			std::vector<CEvent*>::const_iterator iter_event = p_sched->begin();
			std::vector<std::unique_ptr<CEvent>>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
//				CEvent* p = *iter_event;
				CEvent* p = iter_event->get();
				if (
					(p->transport_stream_id == key.transport_stream_id) &&
					(p->original_network_id == key.original_network_id) &&
					(p->service_id == key.service_id) &&
					(p->event_id == event_id)
				) {
					r = p;
					break;
				}
			}
		}
	}

	return r;
}

const CEvent *CEventScheduleContainer::getEvent (const SERVICE_KEY_t &key, int index) const
{
	if (!hasScheduleMap (key)) {
		_UTL_LOG_I ("not hasScheduleMap...");
		return NULL;
	}


	CEvent *r = NULL;
//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
	const auto iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return NULL;

	} else {
//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (p_sched->size() == 0) {
			return NULL;

		} else {
			int _idx = 0;
//			std::vector<CEvent*>::const_iterator iter_event = p_sched->begin();
			std::vector<std::unique_ptr<CEvent>>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				if (index == _idx) {
//					CEvent* p = *iter_event;
					CEvent* p = iter_event->get();
					r = p;
					break;
				}

				++ _idx;
			}
		}
	}

	return r;
}

int CEventScheduleContainer::getEvents (
	const char *p_keyword,
	CEventScheduleManagerIf::EVENT_t *p_out_events,
	int out_array_num,
	bool is_check_extendedEvent
) const
{
	if (!p_keyword || !p_out_events || out_array_num <= 0) {
		return -1;
	}

//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.begin ();
	auto iter = m_sched_map.cbegin ();
	if (iter == m_sched_map.end()) {
		return 0;
	}

	int n = 0;

	for (; iter != m_sched_map.end(); ++ iter) {
		if (out_array_num == 0) {
			break;
		}

//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (p_sched) {

//			std::vector<CEvent*>::const_iterator iter_event = p_sched->begin();
			std::vector<std::unique_ptr<CEvent>>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				if (out_array_num == 0) {
					break;
				}

//				CEvent* p_event = *iter_event;
				CEvent* p_event = iter_event->get();
				if (p_event) {

					char *s_evt_name = NULL;
					char *s_evt_text = NULL;
					char *s_ex_item_desc = NULL;
					char *s_ex_item = NULL;

					if (is_check_extendedEvent) {

						// check event text
						s_evt_text = strstr ((char*)p_event->text.c_str(), p_keyword);

						// check extened event
						std::vector<CEvent::CExtendedInfo>::const_iterator iter_ex = p_event->extendedInfos.begin();
						for (; iter_ex != p_event->extendedInfos.end(); ++ iter_ex) {
							s_ex_item_desc = strstr ((char*)iter_ex->item_description.c_str(), p_keyword);
							s_ex_item = strstr ((char*)iter_ex->item.c_str(), p_keyword);
							if (s_ex_item_desc || s_ex_item) {
								break;
							}
						}

						// debug dump
						if (s_evt_text || s_ex_item_desc || s_ex_item) {
							_UTL_LOG_I ("====================================");
							_UTL_LOG_I ("====  keyword:[%s]  ====", p_keyword);
							_UTL_LOG_I ("====================================");
							p_event->dump();
							p_event->dump_detail();
						}

					} else {

						// check event name
						s_evt_name = strstr ((char*)p_event->event_name.c_str(), p_keyword);
					}

					// check result
					if (s_evt_name || s_evt_text || s_ex_item_desc || s_ex_item) {

						p_out_events->transport_stream_id = p_event->transport_stream_id;
						p_out_events->original_network_id = p_event->original_network_id;
						p_out_events->service_id = p_event->service_id;

						p_out_events->event_id = p_event->event_id;
						p_out_events->start_time = p_event->start_time;
						p_out_events->end_time = p_event->end_time;

						// アドレスで渡してますが 基本的には schedule casheが走らない限り
						// アドレスは変わらない前提です
						p_out_events->p_event_name = &p_event->event_name;
						p_out_events->p_text = &p_event->text;

						++ p_out_events;
						-- out_array_num;
						++ n;
					}
				}
			}
		}
	}

	return n;
}

//void CEventScheduleContainer::clearSchedule (std::vector <CEvent*> *p_sched)
void CEventScheduleContainer::clearSchedule (std::vector<std::unique_ptr<CEvent>> &sched)
{
//	if (!p_sched || p_sched->size() == 0) {
	if (sched.size() == 0) {
		return;
	}

//	std::vector<CEvent*>::const_iterator iter = p_sched->begin();
//	for (; iter != p_sched->end(); ++ iter) {
//		CEvent* p = *iter;
//		if (p) {
//			p->clear();
//			delete p;
//		}
//	}

//	p_sched->clear();
	sched.clear();
}

//void CEventScheduleContainer::dumpSchedule (const std::vector <CEvent*> *p_sched) const
void CEventScheduleContainer::dumpSchedule (const std::vector<std::unique_ptr<CEvent>> &sched) const
{
//	if (!p_sched || p_sched->size() == 0) {
	if (sched.size() == 0) {
		return;
	}

	int i = 0;
//	std::vector<CEvent*>::const_iterator iter = p_sched->begin();
	std::vector<std::unique_ptr<CEvent>>::const_iterator iter = sched.begin();
//	for (; iter != p_sched->end(); ++ iter) {
	for (; iter != sched.end(); ++ iter) {
//		CEvent* p = *iter;
		CEvent* p = (*iter).get();
		if (p) {
			_UTL_LOG_I ("-----------------------------------");
			_UTL_LOG_I ("[[[ %d ]]]", i);
			_UTL_LOG_I ("-----------------------------------");

			p->dump();

			++ i;
		}
	}
}

void CEventScheduleContainer::clear (void)
{
//	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.begin ();
	auto iter = m_sched_map.cbegin ();

	if (iter == m_sched_map.end()) {
		m_sched_map.clear();
		return ;
	}

	for (; iter != m_sched_map.end(); ++ iter) {
		SERVICE_KEY_t key = iter->first;
		deleteScheduleMap (key);
		iter = m_sched_map.begin (); // renew iter
	}

	m_sched_map.clear();
}
