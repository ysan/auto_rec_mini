#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <time.h>
#include <sys/time.h>


//#define ENABLE_AFTER_DECIAML_POINT

class CTime
{
public:
	// generate at current time
	CTime (void);
	// generate at specified epoch time
	explicit CTime (time_t epoch);
	explicit CTime (struct timespec epoch);
	// generate at specified each time
//	CTime (
//		int year,
//		int mon,
//		int day,
//		int hour,
//		int min,
//		int sec
//	);

	virtual ~CTime (void);

	bool operator == (const CTime &obj) const;
	bool operator != (const CTime &obj) const;
	bool operator > (const CTime &obj) const;
	bool operator < (const CTime &obj) const;
	bool operator >= (const CTime &obj) const;
	bool operator <= (const CTime &obj) const;
//	bool operator - (const CTime &obj) const;
//	bool operator + (const CTime &obj) const;

	void setNowTime (void);

	void addSec (int sec);
	void addMin (int min);
	void addHour (int hour);
	void addDay (int day);
	void addWeek (int week);

private:

	struct timespec m_time;
};

#endif
