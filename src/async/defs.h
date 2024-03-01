#include <vector>
#ifndef _ASYNC_DEFS_H_
#define _ASYNC_DEFS_H_

#define HTTP_SVR_NS_BEGIN
#define HTTP_SVR_NS_END
#define USING_HTTP_SVR_NS using namespace __http_svr_ns__;

#define HTTP_HEAD_11                "HTTP/1.1"
#define HTTP_HEAD_11_LEN            8

#define HTTP_HEAD_10                "HTTP/1.0"
#define HTTP_HEAD_10_LEN            8

#define HTTP_CONTENT_TYPE           "Content-Type:"
#define HTTP_CONTENT_TYPE_LEN       13

#define CONTENT_LENGTH_STRING       "Content-Length:"
#define CONTENT_LEN                 15

#define HTTP_STATUS                 "Status:"
#define HTTP_STATUS_LEN             7

#define STANDARD_LINE_TAG           "\r\n"
#define STANDARD_LINE_TAG_LEN       2

#define NONSTANDARD_LINE_TAG        "\n"
#define NONSTANDARD_LINE_TAG_LEN    1

#define STANDARD_HEAD_TAG           "\r\n\r\n"
#define STANDARD_HEAD_TAG_LEN       4

#define NONSTANDARD_HEAD_TAG        "\n\n"
#define NONSTANDARD_HEAD_TAG_LEN    2

#define LONG_MSG_LEN                256
#define SHORT_MSG_LEN               64

#define MAX_HEADER_ITEM_NUMS        40

#define CHUNK_TAG_LEN               6
#define CHUNK_END_TAG               "0\r\n\r\n"
#define CHUNK_END_TAG_LEN           5

#define MAX_WEB_RECV_LEN            102400
#define URL_BEGIN_STRING            " "
#define URL_END_STRING              "?"
#define HEAD_END_CHAR               '\n'

/*#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 	            1024
#endif
*/
#define MAX_CGI_COUNT               512

#define MAX_HTTP_HEAD_LEN           2048

#define RSP_NORMAL_ITEM             0
#define RSP_LAST_ITEM               1

#define MINIMUM_CHUNK               1024//1k

enum CConnState {
    CONN_IDLE,

    CONN_FATAL_ERROR,
    CONN_DATA_ERROR,

    CONN_CONNECTING,
    CONN_DISCONNECT,
    CONN_CONNECTED,

    CONN_DATA_SENDING,
    CONN_DATA_RECVING,

    CONN_SEND_DONE,
    CONN_RECV_DONE,

    CONN_APPEND_SENDING,
    CONN_APPEND_DONE
};

enum TDecodeStatus {
    DECODE_FATAL_ERROR,
    DECODE_DATA_ERROR,

    DECODE_WAIT_HEAD,
    DECODE_WAIT_CONTENT,

    DECODE_URL_DONE,
    DECODE_HEAD_DONE,
    DECODE_CONTENT_DONE,
    DECODE_DONE,

    DECODE_DISCONNECT_BY_USER,
    DECODE_NO_FOUND_URL
};

enum HTTP_STATUS_CODE {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_MOVED_TEMPORARILY = 302,
    HTTP_STATUS_NOT_MODIFIED = 304,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_INSUFFICIENT_DATA = 399,
    HTTP_MAX_STATUS_CODE = 599
};

typedef struct tagRspList {
    char*   _buffer;
    int     _len;
    int     _sent;
    int     _type;

    struct tagRspList* _next;

} TRspList;

typedef struct s_log_buf_tag {
    std::vector<unsigned long> v_point;
    unsigned int size;

} s_log_buf;
#endif
