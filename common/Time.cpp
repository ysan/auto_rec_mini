#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Time.h"


CTime::CTime (void)
{
	memset (&m_time, 0x00, sizeof(m_time));
	setNowTime ();
}

CTime::CTime (time_t epoch)
{
	memset (&m_time, 0x00, sizeof(m_time));
	m_time.tv_sec = epoch;
}

CTime::CTime (struct timespec epoch)
{
	memset (&m_time, 0x00, sizeof(m_time));
	m_time = epoch;
#ifndef ENABLE_AFTER_DECIAML_POINT
	m_time.tv_nsec = 0;
#endif
}

CTime::~CTime (void)
{
	memset (&m_time, 0x00, sizeof(m_time));
}


bool CTime::operator == (const CTime &obj) const
{
	if ((this->m_time.tv_sec == obj.m_time.tv_sec) &&
		(this->m_time.tv_nsec == obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool CTime::operator != (const CTime &obj) const
{
	if ((this->m_time.tv_sec != obj.m_time.tv_sec) ||
		(this->m_time.tv_nsec != obj.m_time.tv_nsec)) {
		return true;
	} else {
		return false;
	}
}

bool CTime::operator > (const CTime &obj) const
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

bool CTime::operator < (const CTime &obj) const
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

bool CTime::operator >= (const CTime &obj) const
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

bool CTime::operator <= (const CTime &obj) const
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

void CTime::setNowTime (void)
{
	memset (&m_time, 0x00, sizeof(m_time));
	clock_gettime (CLOCK_REALTIME, &m_time);
#ifndef ENABLE_AFTER_DECIAML_POINT
	m_time.tv_nsec = 0;
#endif
}

void CTime::addSec (int sec)
{
	// allow minus value
	m_time.tv_sec += sec;
}

void CTime::addMin (int min)
{
	// allow minus value
	m_time.tv_sec += min * 60;
}

void CTime::addHour (int hour)
{
	// allow minus value
	m_time.tv_sec += hour * 60 * 60;
}

void CTime::addDay (int day)
{
	// allow minus value
	m_time.tv_sec += day * 24 * 60 * 60;
}

void CTime::addWeek (int week)
{
	// allow minus value
	m_time.tv_sec += week * 7 * 24 * 60 * 60;
}
