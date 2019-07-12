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
//	CEtime operator - (const CEtime &obj) const;
//	CEtime operator + (const CEtime &obj) const;

	void setCurrentTime (void);

	void addSec (int sec);
	void addMin (int min);
	void addHour (int hour);
	void addDay (int day);
	void addWeek (int week);

	const char * toString (void) const;
	const char * toString2 (void) const;

	void clear (void);

	// cereal 非侵入型対応のため やむなくpublicに 
	struct timespec m_time;

private:
	void updateStrings (void);
	void getString (char *pszOut, size_t nSize);
	void getString2 (char *pszOut, size_t nSize);

	char m_time_str [64];
	char m_time_str2 [64];
};

#endif
