#include "timestamp.h"
#include "common.h"
#include <string.h>
#include <sys/time.h>
#include <chrono>


Timestamp::Timestamp(time_t time): microSecondsSinceEpoch(time) {}

Timestamp::Timestamp() : microSecondsSinceEpoch(0) {}

bool Timestamp::isValid()
{
	return microSecondsSinceEpoch > 0;
}

Timestamp::Timestamp(const string& timeStr)
{
	struct tm _tm;
	memset(&_tm, 0, sizeof(_tm));
	ParseResult r = parseTimeStr(timeStr, &_tm);
	if (r != success)
	{
		microSecondsSinceEpoch = 0;
		return;
	}
	microSecondsSinceEpoch = mktime(&_tm) * microSecondsPerSecond;
}

Timestamp::Timestamp(const MYSQL_TIME* mysqlTime)
{
	struct tm _tm;
	memset(&_tm, 0, sizeof(_tm));
	_tm.tm_year = mysqlTime->year - 1900;
	_tm.tm_mon = mysqlTime->month - 1;
	_tm.tm_mday = mysqlTime->day;
	_tm.tm_hour = mysqlTime->hour;
	_tm.tm_min = mysqlTime->minute;
	_tm.tm_sec = mysqlTime->second;
	microSecondsSinceEpoch = mktime(&_tm) * microSecondsPerSecond + mysqlTime->second_part;
}

void Timestamp::addTime(int value, Timestamp::TimeUnit unit)
{
	struct tm _tm;
	memset(&_tm, 0, sizeof(_tm));
	time_t t = microSecondsSinceEpoch / microSecondsPerSecond;
	localtime_r(&t, &_tm);
	switch (unit)
	{
	case year:
		_tm.tm_year = _isvalid_tm_field(_tm.tm_year + value, year) ? _tm.tm_year + value : _tm.tm_year;
		break;
	case month:
		_tm.tm_mon = _isvalid_tm_field(_tm.tm_mon + value, month) ? _tm.tm_mon + value : _tm.tm_mon;
		break;
	case day:
		_tm.tm_mday = _isvalid_tm_field(_tm.tm_mday + value, day) ? _tm.tm_mday + value : _tm.tm_mday;
		break;
	case hour:
		_tm.tm_hour = _isvalid_tm_field(_tm.tm_hour + value, hour) ? _tm.tm_hour + value : _tm.tm_hour;
		break;
	case minute:
		_tm.tm_min = _isvalid_tm_field(_tm.tm_min + value, minute) ? _tm.tm_min + value : _tm.tm_min;
		break;
	case second:
		_tm.tm_sec = _isvalid_tm_field(_tm.tm_sec + value, second) ? _tm.tm_sec + value : _tm.tm_sec;
		break;
	default: break;
	}
	microSecondsSinceEpoch = mktime(&_tm) * microSecondsPerSecond;
}

void Timestamp::addSeconds(double seconds)
{
	microSecondsSinceEpoch += seconds * microSecondsPerSecond;
}

string Timestamp::toFormattedString()
{
	if (!isValid())
		return string();
	char buf[25];
	memset(buf, 0, sizeof(buf));
	struct tm _tm;
	memset(&_tm, 0, sizeof(_tm));
	time_t t = microSecondsSinceEpoch / microSecondsPerSecond;
	localtime_r(&t, &_tm);
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
		_tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
	return string(buf);
}

MYSQL_TIME Timestamp::toMysqlTime(enum_mysql_timestamp_type type)
{
	MYSQL_TIME mytime;
	memset(&mytime, 0, sizeof(mytime));
	mytime.time_type = type;
	struct tm _tm;
	time_t t = microSecondsSinceEpoch / microSecondsPerSecond;
	time_t micro = microSecondsSinceEpoch % microSecondsPerSecond;
	memset(&_tm, 0, sizeof(_tm));
	localtime_r(&t, &_tm);
	mytime.year = _tm.tm_year + 1900;
	mytime.month = _tm.tm_mon + 1;
	mytime.day = _tm.tm_mday;
	mytime.hour = _tm.tm_hour;
	mytime.minute = _tm.tm_min;
	mytime.second = _tm.tm_sec;
	mytime.second_part = micro;
	return mytime;
}

Timestamp Timestamp::now()
{
	return Timestamp(
		static_cast<time_t>(
			chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count()
		)
	);
}

Timestamp::ParseResult Timestamp::parseTimeStr(const string& str, struct tm* _tm)
{
	string time = Util::strip(str);
	size_t spaceIdx = time.find(' ');
	size_t ymdLen = spaceIdx != string::npos ? spaceIdx : str.length();
	string ymdtPart = time.substr(0, ymdLen);
	vector<string> ymd = Util::split(ymdtPart, "-/");
	if (ymd.size() != 3 || ymd[0].length() != 4)
		return fail;
	_tm->tm_year = stoi(ymd[0]) - 1900;
	_tm->tm_mon = stoi(ymd[1]) - 1;
	if (!_isvalid_tm_field(_tm->tm_mon, month))
		return fail;
	_tm->tm_mday = stoi(ymd[2]);
	if (!_isvalid_tm_field(_tm->tm_mday, day))
		return fail;
	if (spaceIdx != string::npos)
	{
		size_t hourPos = spaceIdx + 1;
		while (isspace(time.at(hourPos)))
			hourPos++;
		vector<string> hms = Util::split(time.substr(hourPos), ":");
		size_t sz = hms.size();
		if (sz == 0)
			return success;
		_tm->tm_hour = stoi(hms[0]);
		if (!_isvalid_tm_field(_tm->tm_hour, hour))
			return fail;
		if (sz > 1)
		{
			_tm->tm_min = stoi(hms[1]);
			if (!_isvalid_tm_field(_tm->tm_min, minute))
				return fail;
		}
		if (sz > 2)
		{
			_tm->tm_sec = stoi(hms[2]);
			if (!_isvalid_tm_field(_tm->tm_sec, second))
				return fail;
		}
	}
	return success;
}

bool _isvalid_tm_field(time_t value, Timestamp::TimeUnit unit)
{
	switch (unit)
	{
	case Timestamp::month:
		return (value >= 0 && value < 12);
	case Timestamp::day:
		return (value > 0 && value <= 31);
	case Timestamp::hour:
		return (value >= 0 && value < 24);
	case Timestamp::minute:
		return (value >= 0 && value <= 59);
	case Timestamp::second:
		return (value >= 0 && value <= 60);
	default: return true;
	}
}