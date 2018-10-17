// stdafx.cpp : 只包括标准包含文件的源文件
// core.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"

// TODO:  在 STDAFX.H 中
// 引用任何所需的附加头文件，而不是在此文件中引用

#define SECS_TO_FT_MULT 10000000  
static LARGE_INTEGER base_time;

#include <time.h>
#ifdef WIN32
#   include <windows.h>
#else
#   include <sys/time.h>
#endif

#ifdef WIN32

typedef unsigned long long uint64;  // NOLINT
typedef long long int64;  // NOLINT

static const int64 kNumMillisecsPerSec = INT64_C(1000);
static const int64 kNumMicrosecsPerSec = INT64_C(1000000);
static const int64 kNumNanosecsPerSec = INT64_C(1000000000);

static const int64 kNumMicrosecsPerMillisec = kNumMicrosecsPerSec /
kNumMillisecsPerSec;
static const int64 kNumNanosecsPerMillisec = kNumNanosecsPerSec /
kNumMillisecsPerSec;
static const int64 kNumNanosecsPerMicrosec = kNumNanosecsPerSec /
kNumMicrosecsPerSec;

// January 1970, in NTP milliseconds.
static const int64 kJan1970AsNtpMillisecs = INT64_C(2208988800000);

static const uint64 kFileTimeToUnixTimeEpochOffset = 116444736000000000ULL;


int gettimeofday(struct timeval *tv, void *tzp)
{
// 	time_t clock;
// 	struct tm tm;
// 	SYSTEMTIME wtm;
// 	GetLocalTime(&wtm);
// 	tm.tm_year = wtm.wYear - 1900;
// 	tm.tm_mon = wtm.wMonth - 1;
// 	tm.tm_mday = wtm.wDay;
// 	tm.tm_hour = wtm.wHour;
// 	tm.tm_min = wtm.wMinute;
// 	tm.tm_sec = wtm.wSecond;
// 	tm.tm_isdst = -1;
// 	clock = mktime(&tm);
// 	tp->tv_sec = clock;
// 	tp->tv_usec = wtm.wMilliseconds * 1000;
// 	return (0);

	// FILETIME is measured in tens of microseconds since 1601-01-01 UTC.
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	// Convert to seconds and microseconds since Unix time Epoch.
	int64 micros = (li.QuadPart - kFileTimeToUnixTimeEpochOffset) / 10;
	tv->tv_sec = static_cast<long>(micros / kNumMicrosecsPerSec);  // NOLINT
	tv->tv_usec = static_cast<long>(micros % kNumMicrosecsPerSec); // NOLINT

	return 0;
}

#endif