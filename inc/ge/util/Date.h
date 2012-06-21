// Date.h

#ifndef DATE_H
#define DATE_H

#include <ge/common.h>
#include <ge/text/String.h>

#include <time.h>

/*
 * This class is a wrapper around the standard library time_t type. As such
 * there are a few things to keep in mind:
 *
 * @ Months are zero based [0-11]
 * @ Day of week is zero based [0-6] starting with Sunday
 * @ Hours are zero based [0-23]
 * @ Day of month is NOT zero based [1-31]
 * @ System daylight savings time is used
 * @ time_t size and type varies by os and compiler
 *
 * Be careful of making too many assumptions about the time_t implementation
 * as it degrades portability. The size, sign and start date vary.
 *
 * TODO: Drop native type and just do the math locally
 */
class EXPORT Date
{
public:
	enum dateMonth
	{
		JANUARY,
		FEBRUARY,
		MARCH,
		APRIL,
		MAY,
		JUNE,
		JULY,
		AUGUST,
		SEPTEMBER,
		OCTOBER,
		NOVEMBER,
		DECEMBER
	};

	enum dateDay
	{
		SUNDAY,
		MONDAY,
		TUESDAY,
		WENDSDAY,
		THURSDAY,
		FRIDAY,
		SATURDAY
	};

	Date();
	Date(const Date& date);
	Date(const time_t unixTime);
	Date(uint32 year, uint32 month, uint32 day);
	Date(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 min, uint32 sec);
	~Date();

	time_t getTime_t();

	uint32 getYear();
	uint32 getMonth();
	uint32 getDayOfMonth();
	uint32 getHour();
	uint32 getMinute();
	uint32 getSecond();

	uint32 getDayOfWeek();
	uint32 getDayOfYear();

	// These functions decrement to the first second in the given time period.
	void roundToYear();
	void roundToMonth();
	void roundToWeek();
	void roundToDay();
	void roundToHour();
	void roundToMinute();

	// These functions increment or decrement the time unit. They may round down
	// a lower unit. For example, changing the month may decrement the day of month
	// because the new month has insufficient days.
	void addYears(int32 years);
	void addMonths(int32 months);
	void addWeeks(int32 weeks);
	void addDays(int32 days);
	void addHours(int32 hours);
	void addMinutes(int32 minutes);
	void addSeconds(int32 seconds);

	// Operators
	Date& operator=(const Date& other);
	bool operator==(const Date& other) const;
	bool operator!=(const Date& other) const;
	bool operator<(const Date& other) const;
	bool operator<=(const Date& other) const;
	bool operator>(const Date& other) const;
	bool operator>=(const Date& other) const;
	Date& operator=(const time_t other);
	bool operator==(const time_t other) const;
	bool operator!=(const time_t other) const;
	bool operator<(const time_t other) const;
	bool operator<=(const time_t other) const;
	bool operator>(const time_t other) const;
	bool operator>=(const time_t other) const;

private:
	void init(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 min, uint32 sec);
	int32 getDaysInMonth(int32 month, int32 year);

	time_t m_date;
};

#endif // DATE_H
