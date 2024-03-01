/*
 * =====================================================================================
 *
 *       Filename:  interface.cpp
 *
 *    Description:  the async interface
 *
 *        Version:  1.0
 *        Created:  06/17/2009 09:37:13 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  allanhuang (for fun), allanhuang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "interface.h"
#include "asyn_api.h"
#include "tprocmon.h"
#include "tcommu.h"
//#include "ttc_api/ttcapi.h"

using namespace tbase::tprocmon;
using namespace tbase::tcommu;


extern "C" int ttc_recv(int event, int sessionId, void* comm_param, char* buf, int len)
{
    int fd = *(int*)comm_param;

    if (fd < 0) {
        return -1;
    }

    int recv_ret = recv(fd, buf, len, 0);

    if (recv_ret < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
            return 0;

        return -1;
    }

    return recv_ret;
}

extern "C" int tdb_recv(int event, int sessionId, void* comm_param, char* buf, int len)
{
    int fd = *(int*)comm_param;

    if (fd < 0) {
        return -1;
    }

    int recv_ret = recv(fd, buf, len, 0);

    if (recv_ret < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
            return 0;

        return -1;
    }

    return recv_ret;
}

extern "C" int tcp_recv(int event, int sessionId, void* comm_param, char* buf, int len)
{
    int fd = *(int*)comm_param;

    if (fd < 0) {
        return -1;
    }

    int recv_ret = recv(fd, buf, len, 0);

    if (recv_ret == 0)
        return -1;

    if (recv_ret < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
            return 0;

        return -1;
    }

    return recv_ret;
}

extern "C" int udp_recv(int event, int sessionId, void* comm_param, char* buf, int len)
{
    int fd = *(int*)comm_param;

    if (fd < 0) {
        return -1;
    }

    sockaddr_in addr;
    socklen_t socklen=4;
   
   int recv_ret = recvfrom(fd, buf, len, 0,(sockaddr*)&addr,&socklen);

    if (recv_ret == 0)
        return -1;
  
   if (recv_ret < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
            return 0;
        return -1;
    }

    return recv_ret;
}

extern "C" int ttc_input(void* input_param, unsigned sessionId , void* blob, void* server)
{
    /*
    static TTC::Server stTTCServer;
    blob_type* b = (blob_type*)blob;
    int ret = stTTCServer.CheckPacketSize(b->data, b->len);
    return ret;
    */
    return 0;
}
/*
extern "C" int tdb_input(void* input_param, unsigned sessionId , void* blob, void* server)
{
    StorageMessage *rsp = NULL;
    blob_type* b = (blob_type*)blob;
    unsigned rsp_msg_seq;
    static TdbPackager tp;
    int datalen = 0;
    int ret2 = tp.GetMessage(b->data, b->len, datalen, rsp_msg_seq, rsp);

    if (datalen > 0) {
        return b->len;
    } else {// 无完整的报文，socket应继续接收数据，然后再交给GetMessage处理
        return 0;
    }

}
*/
extern "C" int tcp_input(void* input_param, unsigned sessionId , void* blob, void* server)
{
    blob_type* b = (blob_type*)blob;

    if (b->len < 4)
        return 0;

    return *((unsigned *)(b->data));
}

extern "C" int udp_input(void* input_param, unsigned sessionId , void* blob, void* server)
{
    blob_type* b = (blob_type*)blob;
    return b->len;
}
