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

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"


#define _PATH	"./settings.json"


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


		template <class T>
		void serialize (T & archive) {
			archive (
				CEREAL_NVP(m_dummy_tuner_ts_path)

			);
		}

	private:

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
		std::stringstream ss;
		{
			cereal::JSONOutputArchive out_archive (ss);
			out_archive (CEREAL_NVP(m_params));
		}

		std::ofstream ofs (_PATH, std::ios::out);
		ofs << ss.str();

		ofs.close();
		ss.clear();
	}

	void load (void) {
		std::stringstream ss;
		std::ifstream ifs (_PATH, std::ios::in);
		ss << ifs.rdbuf();

		cereal::JSONInputArchive in_archive (ss);
		in_archive (CEREAL_NVP(m_params));

		ifs.close();
		ss.clear();
	}


private:

	CParams m_params;

};

#endif
