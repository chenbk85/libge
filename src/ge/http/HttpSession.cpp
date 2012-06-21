// HttpSession.cpp

#include "ge/http/HttpSession.h"

HttpSession::HttpSession()
{
    reset();
}

HttpSession::~HttpSession()
{

}

HttpSession::HttpSession(HttpSession&& other)
{
    // TODO
}

HttpSession& HttpSession::operator=(HttpSession&& other)
{
    // TODO
    return *this;
}

HttpMethod_enum HttpSession::getMethod()
{
    return method;
}

const StringRef HttpSession::getUrl()
{
    return url;
}

const List<String>& HttpSession::getHeaderLines()
{
    return headerLines;
}

const char* HttpSession::getBody()
{
    return content;
}

size_t HttpSession::getBodyLength()
{
    return contentLen;
}

void HttpSession::respondRaw(const char* data,
                             size_t      dataLen,
                             bool        freeData)
{
    _httpServer->addWriteData(this, (char*)data, dataLen, freeData, true);
}

void HttpSession::setResponseHeader(const StringRef& headerKey,
                                    const StringRef& headerValue)
{

}

void HttpSession::respond(const StringRef& header,
                          const char*      data,
                          size_t           dataLen,
                          bool             freeData)
{
    char* headerCopy = new char[header.length()];
    memcpy(headerCopy, header.data(), header.length());

    if (httpProt == HTTP_PROT_10)
    {
        _httpServer->addWriteData(this, "HTTP/1.0 ", 8, false, false);
    }
    else
    {
        _httpServer->addWriteData(this, "HTTP/1.1 ", 8, false, false);
    }

    _httpServer->addWriteData(this, headerCopy, header.length(), true, false);

    // TODO: Append headers here

    _httpServer->addWriteData(this, (char*)data, dataLen, freeData, true);
}

void HttpSession::reset()
{
    state = READING_FIRST_LINE;
    httpProt = HTTP_PROT_10;
    //url.clear();
    headerLines.clear();
        
    content = NULL;
    contentLen = 0;
    contentIndex = 0;

    lineBufferIndex = 0;
    lineBufferFilled = 0;

    writeActive = false;

    writeListHead = NULL;
    writeListTail = NULL;
    writesComplete = false;
}
