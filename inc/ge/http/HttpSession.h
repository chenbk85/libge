// HttpServerRequest.h

#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <ge/aio/AioSocket.h>
#include <ge/data/List.h>
#include <ge/http/Http.h>
#include <ge/http/HttpServer.h>
#include <ge/text/StringRef.h>
#include <ge/thread/Mutex.h>

class HttpServer;

/*
 * Logical representation of a request sent to a HttpServer. A handler
 * function can use this to examine and respond to a request.
 */
class HttpSession
{
    friend class HttpServer;

private:
    HttpSession();
public:
    ~HttpSession();

    HttpSession(HttpSession&& other);
    HttpSession& operator=(HttpSession&& other);

    // Access functions

    /* Returns the HTTP method of the request */
    HttpMethod_enum getMethod();

    /* Returns the request URL */
    const StringRef getUrl();

    /* Returns the raw header lines of the request */
    const List<String>& getHeaderLines();

    /* Returns the request body */
    const char* getBody();

    /* Returns the request body length */
    size_t getBodyLength();

    // Response functions

    /*! \brief Should be used by http_handler_func callbacks to respond to HTTP
    *          requests with a full and complete response.
    *
    * \param  data            Buffer containing a complete HTTP response
    * \param  dataLen         Length of data (the full length, not just content)
    * \param  freeData        Set to BNET_TRUE if you want data to be freed
    *                         after it's no longer needed. Generally you want
    *                         this set, except with static responses.
    */
    void respondRaw(const char* data,
                    size_t      dataLen,
                    bool        freeData);

    /*! \brief Should be used by http_handler_func callbacks to set values in 
     *         the header of a response. If this is never called, you will get
     *         default values.
     *
     * The following fields should not be set as they will be set automatically:
     * Date
     * Content-Length
     *
     * \param  headerKey       Name of the header value ("Content-Type")
     * \param  headerValue     Value for the header field
     */
    void setResponseHeader(const StringRef& headerKey,
                           const StringRef& headerValue);

    /*! \brief Should be used by http_handler_func callbacks to respond to HTTP
     *         requests. Currently, a full and complete response is required.
     *         Eventually, we will probably move toward having a separate
     *         response object eventually so that chunked or compressed
     *         responses can be supported transparently.
     *
     * \param  header          Header text ("200 OK", "404 Not Found", etc)
     * \param  content         Buffer containing response content
     * \param  contentLen      Length of content
     * \param  freeContent     Set to BNET_TRUE if you want content to be freed
     *                         after it's no longer needed.
     */
    void respond(const StringRef& header,
                 const char*      data,
                 size_t           dataLen,
                 bool             freeData);

private:
    HttpSession(const HttpSession& other) DELETED;
    HttpSession& operator=(const HttpSession& other) DELETED;

    void reset();

    AioServer* _aioServer;
    HttpServer* _httpServer;
    AioSocket _socket;

    SessionState_enum state;

    HttpProt_enum httpProt;
    HttpMethod_enum method;
    String url;
    List<String> headerLines;

    char*  content;
    uint32 contentLen;
    uint32 contentIndex;

    // Line reading state
    char   lineBuffer[HTTP_MAX_LINE];
    size_t lineBufferIndex;
    size_t lineBufferFilled;

    // Guards all of the variables beneath here as they get touched by
    // multiple threads.
    Mutex lock;

    bool writeActive;

    WriteEntry* writeListHead;
    WriteEntry* writeListTail;
    bool        writesComplete; // If all writes have been submitted
};

#endif // HTTP_SERVER_SESSION_H
