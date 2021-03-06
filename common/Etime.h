#ifndef _ETIME_H_
#define _ETIME_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <time.h>
#include <sys/time.h>


//#define ENABLE_AFTER_DECIAML_POINT

class CEtime
{
public:
	class CABS {
	public:
//		explicit CABS (const struct timespec &obj) {
		CABS (const struct timespec &obj) {
			m_val.tv_sec = obj.tv_sec;
			m_val.tv_nsec = obj.tv_nsec;
			updateStrings();
		}

		virtual ~CABS (void) {
			m_val.tv_sec = 0;
			m_val.tv_nsec = 0;
			memset (m_string, 0x00, sizeof(m_string));
		}

		CABS & operator = (const struct timespec &obj) {
			m_val.tv_sec = obj.tv_sec;
			m_val.tv_nsec = obj.tv_nsec;
			updateStrings();
			return *this;
		}

		const char * toString (void) const {
			return m_string;
		}

	private:
		void updateStrings (void) {
			int h = m_val.tv_sec / 3600;
			int m = (m_val.tv_sec % 3600) / 60;
			int s =  (m_val.tv_sec % 3600) % 60;

#ifdef ENABLE_AFTER_DECIAML_POINT
			sprintf (m_string, "%02d:%02d:%02d.%03ld", h, m, s, m_val.tv_nsec/1000000);
#else
			sprintf (m_string, "%02d:%02d:%02d", h, m, s);
#endif
		}

		struct timespec m_val;
		char m_string [12+1]; // 00:00:00.000
	};

public:
	// generate at current time
	CEtime (void);
	// generate at specified epoch time
	explicit CEtime (time_t epoch);
	explicit CEtime (struct timespec epoch);
	// generate at specified each time
//	CEtime (
//		int year,
//		int mon,
//		int day,
//		int hour,
//		int min,
//		int sec
//	);

	virtual ~CEtime (void);

	bool operator == (const CEtime &obj) const;
	bool operator != (const CEtime &obj) const;
	bool operator > (const CEtime &obj) const;
	bool operator < (const CEtime &obj) const;
	bool operator >= (const CEtime &obj) const;
	bool operator <= (const CEtime &obj) const;
	struct timespec operator - (const CEtime &obj) const;
//	CEtime operator + (const CEtime &obj) const;

	void setCurrentTime (void);
	void setCurrentDay (void);


	void addSec (int sec);
	void addMin (int min);
	void addHour (int hour);
	void addDay (int day);
	void addWeek (int week);

	const char * toString (void) const;
	const char * toString2 (void) const;
	void updateStrings (void);

	void clear (void);


	// cereal 非侵入型対応のため やむなくpublicに 
	struct timespec m_time;


private:
	void getString (char *pszOut, size_t nSize);
	void getString2 (char *pszOut, size_t nSize);

	char m_time_str [32];
	char m_time_str2 [32];
};

#endif
