// HttpServer.cpp

#include "ge/http/HttpServer.h"

#include "ge/io/Console.h"
#include "ge/thread/Mutex.h"
#include "ge/util/UInt32.h"

#include <cstring>

static const char* badReqMsg =
    "HTTP/1.0 400 Bad Request\r\n"
    "Content-type: text/html\r\n"
    "Content-length: 121\r\n"
    "\r\n"
    "<HTML>\r\n"
    "  <HEAD>\r\n"
    "    <TITLE>Bad Request</TITLE>\r\n"
    "  </HEAD>\r\n"
    "  <BODY>\r\n"
    "    <P>Invalid HTTP request.\r\n"
    "  </BODY>\r\n"
    "</HTML>\r\n"
    "\r\n";

static const char* lengthReqMsg =
    "HTTP/1.0 411 Length Required\r\n"
    "Content-type: text/html\r\n"
    "Content-length: 146\r\n"
    "\r\n"
    "<HTML>\r\n"
    "  <HEAD>\r\n"
    "    <TITLE>Length Required</TITLE>\r\n"
    "  </HEAD>\r\n"
    "  <BODY>\r\n"
    "    <P>HTTP request missing Content-Length field.\r\n"
    "  </BODY>\r\n"
    "</HTML>\r\n"
    "\r\n";

static const char* notImplMsg = 
    "HTTP/1.0 501 Method Not Implemented\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 145\r\n"
    "\r\n"
    "<HTML>\r\n"
    "  <HEAD>\r\n"
    "    <TITLE>Method Not Implemented</TITLE>\r\n"
    "  </HEAD>\r\n"
    "  <BODY>\r\n"
    "    <P>HTTP request method not supported.\r\n"
    "  </BODY>\r\n"
    "</HTML>\r\n"
    "\r\n";

/*! \brief  Given a header line, this checks for a matching key. If the key
 *          does match, it returns a StringRef containing the value portion
 *          of the string. If it doesn't match, returns empty string.
 *
 * Example: headerMatchExtract("Content-Length: 400", "content-length");
 *          Should return a StringRef containing the substring "400"
 *
 * \param headerLine    The full header line from the client
 * \param expectedKey   Name we are expecting to match
 * \param Extracted value or empty string on error
 */
StringRef HttpServer::headerMatchExtract(const StringRef headerLine,
                                         const StringRef expectedKey)
{
    size_t        i;

    size_t headerLen = headerLine.length();
    size_t expectedLen = expectedKey.length();

    for (i = 0;
         i < expectedLen &&
         i < headerLen &&
         headerLine.charAt(i) != ':' &&
         !isspace(headerLine.charAt(i));
         i++)
    {
        char a = tolower(expectedKey.charAt(i));
        char b = tolower(headerLine.charAt(i));

        if (a != b)
            return "";
    }

    while (isspace(headerLine.charAt(i)))
        i++;

    if (i < expectedLen &&
        headerLine.charAt(i) == ':')
    {
        size_t resStart = i + 1;

        while (isspace(headerLine.charAt(resStart)))
            resStart++;

        return StringRef(headerLine.data() + resStart,
                         headerLine.length() - resStart);
    }

    return "";
}

/*! \brief  Parses the first line of the request, extracting the request
 *          type and URL string.
 *
 *  \param  line         First line of the HTTP request
 *  \param  requestType  Extracted request type
 *  \param  urlStr       Will receive an allocated string with the URL
 */
