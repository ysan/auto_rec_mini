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

#include "Utils.h"

#include "cereal/cereal.hpp"
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
		std::string *getDummyTunerTsPath (void) {
			return &m_dummy_tuner_ts_path;
		}
		int getScheduleCacheStartIntervalDay (void) {
			return m_schedule_cache_start_interval_day;
		}
		int getScheduleCacheStartHour (void) {
			return m_schedule_cache_start_hour;
		}
		int getScheduleCacheStartMin (void) {
			return m_schedule_cache_start_min;
		}
		int getScheduleCacheTimeoutMin (void) {
			return m_schedule_cache_timeout_min;
		}



		void dump (void) {
			_UTL_LOG_I ("----------------");
			_UTL_LOG_I ("--  settings  --");
			_UTL_LOG_I ("----------------");
			_UTL_LOG_I ("m_is_syslog_output:[%d]", m_is_syslog_output);
			_UTL_LOG_I ("m_command_server_port:[%d]", m_command_server_port);
			_UTL_LOG_I ("m_channels_json_path:[%s]", m_channels_json_path.c_str());
			_UTL_LOG_I ("m_rec_reserves_json_path:[%s]", m_rec_reserves_json_path.c_str());
			_UTL_LOG_I ("m_rec_results_json_path:[%s]", m_rec_results_json_path.c_str());
			_UTL_LOG_I ("m_rec_ts_path:[%s]", m_rec_ts_path.c_str());
			_UTL_LOG_I ("m_dummy_tuner_ts_path:[%s]", m_dummy_tuner_ts_path.c_str());
			_UTL_LOG_I ("m_schedule_cache_start_interval_day:[%d]", m_schedule_cache_start_interval_day);
			_UTL_LOG_I ("m_schedule_cache_start_hour:[%d]", m_schedule_cache_start_hour);
			_UTL_LOG_I ("m_schedule_cache_start_min:[%d]", m_schedule_cache_start_min);
			_UTL_LOG_I ("m_schedule_cache_timeout_min:[%d]", m_schedule_cache_timeout_min);
			_UTL_LOG_I ("----------------");
		}


		template <class T>
		void serialize (T & archive) {
			archive (
				CEREAL_NVP(m_is_syslog_output),
				CEREAL_NVP(m_command_server_port),
				CEREAL_NVP(m_channels_json_path),
				CEREAL_NVP(m_rec_reserves_json_path),
				CEREAL_NVP(m_rec_results_json_path),
				CEREAL_NVP(m_rec_ts_path),
				CEREAL_NVP(m_dummy_tuner_ts_path),
				CEREAL_NVP(m_schedule_cache_start_interval_day),
				CEREAL_NVP(m_schedule_cache_start_hour),
				CEREAL_NVP(m_schedule_cache_start_min),
				CEREAL_NVP(m_schedule_cache_timeout_min)

			);
		}

	private:

		bool m_is_syslog_output;
		uint16_t m_command_server_port;
		std::string m_channels_json_path;
		std::string m_rec_reserves_json_path;
		std::string m_rec_results_json_path;
		std::string m_rec_ts_path;
		std::string m_dummy_tuner_ts_path;
		int m_schedule_cache_start_interval_day;
		int m_schedule_cache_start_hour;
		int m_schedule_cache_start_min;
		int m_schedule_cache_timeout_min;

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
			_UTL_LOG_E ("settings save failure.");
			return ;
		}

		std::stringstream ss;
		{
			cereal::JSONOutputArchive out_archive (ss);
			out_archive (CEREAL_NVP(m_params));
		}

		std::ofstream ofs (m_path.c_str(), std::ios::out);
		ofs << ss.str();

		ofs.close();
		ss.clear();
	}

	void load (const std::string& path) {
		if (!path.c_str()) {
			_UTL_LOG_E ("settings load failure.");
			return;
		}

		m_path = path;

		std::stringstream ss;
		std::ifstream ifs (m_path.c_str(), std::ios::in);
		ss << ifs.rdbuf();

		cereal::JSONInputArchive in_archive (ss);
		in_archive (CEREAL_NVP(m_params));

		ifs.close();
		ss.clear();
	}


private:

	std::string m_path;
	CParams m_params;

};

#endif
