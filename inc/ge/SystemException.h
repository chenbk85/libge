// SystemException.h

#ifndef SYSTEM_EXCEPTION_H
#define SYSTEM_EXCEPTION_H

#include <ge/text/String.h>
class String;

#include <stdexcept>
 
class SystemException : public std::exception
{
public:
	SystemException(const char* msg) :
		m_str(msg)
	{}

	SystemException(const String& msg) :
		m_str(msg)
	{}

	const char* what()
	{
		return m_str.c_str();
	}

private:
	String m_str;
};

#endif // SYSTEM_EXCEPTION_H