void HttpServer::parseFirstRequestLine(StringRef    line,
                                       HttpSession* session,
                                       bool*        invalid)
{
    size_t lineLen = line.length();

    (*invalid) = false;

    // Extract the request type string
    size_t reqTypeEnd = 0;

    while (!isspace(line.charAt(reqTypeEnd)) &&
           (reqTypeEnd < lineLen - 1))
    {
        reqTypeEnd++;
    }
    
    StringRef requestType = line.substring(0, reqTypeEnd);

    // Extract request type
    if (requestType.engEqualsIgnoreCase("GET"))
    {
        session->method = HTTP_GET;
    }
    else if (requestType.engEqualsIgnoreCase("HEAD"))
    {
        session->method = HTTP_HEAD;
    }
    else if (requestType.engEqualsIgnoreCase("POST"))
    {
        session->method = HTTP_POST;
    }
    else if (requestType.engEqualsIgnoreCase("PUT"))
    {
        session->method = HTTP_PUT;
    }
    else if (requestType.engEqualsIgnoreCase("DELETE"))
    {
        session->method = HTTP_PUT;
    }
    else if (requestType.engEqualsIgnoreCase("TRACE"))
    {
        session->method = HTTP_PUT;
    }
    else
    {
        (*invalid) = true;
    }

    // Find start of URL string
    size_t urlStart = reqTypeEnd;
    while (isspace(line.charAt(urlStart)) &&
           (urlStart < lineLen))
    {
        urlStart++;
    }

    size_t urlEnd = urlStart;

    // Find the length of the URL string
    while (!isspace(line.charAt(urlEnd)) &&
           (urlEnd < lineLen))
    {
        urlEnd++;
    }
    
    // Store the URL
    session->url = line.substring(urlStart, urlEnd);

    // Find the start of the protocol string
    size_t protStart = urlEnd;
    while (isspace(line.charAt(protStart)) &&
           (protStart < lineLen))
    {
        protStart++;
    }

    size_t protEnd = protStart;

    // Find the length of the protocol string
    while (!isspace(line.charAt(protEnd)) &&
           (protEnd < lineLen))
    {
        protEnd++;
    }

    StringRef protStr = line.substring(protStart, protEnd);

    // See which protocol version it is
    if (protStr.engEqualsIgnoreCase("HTTP/1.0"))
    {
        session->httpProt = HTTP_PROT_10;
    }
    else if (protStr.engEqualsIgnoreCase("HTTP/1.1"))
    {
        session->httpProt = HTTP_PROT_11;
    }
    else
    {
        // TODO: Should report unsupported protocol version instead
        (*invalid) = true;
    }
}

/*! \brief Extracts any needed data from the passed HTTP header line.
 *
 * \param  line         The line to parse
 * \param  session      Session to update with parsed data
 * \param  invalid      Set to true if the header line is invalid
 */
void HttpServer::parseHeaders(HttpSession*     session,
                              bool*            invalid)
{
    (*invalid) = false;

    // TODO: Should do general check for a :
    
    size_t headerCount = session->headerLines.size();
    
    for (size_t i = 0; i< headerCount; i++)
    {
        String& line = session->headerLines.get(i);

        StringRef str = headerMatchExtract(line, "Content-Length");

        if (str.length() != 0)
        {
            bool validSize;

            session->contentLen = UInt32::parseUInt32(str, &validSize);

            if (!validSize)
            {
                (*invalid) = true;
                return;
            }
        }
    }
}

/*! \brief Adds a block of data to be written to the session as part of the
 *         response. Will be written after all already added data.
 *
 * TODO: May may more sense to require the response lock to be locked and
 * pass a _writeEntry in.
 *
 * \param  session     Session to have response data added
 * \param  data        Data to add to response
 * \param  dataLen     Length of data
 * \param  freeData    If data should be freed once written
 * \param  lastData    If this is the last part of the response data
 */
void HttpServer::addWriteData(HttpSession* session,
                              char*        data,
                              size_t       dataLen,
                              bool         freeData,
                              bool         lastData)
{
    WriteEntry* newEntry = new WriteEntry();

    newEntry->data = data;
    newEntry->dataLen = dataLen;
    newEntry->freeData = freeData;

    session->lock.lock();

    // Flag as the writes being complete if this is the last write data
    session->writesComplete = lastData;

    // TODO: Only queue if busy writing?
    // Add entry to linked list of data to write
    if (session->writeListTail == NULL)
    {
        session->writeListHead = newEntry;
        session->writeListTail = newEntry;
    }
    else
    {
        session->writeListTail->next = newEntry;
        session->writeListTail = newEntry;
    }

    // If there is no current write active, fire one off
    if (!session->writeActive)
    {
        session->writeActive = true;

        AioServer* aioServer = session->_aioServer;

        aioServer->socketWrite(&session->_socket,
                               writeCallback,
                               session,
                               session->writeListHead->data,
                               session->writeListHead->dataLen);
    }

    session->lock.unlock();
}

void HttpServer::sendRequestFailure(HttpSession* session,
                                    StringRef    message)
{
    addWriteData(session,
                 (char*)message.data(),
                 message.length(),
                 false,
                 true);
}

/*! \brief Attempts to read a line into the session's line buffer.
 *
 *  \param  session   Session to read into
 *  \param  invalid   Indicates if the line is somehow invalid
 */
