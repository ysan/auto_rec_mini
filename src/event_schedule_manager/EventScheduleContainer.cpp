#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "EventScheduleContainer.h"


CEventScheduleContainer::CEventScheduleContainer (void)
	:mp_settings (NULL)
{
	mp_settings = CSettings::getInstance();
	m_sched_map_json_path.clear();
	m_sched_map.clear ();
}

CEventScheduleContainer::~CEventScheduleContainer (void)
{
	mp_settings = NULL;
	m_sched_map_json_path.clear();
	m_sched_map.clear ();
}


void CEventScheduleContainer::set_schedule_map_json_path (std::string sched_map_json_path)
{
	m_sched_map_json_path = sched_map_json_path;
}

bool CEventScheduleContainer::add_schedule_map (const service_key_t &key, std::vector <CEvent*> *p_sched)
{
	if (!p_sched || p_sched->size() == 0) {
		return false;
	}

//	m_sched_map.insert (pair<service_key_t, std::vector <CEvent*> *>(key, p_sched));
	std::vector<std::unique_ptr<CEvent>> *_p_sched = new std::vector<std::unique_ptr<CEvent>>;
	for (const auto e : *p_sched) {
		std::unique_ptr<CEvent> up_e (e);
		_p_sched->push_back (std::move(up_e));
	}
	std::unique_ptr<std::vector<std::unique_ptr<CEvent>>> _up_sched(_p_sched);
	m_sched_map.insert (std::make_pair(key, std::move(_up_sched)));

	return true;
}

void CEventScheduleContainer::delete_schedule_map (const service_key_t &key)
{
//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
	const auto iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return ;
	}

//	std::vector <CEvent*> *p_sched = iter->second;
	std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();

//	clear_schedule (p_sched);
	clear_schedule (*p_sched);

//	delete p_sched;

	m_sched_map.erase (iter);
}

bool CEventScheduleContainer::has_schedule_map (const service_key_t &key) const
{
//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
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

void CEventScheduleContainer::dump_schedule_map (void) const
{
//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.begin ();
	auto iter = m_sched_map.cbegin ();

	if (iter == m_sched_map.end()) {
		return ;
	}

	for (; iter != m_sched_map.end(); ++ iter) {
		service_key_t key = iter->first;
		key.dump();

//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (p_sched) {
			_UTL_LOG_I ("  [%d] items", p_sched->size());
		}
	}
}

void CEventScheduleContainer::dump_schedule_map (const service_key_t &key) const
{
	if (!has_schedule_map (key)) {
		_UTL_LOG_I ("not has_schedule_map...");
		return ;
	}

//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
	const auto iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return ;

	} else {
//		std::vector <CEvent*> *p_sched = iter->second;
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (!p_sched || (p_sched->size() == 0)) {
			return ;

		} else {
//			dump_schedule (p_sched);
			dump_schedule (*p_sched);
		}
	}
}

