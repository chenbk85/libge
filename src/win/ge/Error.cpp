// Error.cpp

#include "ge/Error.h"

#include "ge/text/String.h"
#include "gepriv/WinUtil.h"

extern const char* commonErrorStrings[];

Error::Error() :
    _commonError(err_success),
    _errorType(err_type_system),
    _systemValue(0),
    _failurePoint(NULL),
    _systemCall(NULL)
{
}

Error::Error(CommonError commonValue,
             const char* failurePoint) :
    _commonError(commonValue),
    _errorType(err_type_system),
    _systemValue(0),
    _failurePoint(failurePoint),
    _systemCall(NULL)
{
}

Error::Error(CommonError commonValue,
             uint32      systemError,
             const char* systemCall,
             const char* failurePoint) :
    _commonError(commonValue),
    _errorType(err_type_system),
    _systemValue(systemError),
    _failurePoint(failurePoint),
    _systemCall(systemCall)
{
}

Error::Error(CommonError commonValue,
             ErrorType   errorType,
             uint32      systemError,
             const char* systemCall,
             const char* failurePoint) :
    _commonError(commonValue),
    _errorType(errorType),
    _systemValue(systemError),
    _failurePoint(failurePoint),
    _systemCall(systemCall)
{
}

Error::Error(const Error& other) :
    _commonError(other._commonError),
    _errorType(other._errorType),
    _systemValue(other._systemValue),
    _failurePoint(other._failurePoint),
    _systemCall(other._systemCall)
{
}

Error Error::operator=(const Error& other)
{
    // Can blindly copy even if same object
    _commonError = other._commonError;
    _errorType = other._errorType;
    _systemValue = other._systemValue;
    _failurePoint = other._failurePoint;
    _systemCall = other._systemCall;

    return *this;
}

bool Error::isSet() const
{
    return _commonError != 0;
}

CommonError Error::getCommonValue() const
{
    return _commonError;
}

int32 Error::getSystemValue() const
{
    return _systemValue;
}

const char* Error::getFailurePoint() const
{
    return _failurePoint;
}

const char* Error::getSystemCall() const
{
    return _systemCall;
}

String Error::toString() const
{
    String res("Error: \"");

    res.append(commonErrorStrings[_commonError]);

    if (_failurePoint != NULL)
    {
        res.append("\" from ");
        res.append(_failurePoint);
    }
    else
    {
        res.append("\"");
    }

    if (_errorType == err_type_system &&
        _systemCall != NULL)
    {
        res.append(" calling ");
        res.append(_systemCall);

        if (_systemValue != 0)
        {
            res.append(", which failed with: (");
            res.appendInt32(_systemValue);
            res.append(") \"");
            res.append(WinUtil::getErrorMessage(_systemValue));
            res.append("\"");
        }
    }

    return res;
}