StringRef HttpServer::tryReadLine(HttpSession* session,
                                  size_t       bytesRead,
                                  bool*        lineCompleted,
                                  bool*        invalid)
{
    (*lineCompleted) = false;
    (*invalid) = false;

    // Scan for illegal bytes and the newline at the same time. It's
    // particularly important to reject null characters to avoid security
    // issues when passing strings to OS functions.
    size_t i;
    for (i = session->lineBufferIndex;
         i < session->lineBufferFilled;
         i++)
    {
        char c = session->lineBuffer[i];

        if (c == '\n')
        {
            size_t lineEnd = i;

            if (i != 0 &&
                session->lineBuffer[i-1] == '\r')
            {
                lineEnd--;
            }

            session->lineBufferIndex = i+1;
            (*lineCompleted) = true;
            return StringRef(session->lineBuffer, lineEnd);
        }
        else if (c < 30 &&
                 c != '\r' &&
                 c != '\t')
        {
            (*invalid) = true;
            return StringRef();
        }
    }

    // If haven't yet found end of line and hit end of buffer, mark as
    // invalid. The line is too long.
    if (i == sizeof(session->lineBuffer))
    {
        (*invalid) = true;
    }

    session->lineBufferIndex = i;
    return StringRef();
}

/*! \brief Flushes any line in session->lineBuffer and moves bytes past the
 *         end of line to the start of the buffer.
 *
 *  \param  session    The session to flush the line from
 */
void HttpServer::flushLine(HttpSession* session)
{
    // Move data past end of line to start of buffer
    ::memmove(session->lineBuffer,
              session->lineBuffer + session->lineBufferIndex,
              session->lineBufferFilled - session->lineBufferIndex);

    // Adjust indicies
    session->lineBufferFilled -= session->lineBufferIndex;
    session->lineBufferIndex = 0;
}

HttpServer::HttpServer()
{
}

HttpServer::~HttpServer()
{

}

HttpServer::HttpServer(HttpServer&& other)
{
    // TODO
}

HttpServer& HttpServer::operator=(HttpServer&& other)
{
    // TODO
    return *this;
}

void HttpServer::startServing(AioServer* aioServer,
                              uint32 port,
                              httpHandler_func handler)
{
    _aioServer = aioServer;
    _handler = handler;

    // Create some session objects (with sockets) for new connections
    _pendingSessionIpv4 = new HttpSession();
    _pendingSessionIpv4->_httpServer = this;
    _pendingSessionIpv4->_aioServer = aioServer;

    _pendingSessionIpv6 = new HttpSession();
    _pendingSessionIpv6->_httpServer = this;
    _pendingSessionIpv6->_aioServer = aioServer;

    // Bind the accept sockets to the designated port
    // This is the most likely thing to fail
    _acceptSockIpv4.init(INET_PROT_IPV4);
    _acceptSockIpv6.init(INET_PROT_IPV6);

    _acceptSockIpv4.bind(INetAddress::getAddrAny(INET_PROT_IPV4), port);
    _acceptSockIpv6.bind(INetAddress::getAddrAny(INET_PROT_IPV6), port);

    _acceptSockIpv4.listen();
    _acceptSockIpv6.listen();

    // Start accepting
    _aioServer->socketAccept(&_acceptSockIpv4,
                             &_pendingSessionIpv4->_socket,
                             acceptCallback,
                             _pendingSessionIpv4);

    _aioServer->socketAccept(&_acceptSockIpv6,
                             &_pendingSessionIpv6->_socket,
                             acceptCallback,
                             _pendingSessionIpv6);
}

void HttpServer::shutdown()
{
    // We do not shutdown the passed AioServer as we don't own it
    // But we do depend on it being shut down first
    // TODO: Add check

    // Close accepting sockets
    if (_pendingSessionIpv4 != NULL)
    {
        _pendingSessionIpv4->_socket.hardClose();
        delete _pendingSessionIpv4;
    }

    if (_pendingSessionIpv6 != NULL)
    {
        _pendingSessionIpv6->_socket.hardClose();
        delete _pendingSessionIpv6;
    }
}

