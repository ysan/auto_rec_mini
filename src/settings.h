#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <limits>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>

#include "utils.h"
#include "group.h"

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


namespace arec {

class settings
{
public:
	class parameters {
	public:
		friend class cereal::access;
		parameters (void) = default;
		virtual ~parameters (void) = default;


		bool is_syslog_output (void) {
			return is_syslog_output_;
		}
		uint16_t get_command_server_port (void) {
			return command_server_port_;
		}
		const std::vector<std::string> &get_tuner_hal_allocates (void) {
			return tuner_hal_allocates_;
		}
		const std::string &get_channels_json_path (void) {
			return channels_json_path_;
		}
		const std::string &get_rec_reserves_json_path (void) {
			return rec_reserves_json_path_;
		}
		const std::string &get_rec_results_json_path (void) {
			return rec_results_json_path_;
		}
		const std::string &get_rec_ts_path (void) {
			return rec_ts_path_;
		}
		int get_rec_disk_space_low_limit_MB (void) {
			return rec_disk_space_low_limit_MB_;
		}
		bool is_rec_use_splitter (void) {
			return rec_use_splitter_;
		}
		const std::string &get_dummy_tuner_ts_path (void) {
			return dummy_tuner_ts_path_;
		}
		int is_enable_event_schedule_cache (void) {
			return event_schedule_cache_is_enable_;
		}
		int get_event_schedule_cache_start_interval_day (void) {
			return event_schedule_cache_start_interval_day_;
		}
		int get_event_schedule_cache_start_hour (void) {
			return event_schedule_cache_start_hour_;
		}
		int get_event_schedule_cache_start_min (void) {
			return event_schedule_cache_start_min_;
		}
		int get_event_schedule_cache_timeout_min (void) {
			return event_schedule_cache_timeout_min_;
		}
		int get_event_schedule_cache_retry_interval_min (void) {
			return event_schedule_cache_retry_interval_min_;
		}
		const std::string &get_event_schedule_cache_data_json_path (void) {
			return event_schedule_cache_data_json_path_;
		}
		const std::string &get_event_schedule_cache_histories_json_path (void) {
			return event_schedule_cache_histories_json_path_;
		}
		const std::string &get_event_name_keywords_json_path (void) {
			return event_name_keywords_json_path_;
		}
		const std::string &get_extended_event_keywords_json_path (void) {
			return extended_event_keywords_json_path_;
		}
		const std::string &get_event_name_search_histories_json_path (void) {
			return event_name_search_histories_json_path_;
		}
		const std::string &get_extended_event_search_histories_json_path (void) {
			return extended_event_search_histories_json_path_;
		}
		const std::string &get_viewing_stream_data_path (void) {
			return viewing_stream_data_path_;
		}
		const std::string &get_viewing_stream_command_format (void) {
			return viewing_stream_command_format_;
		}
		bool is_viewing_use_splitter (void) {
			return viewing_use_splitter_;
		}
		uint16_t get_http_server_port (void) {
			return http_server_port_;
		}
		const std::string &get_http_server_static_contents_path (void) {
			return http_server_static_contents_path_;
		}
		const std::string &get_logo_path (void) {
			return logo_path_;
		}

