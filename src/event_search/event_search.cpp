#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "event_schedule_manager_if.h"
#include "event_search.h"
#include "event_search_if.h"
#include "rec_manager_if.h"
#include "modules.h"

#include "settings.h"


namespace arec {
namespace event_search {

manager::manager (std::string name, uint8_t que_max)
	:coseq::module_base (name, que_max)
	,cache_sched_state_notify_client_id_ (0xff)
{
	const int _max = static_cast<int>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){on_module_up(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){on_module_down(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){on_add_rec_reserve_keyword_search(iface);}, std::move("on_add_rec_reserve_keyword_search")},
		{[&](coseq::iface *iface){on_add_rec_reserve_keyword_search(iface);}, std::move("on_add_rec_reserve_keyword_search(ex)")},
		{[&](coseq::iface *iface){on_dump_histories(iface);}, std::move("on_dump_histories")},
		{[&](coseq::iface *iface){on_dump_histories(iface);}, std::move("on_dump_histories(ex)")},
	};
	set_sequences (seqs, _max);


	event_name_keywords_.clear();
	extended_event_keywords_.clear();

	event_name_search_histories_.clear();
	extended_event_search_histories_.clear();
}

manager::~manager (void)
{
	reset_sequences();
}


void manager::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	std::string keywords_path = arec::settings::get_instance()->get_params().get_event_name_keywords_json_path();
	arec::common::utils::makedir(keywords_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	std::string exkeywords_path = arec::settings::get_instance()->get_params().get_extended_event_keywords_json_path();
	arec::common::utils::makedir(exkeywords_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	std::string history_path = arec::settings::get_instance()->get_params().get_event_name_search_histories_json_path();
	arec::common::utils::makedir(history_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	std::string exhistory_path = arec::settings::get_instance()->get_params().get_extended_event_search_histories_json_path();
	arec::common::utils::makedir(exhistory_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	load_event_name_search_histories ();
	load_extended_event_search_histories ();


	// キャッシュスケジュール状態の notify を登録(client_id を保持)
	coseq::source &r = iface->request (
		static_cast<uint8_t>(modules::module_id::event_schedule_manager),
		static_cast<uint8_t>(arec::event_schedule::client_iface::sequence::reg_cache_schedule_state_notify)
	);
	if (r.get_result() != coseq::result::success) {
		_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
		iface->reply (coseq::result::error);
		return;
	}

	cache_sched_state_notify_client_id_ = *(uint8_t*)(r.get_message().data());
	iface->reply (coseq::result::success);
}

void manager::on_module_down (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

//
// do something
//

	iface->reply (coseq::result::success);
}

void manager::on_add_rec_reserve_keyword_search (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 別々の seq idx で同一関数を共有するため lock で直列化(v1同様)
	iface->lock();

	// 実行専用スクラッチはローカルへ(coseq 多重インスタンス対策。
	// s_events は他モジュールへ出力ポインタを渡すため特にローカル必須)
	history current_history;
	current_history.clear();

	bool is_ex = (iface->get_source().get_sequence_idx() == static_cast<int>(client_iface::sequence::add_rec_reserve_keyword_search_ex));

	const std::vector<std::string> *p_keywords = nullptr;
	if (!is_ex) {
		load_event_name_keywords ();
		dump_event_name_keywords ();
		p_keywords = &event_name_keywords_;
	} else {
		load_extended_event_keywords ();
		dump_extended_event_keywords ();
		p_keywords = &extended_event_keywords_;
	}

	for (auto iter = p_keywords->begin(); iter != p_keywords->end(); ++ iter) {

		// history ----------------
		history::keyword current_history_keyword;
		current_history_keyword.clear();
		current_history_keyword.keyword_string = *iter;

		arec::event_schedule::client_iface::event_t events [30];
		memset (events, 0x00, sizeof(events));

		arec::event_schedule::client_iface::request_event_param_t _param;
		_param.arg.p_keyword = iter->c_str();
		_param.p_out_event = events;
		_param.array_max_num = 30;

		// get events (keyword / keyword_ex)
		int get_events_num = 0;
		{
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<uint8_t>(is_ex ? arec::event_schedule::client_iface::sequence::get_events__keyword_search_ex
				                           : arec::event_schedule::client_iface::sequence::get_events__keyword_search),
				(uint8_t*)&_param, sizeof (_param)
			);
			if (r.get_result() == coseq::result::success) {
				get_events_num = *(int*)(r.get_message().data());
				if (get_events_num > 30) {
					_UTL_LOG_W ("trancate get_events_num 30");
					get_events_num = 30;
				}
			} else {
				get_events_num = 0;
			}
		}

		for (int events_idx = 0; events_idx < get_events_num; ++ events_idx) {

			arec::common::etime cur;
			cur.set_current_time();
			if (cur > events [events_idx].end_time) {
				// 終了時間が過ぎていたら録画予約入れません
				continue;
			}

			// 一度予約消してから入れ直します(結果は見ない)
			{
				arec::rec::client_iface::remove_reserve_param_t _rparam;
				_rparam.arg.key.transport_stream_id = events [events_idx].transport_stream_id;
				_rparam.arg.key.original_network_id = events [events_idx].original_network_id;
				_rparam.arg.key.service_id = events [events_idx].service_id;
				_rparam.arg.key.event_id = events [events_idx].event_id;
				_rparam.is_consider_repeatability = false;
				_rparam.is_apply_result = false; // result に記録しない

				iface->request (
					static_cast<uint8_t>(modules::module_id::rec_manager),
					static_cast<uint8_t>(arec::rec::client_iface::sequence::remove_reserve),
					(uint8_t*)&_rparam, sizeof (_rparam)
				);
			}

			// history ----------------
			history::event _event;
			_event.transport_stream_id = events [events_idx].transport_stream_id;
			_event.original_network_id = events [events_idx].original_network_id;
			_event.service_id = events [events_idx].service_id;
			_event.event_id = events [events_idx].event_id;
			_event.start_time = events [events_idx].start_time;
			_event.end_time = events [events_idx].end_time;
			_event.event_name = *(events [events_idx].p_event_name);
			current_history_keyword.events.push_back(_event);

			// add reserve (結果は見ない)
			{
				arec::rec::client_iface::add_reserve_param_t _aparam;
				_aparam.transport_stream_id = events [events_idx].transport_stream_id;
				_aparam.original_network_id = events [events_idx].original_network_id;
				_aparam.service_id = events [events_idx].service_id;
				_aparam.event_id = events [events_idx].event_id;
				_aparam.repeatablity = arec::rec::client_iface::reserve_repeatability::none;

				iface->request (
					static_cast<uint8_t>(modules::module_id::rec_manager),
					static_cast<uint8_t>(arec::rec::client_iface::sequence::add_reserve_event),
					(uint8_t*)&_aparam, sizeof (_aparam)
				);
			}
		}

		// history ----------------
		// ※v1 踏襲: keyword を history に push するのは「次の keyword がある」場合のみ
		//   (=最後の keyword は history に記録されない v1 の挙動をそのまま維持)
		auto next = iter;
		++ next;
		if (next != p_keywords->end()) {
			current_history.keywords.push_back(current_history_keyword);
			arec::common::etime _t;
			_t.set_current_time();
			current_history.timestamp = _t;
		}
	}

	// END
	if (!is_ex) {
		push_histories (current_history, event_name_search_histories_);
		save_event_name_search_histories();
	} else {
		push_histories (current_history, extended_event_search_histories_);
		save_extended_event_search_histories();
	}

	iface->unlock();

	iface->reply (coseq::result::success);
}

void manager::on_dump_histories (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	if (iface->get_source().get_sequence_idx() == static_cast<int>(client_iface::sequence::dump_histories)) {
		dump_histories (event_name_search_histories_);
	} else {
		// client_iface::sequence::dump_histories_ex
		dump_histories (extended_event_search_histories_);
	}


	iface->reply (coseq::result::success);
}

void manager::on_receive_notify (coseq::iface *iface)
{
	if (iface->get_source().get_client_id() != cache_sched_state_notify_client_id_) {
		return ;
	}

	arec::event_schedule::client_iface::cache_schedule_state state = *(arec::event_schedule::client_iface::cache_schedule_state*)(iface->get_source().get_message().data());
	switch (state) {
		case arec::event_schedule::client_iface::cache_schedule_state::busy:
		break;

		case arec::event_schedule::client_iface::cache_schedule_state::ready: {
		// EPG取得が終わったら キーワード予約をかけます(返信不要 fire-and-forget)
		iface->send (static_cast<uint8_t>(modules::module_id::event_search), static_cast<uint8_t>(client_iface::sequence::add_rec_reserve_keyword_search));
		iface->send (static_cast<uint8_t>(modules::module_id::event_search), static_cast<uint8_t>(client_iface::sequence::add_rec_reserve_keyword_search_ex));

		}
		break;

	default:
		break;
	}


}


void manager::dump_event_name_keywords (void) const
{
	_UTL_LOG_I ("----- event name keywords -----");
	std::vector<std::string>::const_iterator iter = event_name_keywords_.begin();
	for (; iter != event_name_keywords_.end(); ++ iter) {
		_UTL_LOG_I ("  [%s]", iter->c_str());
	}
}

void manager::dump_extended_event_keywords (void) const
{
	_UTL_LOG_I ("----- extended event keywords -----");
	std::vector<std::string>::const_iterator iter = extended_event_keywords_.begin();
	for (; iter != extended_event_keywords_.end(); ++ iter) {
		_UTL_LOG_I ("  [%s]", iter->c_str());
	}
}

void manager::push_histories (const history &h, std::vector<history> &histories)
{
	// fifo 30
	// pop -> delete
	while (histories.size() >= 30) {
		histories.erase (histories.begin());
	}

	// push
	histories.push_back (h);

	_UTL_LOG_I ("push_histories  size=[%d]", histories.size());
}

void manager::dump_histories (const std::vector<history> &histories) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (const auto & history : histories) {
		_UTL_LOG_I ("---------------------------------------");
		history.dump();
	}
}

void manager::save_event_name_keywords (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_event_name_keywords", event_name_keywords_));
	}

	std::string path = arec::settings::get_instance()->get_params().get_event_name_keywords_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_event_name_keywords (void)
{
	std::string path = arec::settings::get_instance()->get_params().get_event_name_keywords_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("event_name_keywords.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_event_name_keywords", event_name_keywords_));

	ifs.close();
	ss.clear();
}

void manager::save_extended_event_keywords (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_extended_event_keywords", extended_event_keywords_));
	}

