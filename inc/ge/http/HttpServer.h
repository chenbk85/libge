// HttpServer.h

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <ge/aio/AioSocket.h>
#include <ge/aio/SocketService.h>
#include <ge/data/List.h>
#include <ge/http/Http.h>
#include <ge/http/HttpSession.h>
#include <ge/text/StringRef.h>

class HttpSession;

/*
 * A basic HTTP daemon.
 *
 * Supports the following:
 *
 * GET/POST/PUT
 * Basic header parsing
 * Automatic 100 Continue responses
 *
 * Does not support:
 *
 * Chunked requests
 * Chunked responses
 * Persistant connection
 */
class HttpServer
{
    friend class HttpSession;

public:
    // Function that can handle an incomming HTTP request
    typedef void (*httpHandler_func)(HttpServer&  server,
                                     HttpSession& session);

    HttpServer();
    ~HttpServer();

    HttpServer(HttpServer&& other);
    HttpServer& operator=(HttpServer&& other);

    /*! \brief Starts serving HTTP requests
     *
     * \param socketService   SocketService to run on
     * \param port            Port to accept traffic on
     * \param handler         Handler function for HTTP requests
     */
    void startServing(SocketService* socketService,
                      uint32 port,
                      httpHandler_func handler);

    /*! \brief Shuts down an existing HTTP daemon. This will close the
     *         socket being used to accept connections and cause any thread
     *         blocking on bnetHttpd_BeginAccepting to unblock. The server
     *         will not completely shutdown until you destroy it.
     */
    void shutdown();

private:
    HttpServer(const HttpServer& other) DELETED;
    HttpServer& operator=(const HttpServer& other) DELETED;

    static 
    StringRef headerMatchExtract(const StringRef headerLine,
                                 const StringRef expectedKey);

    static
    void parseFirstRequestLine(const StringRef line,
                               HttpSession*    session,
                               bool*           invalid);

    static
    void parseHeaders(HttpSession*    session,
                      bool*           invalid);

    static
    void addWriteData(HttpSession*    session,
                      char*           data,
                      size_t          dataLen,
                      bool            freeData,
                      bool            lastData);

    static
    StringRef tryReadLine(HttpSession* session,
                          size_t       bytesRead,
                          bool*        lineCompleted,
                          bool*        invalid);

    static
    void flushLine(HttpSession* session);

    static
    void responseHeadersToBuffer(HttpSession* session,
                                 char*        dest);

    static
    void sendRequestFailure(HttpSession* session,
                            StringRef    message);

    static
    bool readHandler(HttpSession* session,
                     AioSocket* aioSocket,
                     uint32 bytesTransfered);
    
    static
    void acceptCallback(AioSocket* aioSocket,
                        AioSocket* acceptedSocket,
                        void* userData,
                        const Error& error);

    static
    void readCallback(AioSocket* aioSocket,
                      void* userData,
                      uint32 bytesTransfered,
                      const Error& error);

    static
    void writeCallback(AioSocket* aioSocket,
                       void* userData,
                       uint32 bytesTransfered,
                       const Error& error);


    SocketService* _socketService;
    httpHandler_func _handler;
    AioSocket _acceptSockIpv4;
    AioSocket _acceptSockIpv6;
    HttpSession* _pendingSessionIpv4;
    HttpSession* _pendingSessionIpv6;
};

#endif // HTTP_SERVER_H
