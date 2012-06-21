// Http.h

#ifndef HTTP_H
#define HTTP_H

// HTTP version enum
enum HttpProt_enum
{
    HTTP_PROT_10,
    HTTP_PROT_11
};

// Enum for HTTP request type
enum HttpMethod_enum
{
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_TRACE
};

// TODO: Move below to private header

// Maximum line length, including URL line. This affects session size.
#define HTTP_MAX_LINE (1024*4)

// Maximum number of request header lines. Sanity check.
#define HTTP_MAX_REQUEST_HEADERS 256

// Session state enum
enum SessionState_enum
{
    READING_FIRST_LINE,
    READING_HEADERS,
    READING_BODY,
    RESPONDING,
};

/*
 * Entry allocated for pending write data
 */
class WriteEntry
{
public:
    WriteEntry* next;

    char*   data;
    size_t  dataLen;
    bool    freeData;
};

#endif // HTTP_H