		void dump (void) {
			_UTL_LOG_I ("--------------------------------");
			_UTL_LOG_I ("--          settings          --");
			_UTL_LOG_I ("--------------------------------");
			_UTL_LOG_I ("is_syslog_output:[%d]", is_syslog_output_);
			_UTL_LOG_I ("command_server_port:[%d]", command_server_port_);
			_UTL_LOG_I ("tuner_hal_allocates:");
			for (const std::string &s : tuner_hal_allocates_) {
				_UTL_LOG_I ("  [%s]", s.c_str());
			}
			_UTL_LOG_I ("channels_json_path:[%s]", channels_json_path_.c_str());
			_UTL_LOG_I ("rec_reserves_json_path:[%s]", rec_reserves_json_path_.c_str());
			_UTL_LOG_I ("rec_results_json_path:[%s]", rec_results_json_path_.c_str());
			_UTL_LOG_I ("rec_ts_path:[%s]", rec_ts_path_.c_str());
			_UTL_LOG_I ("rec_disk_space_low_limit_MB:[%d]", rec_disk_space_low_limit_MB_);
			_UTL_LOG_I ("rec_use_splitter:[%d]", rec_use_splitter_);
			_UTL_LOG_I ("dummy_tuner_ts_path:[%s]", dummy_tuner_ts_path_.c_str());
			_UTL_LOG_I ("event_schedule_cache_is_enable:[%d]", event_schedule_cache_is_enable_);
			_UTL_LOG_I ("event_schedule_cache_start_interval_day:[%d]", event_schedule_cache_start_interval_day_);
			_UTL_LOG_I ("event_schedule_cache_start_time:[%s]", event_schedule_cache_start_time_.c_str());
			_UTL_LOG_I ("event_schedule_cache_timeout_min:[%d]", event_schedule_cache_timeout_min_);
			_UTL_LOG_I ("event_schedule_cache_retry_interval_min:[%d]", event_schedule_cache_retry_interval_min_);
			_UTL_LOG_I ("event_schedule_cache_data_json_path:[%s]", event_schedule_cache_data_json_path_.c_str());
			_UTL_LOG_I ("event_schedule_cache_histories_json_path:[%s]", event_schedule_cache_histories_json_path_.c_str());
			_UTL_LOG_I ("event_name_keywords_json_path:[%s]", event_name_keywords_json_path_.c_str());
			_UTL_LOG_I ("extended_event_keywords_json_path:[%s]", extended_event_keywords_json_path_.c_str());
			_UTL_LOG_I ("event_name_search_histories_json_path:[%s]", event_name_search_histories_json_path_.c_str());
			_UTL_LOG_I ("extended_event_search_histories_json_path:[%s]", extended_event_search_histories_json_path_.c_str());
			_UTL_LOG_I ("viewing_stream_data_path:[%s]", viewing_stream_data_path_.c_str());
			_UTL_LOG_I ("viewing_stream_command_format:[%s]", viewing_stream_command_format_.c_str());
			_UTL_LOG_I ("viewing_use_splitter:[%d]", viewing_use_splitter_);
			_UTL_LOG_I ("http_server_port:[%d]", http_server_port_);
			_UTL_LOG_I ("http_server_static_contents_path:[%s]", http_server_static_contents_path_.c_str());
			_UTL_LOG_I ("logo_path:[%s]", logo_path_.c_str());
			_UTL_LOG_I ("--------------------------------");
		}


		bool is_valid_event_schedule_cache_start_time (void) {
			std::regex regex("[0-9]{2}:[0-9]{2}");
			if (!std::regex_match (event_schedule_cache_start_time_.c_str(), regex)) {
				return false;
			}

			std::string str_hour = event_schedule_cache_start_time_.substr (0, 2);
			std::string str_min = event_schedule_cache_start_time_.substr (3);

			int hour = std::atoi (str_hour.c_str());
			int min = std::atoi (str_min.c_str());

			if (hour > 23) {
				return false;
			}
			if (min > 59) {
				return false;
			}

			event_schedule_cache_start_hour_ = hour;
			event_schedule_cache_start_min_ = min;

			return true;
		}

		void resize_tuner_hal_allocates (void) {
			if (tuner_hal_allocates_.size() > arec::group::GROUP_MAX) {
				tuner_hal_allocates_.resize (arec::group::GROUP_MAX);
				_UTL_LOG_W ("resize tuner hal allocates...");
			}
		}