	std::string path = arec::settings::get_instance()->get_params().get_extended_event_keywords_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_extended_event_keywords (void)
{
	std::string path = arec::settings::get_instance()->get_params().get_extended_event_keywords_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("extended_event_keywords.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_extended_event_keywords", extended_event_keywords_));

	ifs.close();
	ss.clear();
}

void manager::save_event_name_search_histories (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_event_name_search_histories", event_name_search_histories_));
	}

	std::string path = arec::settings::get_instance()->get_params().get_event_name_search_histories_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_event_name_search_histories (void)
{
	std::string path = arec::settings::get_instance()->get_params().get_event_name_search_histories_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("event_name_search_histories.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_event_name_search_histories", event_name_search_histories_));

	ifs.close();
	ss.clear();


	// etimeの値は直接 tv_sec,tv_nsecに書いてるので toString用の文字はここで作ります
	for (auto &history : event_name_search_histories_) {
		history.timestamp.update_strings();
		for (auto &keyword : history.keywords) {
			for (auto &event: keyword.events) {
				event.start_time.update_strings();
				event.end_time.update_strings();
			}
		}
	}
}

void manager::save_extended_event_search_histories (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_extended_event_search_histories", extended_event_search_histories_));
	}

	std::string path = arec::settings::get_instance()->get_params().get_extended_event_search_histories_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_extended_event_search_histories (void)
{
	std::string path = arec::settings::get_instance()->get_params().get_extended_event_search_histories_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("extended_event_search_histories.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_extended_event_search_histories", extended_event_search_histories_));

	ifs.close();
	ss.clear();


	// etimeの値は直接 tv_sec,tv_nsecに書いてるので toString用の文字はここで作ります
	for (auto &history : extended_event_search_histories_) {
		history.timestamp.update_strings();
		for (auto &keyword : history.keywords) {
			for (auto &event: keyword.events) {
				event.start_time.update_strings();
				event.end_time.update_strings();
			}
		}
	}
}


} // namespace event_search
} // namespace arec

//--------------------------------------------------------------------------------

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
	archive (cereal::make_nvp("m_time", t.time_));
}
}} // namespace arec::common

namespace arec {
namespace event_search {

template <class Archive>
void serialize (Archive &archive, history::event &e)
{
	archive (
		cereal::make_nvp("transport_stream_id", e.transport_stream_id)
		,cereal::make_nvp("original_network_id", e.original_network_id)
		,cereal::make_nvp("service_id", e.service_id)
		,cereal::make_nvp("event_id", e.event_id)
		,cereal::make_nvp("start_time", e.start_time)
		,cereal::make_nvp("end_time", e.end_time)
		,cereal::make_nvp("event_name", e.event_name)
	);
}

template <class Archive>
void serialize (Archive &archive, history::keyword &k)
{
	archive (
		cereal::make_nvp("keyword_string", k.keyword_string)
		,cereal::make_nvp("events", k.events)
	);
}

template <class Archive>
void serialize (Archive &archive, history &h)
{
	archive (
		cereal::make_nvp("keywords", h.keywords)
		,cereal::make_nvp("timestamp", h.timestamp)
	);
}

} // namespace event_search
} // namespace arec
