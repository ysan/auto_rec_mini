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

#include "Utils.h"
#include "Group.h"

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


class CSettings
{
public:
	class CParameters {
	public:
		friend class cereal::access;
		CParameters (void) = default;
		virtual ~CParameters (void) = default;


		bool is_syslog_output (void) {
			return m_is_syslog_output;
		}
		uint16_t get_command_server_port (void) {
			return m_command_server_port;
		}
		const std::vector<std::string> &get_tuner_hal_allocates (void) {
			return m_tuner_hal_allocates;
		}
		const std::string &get_channels_json_path (void) {
			return m_channels_json_path;
		}
		const std::string &get_rec_reserves_json_path (void) {
			return m_rec_reserves_json_path;
		}
		const std::string &get_rec_results_json_path (void) {
			return m_rec_results_json_path;
		}
		const std::string &get_rec_ts_path (void) {
			return m_rec_ts_path;
		}
		int get_rec_disk_space_low_limit_MB (void) {
			return m_rec_disk_space_low_limit_MB;
		}
		bool is_rec_use_splitter (void) {
			return m_rec_use_splitter;
		}
		const std::string &get_dummy_tuner_ts_path (void) {
			return m_dummy_tuner_ts_path;
		}
		int is_enable_event_schedule_cache (void) {
			return m_event_schedule_cache_is_enable;
		}
		int get_event_schedule_cache_start_interval_day (void) {
			return m_event_schedule_cache_start_interval_day;
		}
		int get_event_schedule_cache_start_hour (void) {
			return m_event_schedule_cache_start_hour;
		}
		int get_event_schedule_cache_start_min (void) {
			return m_event_schedule_cache_start_min;
		}
		int get_event_schedule_cache_timeout_min (void) {
			return m_event_schedule_cache_timeout_min;
		}
		int get_event_schedule_cache_retry_interval_min (void) {
			return m_event_schedule_cache_retry_interval_min;
		}
		const std::string &get_event_schedule_cache_data_json_path (void) {
			return m_event_schedule_cache_data_json_path;
		}
		const std::string &get_event_schedule_cache_histories_json_path (void) {
			return m_event_schedule_cache_histories_json_path;
		}
		const std::string &get_event_name_keywords_json_path (void) {
			return m_event_name_keywords_json_path;
		}
		const std::string &get_extended_event_keywords_json_path (void) {
			return m_extended_event_keywords_json_path;
		}
		const std::string &get_event_name_search_histories_json_path (void) {
			return m_event_name_search_histories_json_path;
		}
		const std::string &get_extended_event_search_histories_json_path (void) {
			return m_extended_event_search_histories_json_path;
		}
		const std::string &get_viewing_stream_data_path (void) {
			return m_viewing_stream_data_path;
		}
		const std::string &get_viewing_stream_command_format (void) {
			return m_viewing_stream_command_format;
		}
		bool is_viewing_use_splitter (void) {
			return m_viewing_use_splitter;
		}
		uint16_t get_http_server_port (void) {
			return m_http_server_port;
		}
		const std::string &get_http_server_static_contents_path (void) {
			return m_http_server_static_contents_path;
		}
		const std::string &get_logo_path (void) {
			return m_logo_path;
		}

