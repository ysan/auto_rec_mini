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

namespace arec {
namespace common {



//#define ENABLE_AFTER_DECIAML_POINT

class etime
{
public:
	class abs {
	public:
//		explicit abs (const struct timespec &obj) {
		abs (const struct timespec &obj) {
			val_.tv_sec = obj.tv_sec;
			val_.tv_nsec = obj.tv_nsec;
			update_strings();
		}

		virtual ~abs (void) {
			val_.tv_sec = 0;
			val_.tv_nsec = 0;
			memset (string_, 0x00, sizeof(string_));
		}

		abs & operator = (const struct timespec &obj) {
			val_.tv_sec = obj.tv_sec;
			val_.tv_nsec = obj.tv_nsec;
			update_strings();
			return *this;
		}

		const char * to_string (void) const {
			return string_;
		}

	private:
		void update_strings (void) {
			int h = val_.tv_sec / 3600;
			int m = (val_.tv_sec % 3600) / 60;
			int s =  (val_.tv_sec % 3600) % 60;

#ifdef ENABLE_AFTER_DECIAML_POINT
			sprintf (string_, "%02d:%02d:%02d.%03ld", h, m, s, val_.tv_nsec/1000000);
#else
			sprintf (string_, "%02d:%02d:%02d", h, m, s);
#endif
		}

		struct timespec val_;
		char string_ [12+1]; // 00:00:00.000
	};

public:
	// generate at current time
	etime (void);
	// generate at specified epoch time
	explicit etime (time_t epoch);
	explicit etime (struct timespec epoch);
	// generate at specified each time
//	etime (
//		int year,
//		int mon,
//		int day,
//		int hour,
//		int min,
//		int sec
//	);

	virtual ~etime (void);

	bool operator == (const etime &obj) const;
	bool operator != (const etime &obj) const;
	bool operator > (const etime &obj) const;
	bool operator < (const etime &obj) const;
	bool operator >= (const etime &obj) const;
	bool operator <= (const etime &obj) const;
	struct timespec operator - (const etime &obj) const;
//	etime operator + (const etime &obj) const;

	void set_current_time (void);
	void set_current_day (void);


	void add_sec (int sec);
	void add_min (int min);
	void add_hour (int hour);
	void add_day (int day);
	void add_week (int week);

	const char * to_string (void) const;
	const char * to_string2 (void) const;
	void update_strings (void);

	void clear (void);


	// cereal 非侵入型対応のため やむなくpublicに 
	struct timespec time_;


private:
	void get_string (char *pszOut, size_t nSize);
	void get_string2 (char *pszOut, size_t nSize);

	char time_str_ [32];
	char time_str2_ [32];
};


} // namespace common
} // namespace arec

#endif