const CEvent *CEventScheduleContainer::get_event (const service_key_t &key, uint16_t event_id) const
{
	if (!has_schedule_map (key)) {
		_UTL_LOG_I ("not has_schedule_map...");
		return NULL;
	}


	CEvent *r = NULL;
//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
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

const CEvent *CEventScheduleContainer::get_event (const service_key_t &key, int index) const
{
	if (!has_schedule_map (key)) {
		_UTL_LOG_I ("not has_schedule_map...");
		return NULL;
	}


	CEvent *r = NULL;
//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);
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

int CEventScheduleContainer::get_events (
	const char *p_keyword,
	CEventScheduleManagerIf::event_t *p_out_events,
	int out_array_num,
	bool is_check_extended_event
) const
{
	if (!p_keyword || !p_out_events || out_array_num <= 0) {
		return -1;
	}

//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.begin ();
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

					if (is_check_extended_event) {

						// check event text
						s_evt_text = strstr ((char*)p_event->text.c_str(), p_keyword);

						// check extened event
						std::vector<CEvent::CExtendedInfo>::const_iterator iter_ex = p_event->extended_infos.begin();
						for (; iter_ex != p_event->extended_infos.end(); ++ iter_ex) {
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

//void CEventScheduleContainer::clear_schedule (std::vector <CEvent*> *p_sched)
void CEventScheduleContainer::clear_schedule (std::vector<std::unique_ptr<CEvent>> &sched)
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

//void CEventScheduleContainer::dump_schedule (const std::vector <CEvent*> *p_sched) const
void CEventScheduleContainer::dump_schedule (const std::vector<std::unique_ptr<CEvent>> &sched) const
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
//	std::map <service_key_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.begin ();
	auto iter = m_sched_map.cbegin ();

	if (iter == m_sched_map.end()) {
		m_sched_map.clear();
		return ;
	}

	for (; iter != m_sched_map.end(); ++ iter) {
		service_key_t key = iter->first;
		delete_schedule_map (key);
		iter = m_sched_map.begin (); // renew iter
	}

	m_sched_map.clear();
}

void CEventScheduleContainer::save_schedule_map (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_sched_map));
	}

	if (m_sched_map_json_path.length() == 0) {
		_UTL_LOG_E("m_sched_map_json_path.length 0.");
		return;
	}

	std::ofstream ofs (m_sched_map_json_path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CEventScheduleContainer::load_schedule_map (void)
{
	if (m_sched_map_json_path.length() == 0) {
		_UTL_LOG_I("m_sched_map_json_path.length 0.");
		return;
	}

	std::ifstream ifs (m_sched_map_json_path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("[%s] is not found.", m_sched_map_json_path.c_str());
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_sched_map));

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tv_nsecに書いてるので to_string用の文字はここで作ります
	auto iter = m_sched_map.cbegin ();
	for (; iter != m_sched_map.end(); ++ iter) {
		std::vector <std::unique_ptr<CEvent>> *p_sched = iter->second.get();
		if (p_sched) {
			auto iter_event = p_sched->cbegin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				CEvent* p_event = iter_event->get();
				if (p_event) {
					p_event->start_time.updateStrings();
					p_event->end_time.updateStrings();
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, struct _service_key &k)
{
	archive (
		cereal::make_nvp("transport_stream_id", k.transport_stream_id)
		,cereal::make_nvp("original_network_id", k.original_network_id)
		,cereal::make_nvp("service_id", k.service_id)
		,cereal::make_nvp("service_type", k.service_type)
		,cereal::make_nvp("service_name", k.service_name)
	);
}

template <class Archive>
void serialize (Archive &archive, struct timespec &t)
{
	archive (
		cereal::make_nvp("tv_sec", t.tv_sec)
		,cereal::make_nvp("tv_nsec", t.tv_nsec)
	);
}

template <class Archive>
void serialize (Archive &archive, CEtime &t)
{
	archive (
		cereal::make_nvp("m_time", t.m_time)
	);
}

template <class Archive>
void serialize (Archive &archive, CEvent::CExtendedInfo &ex)
{
	archive (
		cereal::make_nvp("item_description", ex.item_description)
		,cereal::make_nvp("item", ex.item)
	);
}

template <class Archive>
void serialize (Archive &archive, CEvent::CGenre &g)
{
	archive (
		cereal::make_nvp("content_nibble_level_1", g.content_nibble_level_1)
		,cereal::make_nvp("content_nibble_level_2", g.content_nibble_level_2)
	);
}

template <class Archive>
void serialize (Archive &archive, CEvent &e)
{
	archive (
		cereal::make_nvp("table_id", e.table_id)
		,cereal::make_nvp("transport_stream_id", e.transport_stream_id)
		,cereal::make_nvp("original_network_id", e.original_network_id)
		,cereal::make_nvp("service_id", e.service_id)
		,cereal::make_nvp("section_number", e.section_number)
		,cereal::make_nvp("event_id", e.event_id)
		,cereal::make_nvp("start_time", e.start_time)
		,cereal::make_nvp("end_time", e.end_time)
		,cereal::make_nvp("event_name", e.event_name)
		,cereal::make_nvp("text", e.text)
		,cereal::make_nvp("component_type", e.component_type)
		,cereal::make_nvp("component_tag", e.component_tag)
		,cereal::make_nvp("audio_component_type", e.audio_component_type)
		,cereal::make_nvp("audio_component_tag", e.audio_component_tag)
		,cereal::make_nvp("ES_multi_lingual_flag", e.ES_multi_lingual_flag)
		,cereal::make_nvp("main_component_flag", e.main_component_flag)
		,cereal::make_nvp("quality_indicator", e.quality_indicator)
		,cereal::make_nvp("sampling_rate", e.sampling_rate)
		,cereal::make_nvp("genres", e.genres)
		,cereal::make_nvp("extended_infos", e.extended_infos)
	);
}