		void dump (void) {
			_UTL_LOG_I ("--------------------------------");
			_UTL_LOG_I ("--          settings          --");
			_UTL_LOG_I ("--------------------------------");
			_UTL_LOG_I ("is_syslog_output:[%d]", m_is_syslog_output);
			_UTL_LOG_I ("command_server_port:[%d]", m_command_server_port);
			_UTL_LOG_I ("tuner_hal_allocates:");
			for (const std::string &s : m_tuner_hal_allocates) {
				_UTL_LOG_I ("  [%s]", s.c_str());
			}
			_UTL_LOG_I ("channels_json_path:[%s]", m_channels_json_path.c_str());
			_UTL_LOG_I ("rec_reserves_json_path:[%s]", m_rec_reserves_json_path.c_str());
			_UTL_LOG_I ("rec_results_json_path:[%s]", m_rec_results_json_path.c_str());
			_UTL_LOG_I ("rec_ts_path:[%s]", m_rec_ts_path.c_str());
			_UTL_LOG_I ("rec_disk_space_low_limit_MB:[%d]", m_rec_disk_space_low_limit_MB);
			_UTL_LOG_I ("rec_use_splitter:[%d]", m_rec_use_splitter);
			_UTL_LOG_I ("dummy_tuner_ts_path:[%s]", m_dummy_tuner_ts_path.c_str());
			_UTL_LOG_I ("event_schedule_cache_is_enable:[%d]", m_event_schedule_cache_is_enable);
			_UTL_LOG_I ("event_schedule_cache_start_interval_day:[%d]", m_event_schedule_cache_start_interval_day);
			_UTL_LOG_I ("event_schedule_cache_start_time:[%s]", m_event_schedule_cache_start_time.c_str());
			_UTL_LOG_I ("event_schedule_cache_timeout_min:[%d]", m_event_schedule_cache_timeout_min);
			_UTL_LOG_I ("event_schedule_cache_retry_interval_min:[%d]", m_event_schedule_cache_retry_interval_min);
			_UTL_LOG_I ("event_schedule_cache_data_json_path:[%s]", m_event_schedule_cache_data_json_path.c_str());
			_UTL_LOG_I ("event_schedule_cache_histories_json_path:[%s]", m_event_schedule_cache_histories_json_path.c_str());
			_UTL_LOG_I ("event_name_keywords_json_path:[%s]", m_event_name_keywords_json_path.c_str());
			_UTL_LOG_I ("extended_event_keywords_json_path:[%s]", m_extended_event_keywords_json_path.c_str());
			_UTL_LOG_I ("event_name_search_histories_json_path:[%s]", m_event_name_search_histories_json_path.c_str());
			_UTL_LOG_I ("extended_event_search_histories_json_path:[%s]", m_extended_event_search_histories_json_path.c_str());
			_UTL_LOG_I ("viewing_stream_data_path:[%s]", m_viewing_stream_data_path.c_str());
			_UTL_LOG_I ("viewing_stream_command_format:[%s]", m_viewing_stream_command_format.c_str());
			_UTL_LOG_I ("viewing_use_splitter:[%d]", m_viewing_use_splitter);
			_UTL_LOG_I ("http_server_port:[%d]", m_http_server_port);
			_UTL_LOG_I ("http_server_static_contents_path:[%s]", m_http_server_static_contents_path.c_str());
			_UTL_LOG_I ("logo_path:[%s]", m_logo_path.c_str());
			_UTL_LOG_I ("--------------------------------");
		}


		bool is_valid_event_schedule_cache_start_time (void) {
			std::regex regex("[0-9]{2}:[0-9]{2}");
			if (!std::regex_match (m_event_schedule_cache_start_time.c_str(), regex)) {
				return false;
			}

			std::string str_hour = m_event_schedule_cache_start_time.substr (0, 2);
			std::string str_min = m_event_schedule_cache_start_time.substr (3);

			int hour = std::atoi (str_hour.c_str());
			int min = std::atoi (str_min.c_str());

			if (hour > 23) {
				return false;
			}
			if (min > 59) {
				return false;
			}

			m_event_schedule_cache_start_hour = hour;
			m_event_schedule_cache_start_min = min;

			return true;
		}

		void resize_tuner_hal_allocates (void) {
			if (m_tuner_hal_allocates.size() > CGroup::GROUP_MAX) {
				m_tuner_hal_allocates.resize (CGroup::GROUP_MAX);
				_UTL_LOG_W ("resize tuner hal allocates...");
			}
		}