void HttpServer::acceptCallback(AioSocket* aioSocket,
                                AioSocket* acceptedSocket,
                                void* userData,
                                const Error& error)
{
    HttpSession* session = (HttpSession*)userData;


    Console::outln("acceptCallback");

    if (error.isSet())
    {
        Console::outln(String("acceptCallback: ") + error.toString());
        return;
    }

    HttpServer* httpServer = session->_httpServer;
    AioServer* aioServer = httpServer->_aioServer;

    // Start reading

    aioServer->socketRead(acceptedSocket,
                          readCallback,
                          session,
                          session->lineBuffer,
                          sizeof(session->lineBuffer));

    HttpSession* newSession = new HttpSession();
    newSession->_httpServer = httpServer;
    newSession->_aioServer = aioServer;

    // Create new requests objects and accept again
    if (aioSocket == &httpServer->_acceptSockIpv4)
    {
        httpServer->_pendingSessionIpv4 = newSession;

        aioServer->socketAccept(&httpServer->_acceptSockIpv4,
                                &httpServer->_pendingSessionIpv4->_socket,
                                acceptCallback,
                                httpServer->_pendingSessionIpv4);
    }
    else
    {
        httpServer->_pendingSessionIpv6 = newSession;

        aioServer->socketAccept(&httpServer->_acceptSockIpv6,
                                &httpServer->_pendingSessionIpv6->_socket,
                                acceptCallback,
                                httpServer->_pendingSessionIpv6);
    }
}

void HttpServer::readCallback(AioSocket* aioSocket,
                              void* userData,
                              uint32 bytesTransfered,
                              const Error& error)
{
    HttpSession* session = (HttpSession*)userData;

    Console::outln("readCallback");

    if (error.isSet())
    {
        Console::outln(String("readCallback: ") + error.toString());
        session->_aioServer->socketClose(&session->_socket,
                                         closeCallback,
                                         session);
        return;
    }

    // If read 0 bytes, peer closed connection
    if (bytesTransfered == 0)
    {
        // TODO: Need to reference count
        //AioServer* aioServer = session->_aioServer;
        //aioServer->socketClose(aioSocket,
        //                       closeCallback,
        //                       session);
        return;
    }

    // Call readHandler to parse the data
    bool keepSock = readHandler(session,
                                aioSocket,
                                bytesTransfered);

    // Close socket if indicated
    if (!keepSock)
    {
        AioServer* aioServer = session->_aioServer;
        aioServer->socketClose(aioSocket,
                               closeCallback,
                               session);
        return;
    }

    // Trigger new read
    if (session->state == READING_BODY)
    {
        session->_aioServer->socketRead(&session->_socket,
                                        readCallback,
                                        session,
                                        session->content + session->contentIndex,
                                        session->contentLen - session->contentIndex);
    }
    else if (session->state != RESPONDING)
    {
        session->_aioServer->socketRead(&session->_socket,
                                        readCallback,
                                        session,
                                        session->lineBuffer + session->lineBufferFilled,
                                        sizeof(session->lineBuffer) - session->lineBufferFilled);
    }
}