		template <class T>
		void serialize (T & archive) {
			archive (
				cereal::make_nvp("is_syslog_output", is_syslog_output_)
				,cereal::make_nvp("command_server_port", command_server_port_)
				,cereal::make_nvp("tuner_hal_allocates", tuner_hal_allocates_)
				,cereal::make_nvp("channels_json_path", channels_json_path_)
				,cereal::make_nvp("rec_reserves_json_path", rec_reserves_json_path_)
				,cereal::make_nvp("rec_results_json_path", rec_results_json_path_)
				,cereal::make_nvp("rec_ts_path", rec_ts_path_)
				,cereal::make_nvp("rec_disk_space_low_limit_MB", rec_disk_space_low_limit_MB_)
				,cereal::make_nvp("rec_use_splitter", rec_use_splitter_)
				,cereal::make_nvp("dummy_tuner_ts_path", dummy_tuner_ts_path_)
				,cereal::make_nvp("event_schedule_cache_is_enable", event_schedule_cache_is_enable_)
				,cereal::make_nvp("event_schedule_cache_start_interval_day", event_schedule_cache_start_interval_day_)
				,cereal::make_nvp("event_schedule_cache_start_time", event_schedule_cache_start_time_)
				,cereal::make_nvp("event_schedule_cache_timeout_min", event_schedule_cache_timeout_min_)
				,cereal::make_nvp("event_schedule_cache_retry_interval_min", event_schedule_cache_retry_interval_min_)
				,cereal::make_nvp("event_schedule_cache_data_json_path", event_schedule_cache_data_json_path_)
				,cereal::make_nvp("event_schedule_cache_histories_json_path", event_schedule_cache_histories_json_path_)
				,cereal::make_nvp("event_name_keywords_json_path", event_name_keywords_json_path_)
				,cereal::make_nvp("extended_event_keywords_json_path", extended_event_keywords_json_path_)
				,cereal::make_nvp("event_name_search_histories_json_path", event_name_search_histories_json_path_)
				,cereal::make_nvp("extended_event_search_histories_json_path", extended_event_search_histories_json_path_)
				,cereal::make_nvp("viewing_stream_data_path", viewing_stream_data_path_)
				,cereal::make_nvp("viewing_stream_command_format", viewing_stream_command_format_)
				,cereal::make_nvp("viewing_use_splitter", viewing_use_splitter_)
				,cereal::make_nvp("http_server_port", http_server_port_)
				,cereal::make_nvp("http_server_static_contents_path", http_server_static_contents_path_)
				,cereal::make_nvp("logo_path", logo_path_)

			);
		}

	private:
		bool is_syslog_output_;
		uint16_t command_server_port_;
		std::vector<std::string> tuner_hal_allocates_;
		std::string channels_json_path_;
		std::string rec_reserves_json_path_;
		std::string rec_results_json_path_;
		std::string rec_ts_path_;
		int rec_disk_space_low_limit_MB_;
		bool rec_use_splitter_;
		std::string dummy_tuner_ts_path_;
		bool event_schedule_cache_is_enable_;
		int event_schedule_cache_start_interval_day_;
		std::string event_schedule_cache_start_time_;
		int event_schedule_cache_start_hour_;
		int event_schedule_cache_start_min_;
		int event_schedule_cache_timeout_min_;
		int event_schedule_cache_retry_interval_min_;
		std::string event_schedule_cache_data_json_path_;
		std::string event_schedule_cache_histories_json_path_;
		std::string event_name_keywords_json_path_;
		std::string extended_event_keywords_json_path_;
		std::string event_name_search_histories_json_path_;
		std::string extended_event_search_histories_json_path_;
		std::string viewing_stream_data_path_;
		std::string viewing_stream_command_format_;
		bool viewing_use_splitter_;
		uint16_t http_server_port_;
		std::string http_server_static_contents_path_;
		std::string logo_path_;

	};

public:
	virtual ~settings (void) = default;

	static settings *get_instance (void) {
		static settings instance;
		return &instance;
	}

	parameters &get_params (void) {
		return params_;
	}

	void save (void) {
		if (path_.length() == 0) {
			return ;
		}

		std::stringstream ss;
		{
			cereal::JSONOutputArchive out_archive (ss);
			out_archive (cereal::make_nvp("settings", params_));
		}

		std::ofstream ofs (path_.c_str(), std::ios::out);
		ofs << ss.str();

		ofs.close();
		ss.clear();
	}

	bool load (const std::string& path) {
		if (path.length() == 0) {
			return false;
		}

		path_ = path;

		std::stringstream ss;
		std::ifstream ifs (path_.c_str(), std::ios::in);
		ss << ifs.rdbuf();

		cereal::JSONInputArchive in_archive (ss);
		in_archive (cereal::make_nvp("settings", params_));

		ifs.close();
		ss.clear();


		if (!params_.is_valid_event_schedule_cache_start_time()) {
			return false;
		}

		params_.resize_tuner_hal_allocates();

		return true;
	}


private:
	settings (void) = default;

	std::string path_;
	parameters params_;

};

} // namespace arec

#endif
