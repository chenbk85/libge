// INetException.h

#ifndef INET_EXCEPTION_H
#define INET_EXCEPTION_H

#include <ge/text/String.h>
class String;

#include <stdexcept>


class INetException : public std::exception
{
public:
	INetException(const char* msg) :
		m_str(msg)
	{}

	INetException(const String& msg) :
		m_str(msg)
	{}

	const char* what()
	{
		return m_str.c_str();
	}

private:
	String m_str;
};

#endif // INET_EXCEPTION_H