bool HttpServer::readHandler(HttpSession* session,
                             AioSocket* aioSocket,
                             uint32 bytesTransfered)
{
    bool lineCompleted;
    bool invalid;
    
    // If in state of reading first line of request
    if (session->state == READING_FIRST_LINE)
    {
        session->lineBufferFilled += bytesTransfered;

        StringRef line = tryReadLine(session,
                                     bytesTransfered,
                                     &lineCompleted,
                                     &invalid);
        bytesTransfered = 0;

        if (invalid)
        {
            sendRequestFailure(session, badReqMsg);
            return true;
        }

        // Just return if didn't read the full line
        if (!lineCompleted)
        {
            // Issue another read?
            return true;
        }

        Console::outln(String("Request: ") + line);

        // Parse the first line
        parseFirstRequestLine(line, session, &invalid);

        if (invalid)
        {
            sendRequestFailure(session, notImplMsg);
            return true;
        }

        // Flush the line read
        flushLine(session);

        // If we read a HTTP1.1 header, add a continue response
        if (session->httpProt == HTTP_PROT_11)
        {
            addWriteData(session,
                         (char*)"HTTP/1.1 100 Continue\r\n\r\n",
                         25,
                         false,
                         false);
        }

        // Change state to reading header lines
        session->state = READING_HEADERS;
    }
    
    // If in state of reading headers
    if (session->state == READING_HEADERS)
    {
        session->lineBufferFilled += bytesTransfered;

        do
        {
            // Try to read the line
            StringRef line = tryReadLine(session,
                                         bytesTransfered,
                                         &lineCompleted,
                                         &invalid);
            bytesTransfered = 0;

            if (invalid)
            {
                sendRequestFailure(session, badReqMsg);
                return true;
            }

            // Just return if didn't read the full line
            if (!lineCompleted)
            {
                // TODO: Issue another read?
                return true;
            }

            // If the line length is 0 it marks the end of headers
            if (line.length() == 0)
            {
                // Parse the headers for data we need (content-length)
                parseHeaders(session, &invalid);

                if (invalid)
                {
                    sendRequestFailure(session, badReqMsg);
                    return true;
                }

                if (session->contentLen == 0 &&
                    (session->method == HTTP_PUT ||
                     session->method == HTTP_POST))
                {
                    sendRequestFailure(session, lengthReqMsg);
                    return true;
                }

                // Allocate space for the content
                if (session->contentLen != 0)
                {
                    session->content = new char[session->contentLen];

                    // Copy as much as we can from the line buffer
                    if (session->lineBufferFilled > 0)
                    {
                        size_t copyable = session->lineBufferFilled;

                        if (copyable > session->contentLen)
                            copyable = session->contentLen;

                        ::memcpy(session->content,
                                 session->lineBuffer,
                                 copyable);
                        session->contentIndex += copyable;

                        // TODO: Adjust line buffer vars here if reusing session
                        session->lineBufferFilled = 0;
                    }
                }

                session->state = READING_BODY;
            }
            else if (session->lineBuffer[0] == ' ' ||
                     session->lineBuffer[0] == '\t')
            {
                size_t headerCount = session->headerLines.size();

                // A space or tab at start of line indicates a header continues on the next line
                // Of course it makes no sense if it's the first header line
                if (headerCount == 0)
                {
                    sendRequestFailure(session, badReqMsg);
                    return true;
                }

                // Build a header of the two values appended
                String& headerLine = session->headerLines.get(headerCount-1);
                headerLine.append(line);
            }
            else
            {
                size_t headerCount = session->headerLines.size();

                // If they exceeded the maximum number of headers, reject the request
                if (headerCount > HTTP_MAX_REQUEST_HEADERS)
                {
                    sendRequestFailure(session, badReqMsg);
                    return true;
                }

                // Make a copy of the header line
                session->headerLines.addBack(line);
            }

            // Flush the line read
            flushLine(session);

        } while (session->state == READING_HEADERS);
    }

    if (session->state == READING_BODY)
    {
        session->contentIndex += bytesTransfered;

        // If we read everything, toss the data into a _httpdWorkerData and
        // have the thread pool do the callback
        if (session->contentIndex == session->contentLen)
        {
            session->state = RESPONDING;

            session->_httpServer->_handler(*session->_httpServer,
                                           *session);

            return true;
        }
    }

    return false;
}

void HttpServer::writeCallback(AioSocket* aioSocket,
                               void* userData,
                               uint32 bytesTransfered,
                               const Error& error)
{
    HttpSession* session = (HttpSession*)userData;

    Console::outln("writeCallback");

    if (error.isSet())
    {
        Console::outln(String("writeCallback: ") + error.toString());
        session->_aioServer->socketClose(&session->_socket,
                                         closeCallback,
                                         session);
        return;
    }

    WriteEntry* prevHead = NULL;
    bool sessionComplete = false;

    session->lock.lock();

    // Remove current head
    prevHead = session->writeListHead;
    session->writeListHead = session->writeListHead->next;

    if (session->writeListHead == NULL)
        session->writeListTail = NULL;

    // Trigger new write if we have more data to write at the moment
    if (session->writeListHead != NULL)
    {
        session->_aioServer->socketWrite(&session->_socket,
                                         writeCallback,
                                         session,
                                         session->writeListHead->data,
                                         session->writeListHead->dataLen);
    }
    else
    {
        // Note if no longer writing
        session->writeActive = false;

        // Note if we need to end the session
        sessionComplete = session->writesComplete;
    }

    session->lock.unlock();

    // Free the removed WriteEntry
    if (prevHead->freeData)
        delete[] prevHead->data;

    delete prevHead;

    // Close the session if everything has been written
    if (sessionComplete)
    {
        session->_aioServer->socketClose(&session->_socket,
                                         closeCallback,
                                         session);
    }
}

void HttpServer::closeCallback(AioSocket* aioSocket,
                               void* userData,
                               const Error& error)
{
    HttpSession* session = (HttpSession*)userData;

    Console::outln("closeCallback");

    if (error.isSet())
    {
        Console::outln(error.toString());
    }

    delete session;
}