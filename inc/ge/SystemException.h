// SystemException.h

#ifndef SYSTEM_EXCEPTION_H
#define SYSTEM_EXCEPTION_H

#include <ge/Error.h>
#include <ge/text/String.h>
class String;

#include <stdexcept>
 
class SystemException : public std::exception
{
public:
	SystemException(const char* msg) :
		_str(msg)
	{}

	SystemException(const String& msg) :
		_str(msg)
	{}

	SystemException(const Error& error) :
        _error(error)
    {}

    const char* what()
    {
        if (_str.length() == 0)
            _str = _error.toString();

        return _str.c_str();
    }

    const Error getError()
    {
        return _error;
    }

private:
	String _str;
	Error _error;
};

#endif // SYSTEM_EXCEPTION_H
