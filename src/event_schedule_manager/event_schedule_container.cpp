#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "event_schedule_container.h"


namespace arec {
namespace event_schedule {


container::container (void)
	:settings_ (NULL)
{
	settings_ = arec::settings::get_instance();
	sched_map_json_path_.clear();
	sched_map_.clear ();
}

container::~container (void)
{
	settings_ = NULL;
	sched_map_json_path_.clear();
	sched_map_.clear ();
}


void container::set_schedule_map_json_path (std::string sched_map_json_path)
{
	sched_map_json_path_ = sched_map_json_path;
}

bool container::add_schedule_map (const service_key_t &key, std::vector <event*> *p_sched)
{
	if (!p_sched || p_sched->size() == 0) {
		return false;
	}

//	sched_map_.insert (pair<service_key_t, std::vector <event*> *>(key, p_sched));
	std::vector<std::unique_ptr<event>> *_p_sched = new std::vector<std::unique_ptr<event>>;
	for (const auto e : *p_sched) {
		std::unique_ptr<event> up_e (e);
		_p_sched->push_back (std::move(up_e));
	}
	std::unique_ptr<std::vector<std::unique_ptr<event>>> _up_sched(_p_sched);
	sched_map_.insert (std::make_pair(key, std::move(_up_sched)));

	return true;
}

void container::delete_schedule_map (const service_key_t &key)
{
//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.find (key);
	const auto iter = sched_map_.find (key);

	if (iter == sched_map_.end()) {
		return ;
	}

//	std::vector <event*> *p_sched = iter->second;
	std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();

//	clear_schedule (p_sched);
	clear_schedule (*p_sched);

//	delete p_sched;

	sched_map_.erase (iter);
}

bool container::has_schedule_map (const service_key_t &key) const
{
//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.find (key);
	const auto iter = sched_map_.find (key);

	if (iter == sched_map_.end()) {
		return false;

	} else {
//		std::vector <event*> *p_sched = iter->second;
		std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();
		if (!p_sched || (p_sched->size() == 0)) {
			return false;
		}
	}

	return true;
}

void container::dump_schedule_map (void) const
{
//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.begin ();
	auto iter = sched_map_.cbegin ();

	if (iter == sched_map_.end()) {
		return ;
	}

	for (; iter != sched_map_.end(); ++ iter) {
		service_key_t key = iter->first;
		key.dump();

//		std::vector <event*> *p_sched = iter->second;
		std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();
		if (p_sched) {
			_UTL_LOG_I ("  [%d] items", p_sched->size());
		}
	}
}

void container::dump_schedule_map (const service_key_t &key) const
{
	if (!has_schedule_map (key)) {
		_UTL_LOG_I ("not has_schedule_map...");
		return ;
	}

//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.find (key);
	const auto iter = sched_map_.find (key);

	if (iter == sched_map_.end()) {
		return ;

	} else {
//		std::vector <event*> *p_sched = iter->second;
		std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();
		if (!p_sched || (p_sched->size() == 0)) {
			return ;

		} else {
//			dump_schedule (p_sched);
			dump_schedule (*p_sched);
		}
	}
}

const event *container::get_event (const service_key_t &key, uint16_t event_id) const
{
	if (!has_schedule_map (key)) {
		_UTL_LOG_I ("not has_schedule_map...");
		return NULL;
	}


	event *r = NULL;
//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.find (key);
	const auto iter = sched_map_.find (key);

	if (iter == sched_map_.end()) {
		return NULL;

	} else {
//		std::vector <event*> *p_sched = iter->second;
		std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();
		if (p_sched->size() == 0) {
			return NULL;

		} else {
//			std::vector<event*>::const_iterator iter_event = p_sched->begin();
			std::vector<std::unique_ptr<event>>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
//				event* p = *iter_event;
				event* p = iter_event->get();
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

const event *container::get_event (const service_key_t &key, int index) const
{
	if (!has_schedule_map (key)) {
		_UTL_LOG_I ("not has_schedule_map...");
		return NULL;
	}


	event *r = NULL;
//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.find (key);
	const auto iter = sched_map_.find (key);

	if (iter == sched_map_.end()) {
		return NULL;

	} else {
//		std::vector <event*> *p_sched = iter->second;
		std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();
		if (p_sched->size() == 0) {
			return NULL;

		} else {
			int _idx = 0;
//			std::vector<event*>::const_iterator iter_event = p_sched->begin();
			std::vector<std::unique_ptr<event>>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				if (index == _idx) {
//					event* p = *iter_event;
					event* p = iter_event->get();
					r = p;
					break;
				}

				++ _idx;
			}
		}
	}

	return r;
}

int container::get_events (
	const char *p_keyword,
	client_iface::event_t *p_out_events,
	int out_array_num,
	bool is_check_extended_event
) const
{
	if (!p_keyword || !p_out_events || out_array_num <= 0) {
		return -1;
	}

//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.begin ();
	auto iter = sched_map_.cbegin ();
	if (iter == sched_map_.end()) {
		return 0;
	}

	int n = 0;

	for (; iter != sched_map_.end(); ++ iter) {
		if (out_array_num == 0) {
			break;
		}

//		std::vector <event*> *p_sched = iter->second;
		std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();
		if (p_sched) {

//			std::vector<event*>::const_iterator iter_event = p_sched->begin();
			std::vector<std::unique_ptr<event>>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				if (out_array_num == 0) {
					break;
				}

//				event* p_event = *iter_event;
				event* p_event = iter_event->get();
				if (p_event) {

					char *s_evt_name = NULL;
					char *s_evt_text = NULL;
					char *s_ex_item_desc = NULL;
					char *s_ex_item = NULL;

					if (is_check_extended_event) {

						// check event text
						s_evt_text = strstr ((char*)p_event->text.c_str(), p_keyword);

						// check extened event
						std::vector<event::extended_info>::const_iterator iter_ex = p_event->extended_infos.begin();
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

//void container::clear_schedule (std::vector <event*> *p_sched)
void container::clear_schedule (std::vector<std::unique_ptr<event>> &sched)
{
//	if (!p_sched || p_sched->size() == 0) {
	if (sched.size() == 0) {
		return;
	}

//	std::vector<event*>::const_iterator iter = p_sched->begin();
//	for (; iter != p_sched->end(); ++ iter) {
//		event* p = *iter;
//		if (p) {
//			p->clear();
//			delete p;
//		}
//	}

//	p_sched->clear();
	sched.clear();
}

//void container::dump_schedule (const std::vector <event*> *p_sched) const
void container::dump_schedule (const std::vector<std::unique_ptr<event>> &sched) const
{
//	if (!p_sched || p_sched->size() == 0) {
	if (sched.size() == 0) {
		return;
	}

	int i = 0;
//	std::vector<event*>::const_iterator iter = p_sched->begin();
	std::vector<std::unique_ptr<event>>::const_iterator iter = sched.begin();
//	for (; iter != p_sched->end(); ++ iter) {
	for (; iter != sched.end(); ++ iter) {
//		event* p = *iter;
		event* p = (*iter).get();
		if (p) {
			_UTL_LOG_I ("-----------------------------------");
			_UTL_LOG_I ("[[[ %d ]]]", i);
			_UTL_LOG_I ("-----------------------------------");

			p->dump();

			++ i;
		}
	}
}

void container::clear (void)
{
//	std::map <service_key_t, std::vector <event*> *> ::const_iterator iter = sched_map_.begin ();
	auto iter = sched_map_.cbegin ();

	if (iter == sched_map_.end()) {
		sched_map_.clear();
		return ;
	}

	for (; iter != sched_map_.end(); ++ iter) {
		service_key_t key = iter->first;
		delete_schedule_map (key);
		iter = sched_map_.begin (); // renew iter
	}

	sched_map_.clear();
}

void container::save_schedule_map (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_sched_map", sched_map_));
	}

	if (sched_map_json_path_.length() == 0) {
		_UTL_LOG_E("sched_map_json_path_.length 0.");
		return;
	}

	std::ofstream ofs (sched_map_json_path_.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void container::load_schedule_map (void)
{
	if (sched_map_json_path_.length() == 0) {
		_UTL_LOG_I("sched_map_json_path_.length 0.");
		return;
	}

	std::ifstream ifs (sched_map_json_path_.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("[%s] is not found.", sched_map_json_path_.c_str());
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_sched_map", sched_map_));

	ifs.close();
	ss.clear();


	// etimeの値は直接 tv_sec,tv_nsecに書いてるので to_string用の文字はここで作ります
	auto iter = sched_map_.cbegin ();
	for (; iter != sched_map_.end(); ++ iter) {
		std::vector <std::unique_ptr<event>> *p_sched = iter->second.get();
		if (p_sched) {
			auto iter_event = p_sched->cbegin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				event* p_event = iter_event->get();
				if (p_event) {
					p_event->start_time.update_strings();
					p_event->end_time.update_strings();
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

} // namespace event_schedule
} // namespace arec

template <class Archive>
void serialize (Archive &archive, struct timespec &t)
{
	archive (
		cereal::make_nvp("tv_sec", t.tv_sec)
		,cereal::make_nvp("tv_nsec", t.tv_nsec)
	);
}

namespace arec { namespace common {
template <class Archive>
void serialize (Archive &archive, arec::common::etime &t)
{
	archive (
		cereal::make_nvp("m_time", t.time_)
	);
}
}} // namespace arec::common

namespace arec {
namespace event_schedule {

template <class Archive>
void serialize (Archive &archive, event::extended_info &ex)
{
	archive (
		cereal::make_nvp("item_description", ex.item_description)
		,cereal::make_nvp("item", ex.item)
	);
}

template <class Archive>
void serialize (Archive &archive, event::genre &g)
{
	archive (
		cereal::make_nvp("content_nibble_level_1", g.content_nibble_level_1)
		,cereal::make_nvp("content_nibble_level_2", g.content_nibble_level_2)
	);
}

template <class Archive>
void serialize (Archive &archive, event &e)
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

} // namespace event_schedule
} // namespace arec