		template <class T>
		void serialize (T & archive) {
			archive (
				cereal::make_nvp("is_syslog_output", m_is_syslog_output)
				,cereal::make_nvp("command_server_port", m_command_server_port)
				,cereal::make_nvp("tuner_hal_allocates", m_tuner_hal_allocates)
				,cereal::make_nvp("channels_json_path", m_channels_json_path)
				,cereal::make_nvp("rec_reserves_json_path", m_rec_reserves_json_path)
				,cereal::make_nvp("rec_results_json_path", m_rec_results_json_path)
				,cereal::make_nvp("rec_ts_path", m_rec_ts_path)
				,cereal::make_nvp("rec_disk_space_low_limit_MB", m_rec_disk_space_low_limit_MB)
				,cereal::make_nvp("rec_use_splitter", m_rec_use_splitter)
				,cereal::make_nvp("dummy_tuner_ts_path", m_dummy_tuner_ts_path)
				,cereal::make_nvp("event_schedule_cache_is_enable", m_event_schedule_cache_is_enable)
				,cereal::make_nvp("event_schedule_cache_start_interval_day", m_event_schedule_cache_start_interval_day)
				,cereal::make_nvp("event_schedule_cache_start_time", m_event_schedule_cache_start_time)
				,cereal::make_nvp("event_schedule_cache_timeout_min", m_event_schedule_cache_timeout_min)
				,cereal::make_nvp("event_schedule_cache_retry_interval_min", m_event_schedule_cache_retry_interval_min)
				,cereal::make_nvp("event_schedule_cache_data_json_path", m_event_schedule_cache_data_json_path)
				,cereal::make_nvp("event_schedule_cache_histories_json_path", m_event_schedule_cache_histories_json_path)
				,cereal::make_nvp("event_name_keywords_json_path", m_event_name_keywords_json_path)
				,cereal::make_nvp("extended_event_keywords_json_path", m_extended_event_keywords_json_path)
				,cereal::make_nvp("event_name_search_histories_json_path", m_event_name_search_histories_json_path)
				,cereal::make_nvp("extended_event_search_histories_json_path", m_extended_event_search_histories_json_path)
				,cereal::make_nvp("viewing_stream_data_path", m_viewing_stream_data_path)
				,cereal::make_nvp("viewing_stream_command_format", m_viewing_stream_command_format)
				,cereal::make_nvp("viewing_use_splitter", m_viewing_use_splitter)
				,cereal::make_nvp("http_server_port", m_http_server_port)
				,cereal::make_nvp("http_server_static_contents_path", m_http_server_static_contents_path)
				,cereal::make_nvp("logo_path", m_logo_path)

			);
		}

	private:
		bool m_is_syslog_output;
		uint16_t m_command_server_port;
		std::vector<std::string> m_tuner_hal_allocates;
		std::string m_channels_json_path;
		std::string m_rec_reserves_json_path;
		std::string m_rec_results_json_path;
		std::string m_rec_ts_path;
		int m_rec_disk_space_low_limit_MB;
		bool m_rec_use_splitter;
		std::string m_dummy_tuner_ts_path;
		bool m_event_schedule_cache_is_enable;
		int m_event_schedule_cache_start_interval_day;
		std::string m_event_schedule_cache_start_time;
		int m_event_schedule_cache_start_hour;
		int m_event_schedule_cache_start_min;
		int m_event_schedule_cache_timeout_min;
		int m_event_schedule_cache_retry_interval_min;
		std::string m_event_schedule_cache_data_json_path;
		std::string m_event_schedule_cache_histories_json_path;
		std::string m_event_name_keywords_json_path;
		std::string m_extended_event_keywords_json_path;
		std::string m_event_name_search_histories_json_path;
		std::string m_extended_event_search_histories_json_path;
		std::string m_viewing_stream_data_path;
		std::string m_viewing_stream_command_format;
		bool m_viewing_use_splitter;
		uint16_t m_http_server_port;
		std::string m_http_server_static_contents_path;
		std::string m_logo_path;

	};

public:
	virtual ~CSettings (void) = default;

	static CSettings *get_instance (void) {
		static CSettings instance;
		return &instance;
	}

	CParameters &get_params (void) {
		return m_params;
	}

	void save (void) {
		if (m_path.length() == 0) {
			return ;
		}

		std::stringstream ss;
		{
			cereal::JSONOutputArchive out_archive (ss);
			out_archive (cereal::make_nvp("settings", m_params));
		}

		std::ofstream ofs (m_path.c_str(), std::ios::out);
		ofs << ss.str();

		ofs.close();
		ss.clear();
	}

	bool load (const std::string& path) {
		if (path.length() == 0) {
			return false;
		}

		m_path = path;

		std::stringstream ss;
		std::ifstream ifs (m_path.c_str(), std::ios::in);
		ss << ifs.rdbuf();

		cereal::JSONInputArchive in_archive (ss);
		in_archive (cereal::make_nvp("settings", m_params));

		ifs.close();
		ss.clear();


		if (!m_params.is_valid_event_schedule_cache_start_time()) {
			return false;
		}

		m_params.resize_tuner_hal_allocates();

		return true;
	}


private:
	CSettings (void) = default;

	std::string m_path;
	CParameters m_params;

};

#endif
