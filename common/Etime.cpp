#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Etime.h"


CEtime::CEtime (void)
{
	clear ();
}

CEtime::CEtime (time_t epoch)
{
	clear ();

	m_time.tv_sec = epoch;

	updateStrings ();
}

CEtime::CEtime (struct timespec epoch)
{
	clear ();

	m_time = epoch;
#ifndef ENABLE_AFTER_DECIAML_POINT
	m_time.tv_nsec = 0;
#endif

	updateStrings ();
}

CEtime::~CEtime (void)
{
	clear ();
}


bool CEtime::operator == (const CEtime &obj) const
{
	if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
		(this->m_time.tv_nsec == obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool CEtime::operator != (const CEtime &obj) const
{
	if ((this->m_time.tv_sec != obj.m_time.tv_sec) ||
		(this->m_time.tv_nsec != obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool CEtime::operator > (const CEtime &obj) const
{
	if (this->m_time.tv_sec > obj.m_time.tv_sec) {
		return true;
	} else if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
				(this->m_time.tv_nsec > obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool CEtime::operator < (const CEtime &obj) const
{
	if (this->m_time.tv_sec < obj.m_time.tv_sec) {
		return true;
	} else if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
				(this->m_time.tv_nsec < obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool CEtime::operator >= (const CEtime &obj) const
{
	if (this->m_time.tv_sec > obj.m_time.tv_sec) {
		return true;
	} else if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
				(this->m_time.tv_nsec > obj.m_time.tv_nsec)) {
		return true;
	} else if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
				(this->m_time.tv_nsec == obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool CEtime::operator <= (const CEtime &obj) const
{
	if (this->m_time.tv_sec < obj.m_time.tv_sec) {
		return true;
	} else if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
				(this->m_time.tv_nsec < obj.m_time.tv_nsec)) {
		return true;
	} else if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
				(this->m_time.tv_nsec == obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

void CEtime::setCurrentTime (void)
{
	memset (&m_time, 0x00, sizeof(m_time));

	clock_gettime (CLOCK_REALTIME, &m_time);
#ifndef ENABLE_AFTER_DECIAML_POINT
	m_time.tv_nsec = 0;
#endif

	updateStrings ();
}

void CEtime::addSec (int sec)
{
	// allow minus value
	m_time.tv_sec += sec;

	updateStrings ();
}

void CEtime::addMin (int min)
{
	// allow minus value
	m_time.tv_sec += min * 60;

	updateStrings ();
}

void CEtime::addHour (int hour)
{
	// allow minus value
	m_time.tv_sec += hour * 60 * 60;

	updateStrings ();
}

void CEtime::addDay (int day)
{
	// allow minus value
	m_time.tv_sec += day * 24 * 60 * 60;

	updateStrings ();
}

void CEtime::addWeek (int week)
{
	// allow minus value
	m_time.tv_sec += week * 7 * 24 * 60 * 60;

	updateStrings ();
}

const char* CEtime::toString (void) const
{
	return m_time_str;
}

const char* CEtime::toString2 (void) const
{
	return m_time_str2;
}

void CEtime::updateStrings (void)
{
	memset (&m_time_str, 0x00, sizeof(m_time_str));
	memset (&m_time_str2, 0x00, sizeof(m_time_str2));

	getString (m_time_str, sizeof(m_time_str));
	getString2 (m_time_str2, sizeof(m_time_str2));
}

void CEtime::getString (char *pszOut, size_t nSize)
{
	struct tm *pstTmLocal = NULL;
	struct tm stTmLocalTmp;

	pstTmLocal = localtime_r (&m_time.tv_sec, &stTmLocalTmp);

#ifdef ENABLE_AFTER_DECIAML_POINT
	snprintf (
		pszOut,
		nSize,
		"%d/%02d/%02d %02d:%02d:%02d.%03ld",
		pstTmLocal->tm_year+1900,
		pstTmLocal->tm_mon+1,
		pstTmLocal->tm_mday,
		pstTmLocal->tm_hour,
		pstTmLocal->tm_min,
		pstTmLocal->tm_sec,
		m_time.tv_nsec/1000000
	);
#else
	snprintf (
		pszOut,
		nSize,
		"%d/%02d/%02d %02d:%02d:%02d",
		pstTmLocal->tm_year+1900,
		pstTmLocal->tm_mon+1,
		pstTmLocal->tm_mday,
		pstTmLocal->tm_hour,
		pstTmLocal->tm_min,
		pstTmLocal->tm_sec
	);
#endif
}

void CEtime::getString2 (char *pszOut, size_t nSize)
{
	struct tm *pstTmLocal = NULL;
	struct tm stTmLocalTmp;

	pstTmLocal = localtime_r (&m_time.tv_sec, &stTmLocalTmp);

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
		m_time.tv_nsec/1000000
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

void CEtime::clear (void)
{
	memset (&m_time, 0x00, sizeof(m_time));
	memset (&m_time_str, 0x00, sizeof(m_time_str));
	memset (&m_time_str2, 0x00, sizeof(m_time_str2));
}
