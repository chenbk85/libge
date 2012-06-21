// Date.cpp

#include <ge/util/Date.h>

#include <stdio.h>
#include <string.h>

// Alias the windows function
#ifdef _MSC_VER
#	define localtime_r(y, x) localtime_s((x), (y))
#endif

// The number of days in each month on a non-leap year
const uint32 daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

Date::Date()
{
	// Get the current time with time()
	time(&m_date);
}

Date::Date(const Date& date)
{
	m_date = date.m_date;
}

Date::Date(const time_t utc)
{
	m_date = utc;
}

Date::Date(uint32 year, uint32 month, uint32 day)
{
	init(year, month, day, 0, 0, 0);
}

Date::Date(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 min, uint32 sec)
{
	init(year, month, day, hour, min, sec);
}

Date::~Date()
{

}

time_t Date::getTime_t()
{
	return m_date;
}

uint32 Date::getYear()
{
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	return tm_struct.tm_year + 1900;
}

uint32 Date::getMonth()
{
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	return tm_struct.tm_mon;
}

uint32 Date::getDayOfMonth()
{
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	return tm_struct.tm_mday;
}

uint32 Date::getHour()
{
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	return tm_struct.tm_hour;
}

uint32 Date::getMinute()
{
	return m_date % 3600;
}

uint32 Date::getSecond()
{
	return m_date % 60;
}

uint32 Date::getDayOfWeek()
{
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	return tm_struct.tm_wday;
}

uint32 Date::getDayOfYear()
{
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	return tm_struct.tm_yday + 1;
}

void Date::roundToYear()
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Set to January
	tm_struct.tm_mon = JANUARY;

	// Set to the first day of the month
	tm_struct.tm_mday = 1;

	// Zero the hours, minutes and seconds
	tm_struct.tm_hour = 0;
	tm_struct.tm_min = 0;
	tm_struct.tm_sec = 0;

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::roundToMonth()
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Set to the first day of the month
	tm_struct.tm_mday = 1;

	// Zero the hours, minutes and seconds
	tm_struct.tm_hour = 0;
	tm_struct.tm_min = 0;
	tm_struct.tm_sec = 0;

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::roundToWeek()
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Zero the hours, minutes and seconds
	tm_struct.tm_hour = 0;
	tm_struct.tm_min = 0;
	tm_struct.tm_sec = 0;
	
	// Subtract the day of the week from the day of month
	tm_struct.tm_mday -= tm_struct.tm_wday;

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::roundToDay()
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Zero the hours, minutes and seconds
	tm_struct.tm_hour = 0;
	tm_struct.tm_min = 0;
	tm_struct.tm_sec = 0;

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::roundToHour()
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Zero the minutes and seconds
	tm_struct.tm_min = 0;
	tm_struct.tm_sec = 0;

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::roundToMinute()
{
	// As no timezone has partial minutes we can just round the
	// existing time_t
	m_date -= (m_date % 60);
}

void Date::addYears(int32 years)
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Increment the year
	tm_struct.tm_year += years;

	// If the day of month is now past the last day in the month, use the
	// closest day (only happens with February)
	int32 maxDays = getDaysInMonth(tm_struct.tm_mon, tm_struct.tm_year+1900);
	if (tm_struct.tm_mday > maxDays)
	{
		tm_struct.tm_mday = maxDays;
	}

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::addMonths(int32 months)
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Increment the month and year
	tm_struct.tm_year += months / 12;
	tm_struct.tm_mon += months % 12;

	// Adjust if month increment stepped past december
	if (tm_struct.tm_mon > DECEMBER)
	{
		tm_struct.tm_year++;
		tm_struct.tm_mon %= 12;
	}

	// If the day of month is now past the last day in the month, use the
	// closest day
	int32 maxDays = getDaysInMonth(tm_struct.tm_mon, tm_struct.tm_year+1900);
	if (tm_struct.tm_mday > maxDays)
	{
		tm_struct.tm_mday = maxDays;
	}

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::addWeeks(int32 weeks)
{
	return addDays(weeks * 7);
}

void Date::addDays(int32 days)
{
	// Make a tm structure out of our time_t
	tm tm_struct;
	localtime_r(&m_date, &tm_struct);
	tm_struct.tm_isdst = -1;

	// Increment the day of month
	tm_struct.tm_mday += days;

	// Convert back to a time_t
	m_date = mktime(&tm_struct);
}

void Date::addHours(int32 hours)
{
	m_date += (3600 * hours);
}

void Date::addMinutes(int32 minutes)
{
	m_date += (60 * minutes);
}

void Date::addSeconds(int32 seconds)
{
	m_date += seconds;
}

// Private functions --------------------------------------------------------

void Date::init(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 min, uint32 sec)
{
	// Adjust year, needs to be "current year -1900"
	if (year > 1900)
		year -= 1900;

	// Fill a tm struct
	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));

	// Copy in the parameter values
	tm_struct.tm_year = year;
	tm_struct.tm_mon = month;
	tm_struct.tm_mday = day;
	tm_struct.tm_hour = hour;
	tm_struct.tm_min = min;
	tm_struct.tm_sec = sec;
	tm_struct.tm_isdst = -1;

	// Convert to time_t
	m_date = mktime(&tm_struct);
}

int32 Date::getDaysInMonth(int32 month, int32 year)
{
	// If not February, return the array value
	if (month != FEBRUARY)
		return daysInMonth[month];

	// Otherwise factor in the leap year
	int32 leap = (int32)(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
	return 28 + leap;
}

// Operator functions -------------------------------------------------------

Date& Date::operator=(const Date& other)
{
	m_date = other.m_date;
	return *this;
}

bool Date::operator==(const Date& other) const
{
	return m_date == other.m_date;
}

bool Date::operator!=(const Date& other) const
{
	return m_date != other.m_date;
}

bool Date::operator<(const Date& other) const
{
	return m_date < other.m_date;
}

bool Date::operator<=(const Date& other) const
{
	return m_date <= other.m_date;
}

bool Date::operator>(const Date& other) const
{
	return m_date > other.m_date;
}

bool Date::operator>=(const Date& other) const
{
	return m_date >= other.m_date;
}

Date& Date::operator=(const time_t other)
{
	m_date = other;
	return *this;
}

bool Date::operator==(const time_t other) const
{
	return m_date == other;
}

bool Date::operator!=(const time_t other) const
{
	return m_date != other;
}

bool Date::operator<(const time_t other) const
{
	return m_date < other;
}

bool Date::operator<=(const time_t other) const
{
	return m_date <= other;
}

bool Date::operator>(const time_t other) const
{
	return m_date > other;
}

bool Date::operator>=(const time_t other) const
{
	return m_date >= other;
}
