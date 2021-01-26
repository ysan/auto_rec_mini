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

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


class CSettings
{
public:
	class CParams {
	public:
		friend class cereal::access;
		CParams (void) {}
		virtual ~CParams (void) {}


		bool isSyslogOutput (void) {
			return m_is_syslog_output;
		}
		uint16_t getCommandServerPort (void) {
			return m_command_server_port;
		}
		std::vector<std::string> *getTunerHalAllocates (void) {
			return &m_tuner_hal_allocates;
		}
		std::string *getChannelsJsonPath (void) {
			return &m_channels_json_path;
		}
		std::string *getRecReservesJsonPath (void) {
			return &m_rec_reserves_json_path;
		}
		std::string *getRecResultsJsonPath (void) {
			return &m_rec_results_json_path;
		}
		std::string *getRecTsPath (void) {
			return &m_rec_ts_path;
		}
		int getRecDiskSpaceLowLimitMB (void) {
			return m_rec_disk_space_low_limit_MB;
		}
		std::string *getDummyTunerTsPath (void) {
			return &m_dummy_tuner_ts_path;
		}
		int isEnableEventScheduleCache (void) {
			return m_event_schedule_cache_is_enable;
		}
		int getEventScheduleCacheStartIntervalDay (void) {
			return m_event_schedule_cache_start_interval_day;
		}
		int getEventScheduleCacheStartHour (void) {
			return m_event_schedule_cache_start_hour;
		}
		int getEventScheduleCacheStartMin (void) {
			return m_event_schedule_cache_start_min;
		}
		int getEventScheduleCacheTimeoutMin (void) {
			return m_event_schedule_cache_timeout_min;
		}
		int getEventScheduleCacheRetryIntervalMin (void) {
			return m_event_schedule_cache_retry_interval_min;
		}
		std::string *getEventScheduleCacheDataJsonPath (void) {
			return &m_event_schedule_cache_data_json_path;
		}
		std::string *getEventScheduleCacheHistoriesJsonPath (void) {
			return &m_event_schedule_cache_histories_json_path;
		}
		std::string *getEventNameKeywordsJsonPath (void) {
			return &m_event_name_keywords_json_path;
		}
		std::string *getExtendedEventKeywordsJsonPath (void) {
			return &m_extended_event_keywords_json_path;
		}
		std::string *getEventNameSearchHistoriesJsonPath (void) {
			return &m_event_name_search_histories_json_path;
		}
		std::string *getExtendedEventSearchHistoriesJsonPath (void) {
			return &m_extended_event_search_histories_json_path;
		}
		std::string *getLogoPath (void) {
			return &m_logo_path;
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
			_UTL_LOG_I ("logo_path:[%s]", m_logo_path.c_str());
			_UTL_LOG_I ("--------------------------------");
		}


		bool isValidEventScheduleCacheStartTime (void) {
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
		std::string m_logo_path;

	};

public:
	CSettings (void) {}
	virtual ~CSettings (void) {}

	static CSettings *getInstance (void) {
		static CSettings instance;
		return &instance;
	}

	CParams *getParams (void) {
		return &m_params;
	}

	void save (void) {
		if (!m_path.c_str()) {
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
		if (!path.c_str()) {
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


		if (!m_params.isValidEventScheduleCacheStartTime()) {
			return false;
		}

		return true;
	}


private:

	std::string m_path;
	CParams m_params;

};

#endif
