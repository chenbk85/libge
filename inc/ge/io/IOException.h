// IOException.h

#ifndef IO_EXCEPTION_H
#define IO_EXCEPTION_H

#include <ge/Error.h>
#include <ge/text/String.h>
class String;

#include <stdexcept>
 

class IOException : public std::exception
{
public:
	IOException(const char* msg) :
		_str(msg)
	{}

	IOException(const String& msg) :
		_str(msg)
	{}

    IOException(const Error& error) :
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

#endif // IO_EXCEPTION_H
