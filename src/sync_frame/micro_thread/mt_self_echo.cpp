#include<sys/types.h>
#include<sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mt_incl.h"

using namespace NS_MICRO_THREAD;

static bool run = true;

static struct sockaddr_in addr;

int mt_tcp_create_sock(void)
{
    int fd;
    int flag;

    // 创建socket
    fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        printf("create tcp socket failed, error: %m\n");
        return -1;
    }

    // 设置socket非阻塞
    flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
    {
        ::close(fd);
        printf("get fd flags failed, error: %m\n");
        return -2;
    }

    if (flag & O_NONBLOCK)
        return fd;

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK | O_NDELAY) == -1)
    {
        ::close(fd);
        printf("set fd flags failed, error: %m\n");
        return -3;
    }

    return fd;
}

void server(void* arg)
{
    int fd = mt_tcp_create_sock();
    if(fd<0)
    {
        run = false;
        printf("create listen socket failed\n");
        return;
    }


    int optval = 1;
    unsigned optlen = sizeof(optval);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(fd);
        printf("bind failed [%m]\n");
        return ;
    }

    if (listen(fd, 1024) < 0)
    {
        close(fd);
        printf("listen failed[%m]\n");
        return ;
    }
    
    int clt_fd = 0;
    int timeout = 10;
    char buf[1024];
    //int rcv_len = 0;
    int ret = 0;
    int msg_len = 0;
    int msg_max = 10<<20; // 10M
    int msg_min = 128;
    srand((int)time(0));
    while(run)
    {   
        struct sockaddr_in client_addr;
        int addr_len=sizeof(client_addr);;
        
        clt_fd = mt_accept(fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len, timeout);
        if(clt_fd<0)
        {
            mt_sleep(1);
            continue;
        }
        
        ret = mt_recv(clt_fd, (void*)buf,1,0,1000);
        if(ret<0)
        {
            close(clt_fd);
            printf("recv client data failed[%m]\n");
            mt_sleep(1);
            continue;
        }

        if(buf[0]==0)
        {
            run = false;
        }
        
        msg_len = msg_min+ rand()%msg_max;
        printf("server will send bytes: %d\n",msg_len);
        *(uint32_t*)buf = htonl(msg_len);
        ret = mt_send(clt_fd, buf, 4,0, 1000);
        if(ret!=4)
        {
            close(clt_fd);
            printf("svr send head failed[%m]\n");
            continue;
        }
        int left_bytes=msg_len - 4;
        int send_len = 0;
        while(left_bytes>0)
        {
            send_len = 1024;
            if(left_bytes<send_len)
            {
                send_len = left_bytes;
            }
            //printf("server send %d,left_bytes=%d\n", send_len,left_bytes);
            ret = mt_send(clt_fd,buf,send_len,0,1000);
            if(ret!=send_len)
            {   
                close(clt_fd);
                clt_fd = -1;
                printf("svr send body failed[%m]\n");
                break;
            }
            left_bytes -= send_len;
            mt_sleep(0);
        }

        if(clt_fd>0)     close(clt_fd);
            
        mt_sleep(1);
    }
    printf("server exit\n"); 
    
    
}

struct MsgCtx
{
    int check_count;
    int msg_id;
};

int TcpMsgChecker(void* buf, int len, bool closed, void* msg_ctx, bool& msg_len_detected)
{
    
    struct MsgCtx* ctx = (struct MsgCtx*)msg_ctx;
    
    ctx->check_count++;
    printf("#%d msg check msg times #%d, buf=%p, len=%d, closed=%d\n", ctx->msg_id, ctx->check_count, buf,len,closed);    

    if(len<4)
    {
        return 0;
    }

    
    int r_len=ntohl(*(uint32_t*)buf);
    //if(r_len!=len)
   // {
    //    return 0;
    //}
    msg_len_detected = true;

    return r_len;
}


void client(void* arg)
{
    //char buf[1024];
    
    struct MsgCtx ctx;
    void* rcv_buf = NULL;
    int   rcv_len = 0;
    bool keep_rcv_buf = true;
    int ret = 0;
    char snd_ch = 1;
    int count=0;
    while(true)
    {
        rcv_buf = NULL;
        rcv_len = 1;
        keep_rcv_buf = (((++count)%2) == 0);
        ctx.check_count = 0;
        ctx.msg_id = count;
        ret = mt_tcpsendrcv_ex((struct sockaddr_in*)&addr, (void*)&snd_ch, 1, rcv_buf, rcv_len,20000, &TcpMsgChecker, (void*)&ctx, MT_TCP_SHORT,keep_rcv_buf);
        if(ret<0)
        {
            printf("client send rcv failed[%m]\n");
            continue;
        }
        printf("#%d client tcp finished: rcv_len=%d, rcv_buf=%p, keep_rcv_buf=%d\n",count, rcv_len, rcv_buf,keep_rcv_buf);

        if(keep_rcv_buf)
        {
            if(rcv_buf==NULL)
            {
                printf("client should hold rcvbuf, something wrong\n");
                continue;
            }
            free(rcv_buf);
        }
    }

    printf("client exit!");
}

int main(int argc, char* argv[])
{
    memset((void*)&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(19999);

    mt_init_frame();

    mt_start_thread((void*)server,NULL);
    mt_sleep(100);
    mt_start_thread((void*)client,NULL);    

    while(run)
    {
        mt_sleep(10);
    };      
    
    printf("main exit");
}
