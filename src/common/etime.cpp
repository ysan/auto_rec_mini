#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "etime.h"

namespace arec {
namespace common {



etime::etime (void)
{
	clear ();
}

etime::etime (time_t epoch)
{
	clear ();

	time_.tv_sec = epoch;

	update_strings ();
}

etime::etime (struct timespec epoch)
{
	clear ();

	time_ = epoch;
#ifndef ENABLE_AFTER_DECIAML_POINT
	time_.tv_nsec = 0;
#endif

	update_strings ();
}

etime::~etime (void)
{
	clear ();
}


bool etime::operator == (const etime &obj) const
{
	if ((this->time_.tv_sec == obj.time_.tv_sec) &&
		(this->time_.tv_nsec == obj.time_.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool etime::operator != (const etime &obj) const
{
	if ((this->time_.tv_sec != obj.time_.tv_sec) ||
		(this->time_.tv_nsec != obj.time_.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool etime::operator > (const etime &obj) const
{
	if (this->time_.tv_sec > obj.time_.tv_sec) {
		return true;
	} else if ((this->time_.tv_sec == obj.time_.tv_sec) &&
				(this->time_.tv_nsec > obj.time_.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool etime::operator < (const etime &obj) const
{
	if (this->time_.tv_sec < obj.time_.tv_sec) {
		return true;
	} else if ((this->time_.tv_sec == obj.time_.tv_sec) &&
				(this->time_.tv_nsec < obj.time_.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool etime::operator >= (const etime &obj) const
{
	if (this->time_.tv_sec > obj.time_.tv_sec) {
		return true;
	} else if ((this->time_.tv_sec == obj.time_.tv_sec) &&
				(this->time_.tv_nsec > obj.time_.tv_nsec)) {
		return true;
	} else if ((this->time_.tv_sec == obj.time_.tv_sec) &&
				(this->time_.tv_nsec == obj.time_.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool etime::operator <= (const etime &obj) const
{
	if (this->time_.tv_sec < obj.time_.tv_sec) {
		return true;
	} else if ((this->time_.tv_sec == obj.time_.tv_sec) &&
				(this->time_.tv_nsec < obj.time_.tv_nsec)) {
		return true;
	} else if ((this->time_.tv_sec == obj.time_.tv_sec) &&
				(this->time_.tv_nsec == obj.time_.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

struct timespec etime::operator - (const etime &obj) const
{
	if (*this <= obj) {
		struct timespec r = {0, 0};
		return r;
	}

	time_t _diff_sec = 0;
	long _diff_nsec = 0;

	if (this->time_.tv_nsec >= obj.time_.tv_nsec) {
		_diff_sec = this->time_.tv_sec - obj.time_.tv_sec;
		_diff_nsec = this->time_.tv_nsec - obj.time_.tv_nsec;
	} else {
		_diff_sec = (this->time_.tv_sec - 1) - obj.time_.tv_sec;
		_diff_nsec = (this->time_.tv_nsec + 1000000000) - obj.time_.tv_nsec;
	}

	struct timespec r = {_diff_sec, _diff_nsec};
	return r;
}


void etime::set_current_time (void)
{
	memset (&time_, 0x00, sizeof(time_));

	clock_gettime (CLOCK_REALTIME, &time_);
#ifndef ENABLE_AFTER_DECIAML_POINT
	time_.tv_nsec = 0;
#endif

	update_strings ();
}

// date and time is midnight (0:00:00)
void etime::set_current_day (void)
{
	memset (&time_, 0x00, sizeof(time_));

	clock_gettime (CLOCK_REALTIME, &time_);

	struct tm *pstTmLocal = NULL;
	struct tm stTmLocalTmp;

	pstTmLocal = localtime_r (&time_.tv_sec, &stTmLocalTmp);
	time_t s = (pstTmLocal->tm_hour * 60 * 60) + (pstTmLocal->tm_min * 60) + pstTmLocal->tm_sec;
	time_.tv_sec -= s;
	time_.tv_nsec = 0;

	update_strings ();
}

void etime::add_sec (int sec)
{
	// allow minus value
	time_.tv_sec += sec;

	update_strings ();
}

void etime::add_min (int min)
{
	// allow minus value
	time_.tv_sec += min * 60;

	update_strings ();
}

void etime::add_hour (int hour)
{
	// allow minus value
	time_.tv_sec += hour * 60 * 60;

	update_strings ();
}

void etime::add_day (int day)
{
	// allow minus value
	time_.tv_sec += day * 24 * 60 * 60;

	update_strings ();
}

void etime::add_week (int week)
{
	// allow minus value
	time_.tv_sec += week * 7 * 24 * 60 * 60;

	update_strings ();
}

const char* etime::to_string (void) const
{
	return time_str_;
}

const char* etime::to_string2 (void) const
{
	return time_str2_;
}

void etime::update_strings (void)
{
	memset (&time_str_, 0x00, sizeof(time_str_));
	memset (&time_str2_, 0x00, sizeof(time_str2_));

	get_string (time_str_, sizeof(time_str_));
	get_string2 (time_str2_, sizeof(time_str2_));
}

void etime::get_string (char *pszOut, size_t nSize)
{
	struct tm *pstTmLocal = NULL;
	struct tm stTmLocalTmp;

	pstTmLocal = localtime_r (&time_.tv_sec, &stTmLocalTmp);

#ifdef ENABLE_AFTER_DECIAML_POINT
	snprintf (
		pszOut,
		nSize,
		"%d-%02d-%02d-%02d%02d%02d.%03ld",
		pstTmLocal->tm_year+1900,
		pstTmLocal->tm_mon+1,
		pstTmLocal->tm_mday,
		pstTmLocal->tm_hour,
		pstTmLocal->tm_min,
		pstTmLocal->tm_sec,
		time_.tv_nsec/1000000
	);
#else
	snprintf (
		pszOut,
		nSize,
		"%d-%02d-%02d-%02d%02d%02d",
		pstTmLocal->tm_year+1900,
		pstTmLocal->tm_mon+1,
		pstTmLocal->tm_mday,
		pstTmLocal->tm_hour,
		pstTmLocal->tm_min,
		pstTmLocal->tm_sec
	);
#endif
}

void etime::get_string2 (char *pszOut, size_t nSize)
{
	struct tm *pstTmLocal = NULL;
	struct tm stTmLocalTmp;

	pstTmLocal = localtime_r (&time_.tv_sec, &stTmLocalTmp);

#ifdef ENABLE_AFTER_DECIAML_POINT
	snprintf (
		pszOut,
		nSize,
		"%d%02d%02d%02d%02d%02d%03ld",
		pstTmLocal->tm_year+1900,
		pstTmLocal->tm_mon+1,
		pstTmLocal->tm_mday,
		pstTmLocal->tm_hour,
		pstTmLocal->tm_min,
		pstTmLocal->tm_sec,
		time_.tv_nsec/1000000
	);
#else
	snprintf (
		pszOut,
		nSize,
		"%d%02d%02d%02d%02d%02d",
		pstTmLocal->tm_year+1900,
		pstTmLocal->tm_mon+1,
		pstTmLocal->tm_mday,
		pstTmLocal->tm_hour,
		pstTmLocal->tm_min,
		pstTmLocal->tm_sec
	);
#endif
}

void etime::clear (void)
{
	memset (&time_, 0x00, sizeof(time_));
	memset (&time_str_, 0x00, sizeof(time_str_));
	memset (&time_str2_, 0x00, sizeof(time_str2_));
}


} // namespace common
} // namespace arec