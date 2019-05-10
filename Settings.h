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


		std::string *getDummyTunerTsPath (void) {
			return &m_dummy_tuner_ts_path;
		}

		void dump (void) {
			_UTL_LOG_I ("--  settings  --");
			_UTL_LOG_I ("m_command_server_port:[%d]", m_command_server_port);
			_UTL_LOG_I ("m_channel_scan_data_path:[%s]", m_channel_scan_data_path.c_str());
			_UTL_LOG_I ("m_rec_reserve_data_path:[%s]", m_rec_reserve_data_path.c_str());
			_UTL_LOG_I ("m_rec_result_data_path:[%s]", m_rec_result_data_path.c_str());
			_UTL_LOG_I ("m_dummy_tuner_ts_path:[%s]", m_dummy_tuner_ts_path.c_str());
			_UTL_LOG_I ("----------------");
		}


		template <class T>
		void serialize (T & archive) {
			archive (
				CEREAL_NVP(m_command_server_port),
				CEREAL_NVP(m_channel_scan_data_path),
				CEREAL_NVP(m_rec_reserve_data_path),
				CEREAL_NVP(m_rec_result_data_path),
				CEREAL_NVP(m_dummy_tuner_ts_path)

			);
		}

	private:

		uint16_t m_command_server_port;
		std::string m_channel_scan_data_path;
		std::string m_rec_reserve_data_path;
		std::string m_rec_result_data_path;
		std::string m_dummy_tuner_ts_path;

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
