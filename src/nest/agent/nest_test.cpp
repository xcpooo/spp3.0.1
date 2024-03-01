/**
 *  文件名： nest_test.cpp
 *  功  能： 
 *
 *  版  本： v1.0
 *  ----------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include "errno.h"
#include "nest_proto.h"

using namespace std;


/**
 *  配置信息定义
 */
typedef struct main_config_tag
{
    vector<string>  servers;
    uint32_t        setid;
    string          agent;
    uint32_t        cmd;
}ENV_CONFIG_T;

typedef enum 
{
    SET_NODE     = 1,
    DEL_NODE     = 2,
    ADD_PROC     = 3,
    DEL_PROC     = 4,
    RESTART_PROC = 5,
    INIT_PROC    = 6,
    RESTART_ALL  = 7,
    SERVICE_INFO = 8,

    UNKNOWN_OP
}OP_TYPE_EN;


/**
 *  全局变量定义区域
 */
ENV_CONFIG_T  g_config;



/**
 *  功能：提示帮助信息
 *  描述：不打印设置、清除的帮助，防止误操作
 *  参数：无
 *  返回：0 成功
 */
void  UsageInfo() 
{
    printf("*************************************************\n"); 
    printf("nest cmd list:\n");
    printf("node init:      1 [agentip] [setid] [server1] [server2];\n"); 
    printf("node term:      2 [agentip] [setid];\n"); 
    printf("proc add :      3 [agentip] [setid] [sname] [path] [sid] [type] [pno] [proxy] [port] [proxyno];\n"); 
    printf("proc del :      4 [agentip] [setid] [sid] [type] [pno] [pid];\n");
    printf("proc restart:   5 [agentip] [setid] [sname] [path] [[sid] [type] [pno]];\n");
    printf("proc init :     6 [sid] [type] [pno] [pid] [proxyip] [port] [sname] [pname];\n");
    printf("restart all:    7 [agentip] [setid] [sname] [path] ;\n");
    printf("restart all:    8 [agentip] [setid] [sname] ;\n");
    printf("*************************************************\n"); 

    return;
}

/**
 *  功能：分析参数信息
 *  描述：主函数
 *  参数：IN   argc  - 参数个数
 *             argv  - 参数数组
 *        OUT        - 更新全局配置
 *  返回：小于0失败
 */
int ParseArgs(int argc, char* argv[])
{
    // 提示信息
    if (argc < 2)
    {
        UsageInfo();
        return -1;
    }

    // 获取命令信息
    g_config.cmd = (unsigned int)strtoul(argv[1], NULL, 10);

    return 0;
}


uint64_t GetSystemMS(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL);
};




int32_t NestTcpSendRcv(struct sockaddr_in* dest, char* req_buff, uint32_t req_len, 
                            char* rsp_buff, uint32_t& rsp_len, uint32_t timewait)
{
    uint64_t start = GetSystemMS();

    // 1. 打开socket 建立连接
    int32_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("create tcp socket failed, error: %d\n", errno);
        return -1;
    }
    
    struct timeval tv;
    tv.tv_sec = timewait / 1000;
    tv.tv_usec = timewait * 1000 % 1000000 ;

    int32_t rc = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (rc < 0)
    {
        close(sock);
        printf("set TCP timeout failed, ret %d\n", rc);
        return -1;
    }

    rc = connect(sock, (struct sockaddr*)dest, (int)sizeof(*dest));
    if (rc < 0)
    {
        close(sock);
        printf("conncet TCP  failed, ret %d\n", rc);
        return -2;
    }
    printf("Tcp connect OK\n");

    tv.tv_sec = 0;
    tv.tv_usec = 20 * 1000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 2. 循环发送直至OK
    int32_t has_send = 0;
    while (has_send < (int32_t)req_len)
    {
        uint64_t now = GetSystemMS();
        if ((now - start) > timewait)
        {
            close(sock);
            printf("send timeout, ret %d\n", rc);
            return -3;
        }
    
        int32_t bytes = send(sock,  req_buff + has_send, req_len - has_send, 0);
        if (bytes > 0)
        {
            has_send += bytes;
        }
        else if(bytes == 0)
        {       
            close(sock);
            printf("req send failed, remote closed\n");
            return -4;
        }       
        else    
        {       
            if(errno==EINTR || errno==EAGAIN || errno==EINPROGRESS)
            {       
                continue;
            }       
            close(sock);
            printf("req send failed, ret %d, [%m]\n", bytes); 
            return -5;
        }   

  }

    // 3. 循环接收直至报文完整
    int32_t has_recv = 0;
    while (has_recv < (int32_t)rsp_len)
    {
        uint64_t now = GetSystemMS();
        if ((now - start) > timewait)
        {
            close(sock);
            printf("recv timeout, ret %d\n", rc);
            return -4;
        }
    
        int32_t bytes = recv(sock,  rsp_buff + has_recv, rsp_len - has_recv, 0);
        if (bytes > 0)
        {
            has_recv += bytes;
     
            // 检查报文
            string err_msg;
            NestPkgInfo pkg_info = {0};
            pkg_info.msg_buff  = rsp_buff;
            pkg_info.buff_len  = has_recv;
            
            rc = NestProtoCheck(&pkg_info, err_msg);
            if (rc < 0)
            {
                close(sock);
                printf("rsp check failed, ret %d\n", rc);
                return -3;
            }
            else if (rc == 0)
            {
                continue;
            }
            else
            {
                rsp_len = has_recv;
                break;
            }
        }
        else if(bytes == 0)
        {
            close(sock);
            printf("rsp recv failed, remote closed\n");
            return -4;
        }
        else
        {
            if(errno==EINTR || errno==EAGAIN || errno==EINPROGRESS)
            {
                continue;
            }
            close(sock);
            printf("rsp rec failed, ret %d, [%m]\n", bytes);
            return -5;
        }
    }
    
    // 成功返回    
    close(sock);
    return 0;
}  

/**
 * @brief 向本地地址发送并等待应答消息
 */
int32_t NestUnixSendRcv(struct sockaddr_un* addr, const void* data, uint32_t len, 
                          void* buff, uint32_t& buff_len, uint32_t time_wait)
{
    int32_t sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("socket create failed(%m)\n");
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), NEST_THREAD_UNIX_PATH"_%u", getpid());
    
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    snprintf(uaddr.sun_path, sizeof(uaddr.sun_path), "%s", path);
    uint32_t addr_len = SUN_LEN(&uaddr);
    uaddr.sun_path[0] = '\0';
    if (bind(sockfd, (struct sockaddr*)&uaddr, addr_len) < 0)
    {
        printf("socket bind failed(%m)\n");
        close(sockfd);
        return -1;
    }

    // 发送超时固定10ms, 接收超时按实际情况
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10 * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    tv.tv_sec = time_wait / 1000;
    tv.tv_usec = (time_wait % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint32_t dst_addr_len = SUN_LEN(addr);
    addr->sun_path[0] = '\0';  // 新版默认抽象的address
    int32_t bytes = sendto(sockfd, data, len, 0, (struct sockaddr*)addr, dst_addr_len);
    if (bytes < 0)
    {
        printf("socket send failed, dst (%m)\n");
        close(sockfd);
        return -2;
    }

    bytes = recvfrom(sockfd, buff, buff_len, 0, NULL, 0);
    if (bytes < 0)
    {
        printf("socket recv failed(%m)\n");
        close(sockfd);
        return -3;
    }

    buff_len = bytes;
    close(sockfd);
    return bytes;   
}                          




int DoSetNode(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 1 agentip setid server1 server2
    if (argc < 5)
    {
        UsageInfo();
        return -1;
    }

    // 2. 获取信息
    string nodeip = argv[2];
    struct sockaddr_in dest = {0};
    dest.sin_family         = AF_INET;
    dest.sin_addr.s_addr    = inet_addr(argv[2]);
    dest.sin_port           = htons(NEST_AGENT_PORT);

    uint32_t setid = strtoul(argv[3], NULL, 10);

    std::vector<string> servers;
    for (int32_t i = 4; i < argc; i++)
    {
        servers.push_back(argv[i]);
    }
    
    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_ADMIN_CMD_ADD_NODE);
    head.set_set_id(setid);
    head.set_sequence(++seq);

    nest_sched_node_init_req req_body;
    req_body.set_node_ip(nodeip);
    req_body.set_task_id((uint32_t)random());
    req_body.set_set_id(setid);
    for (uint32_t i = 0; i < servers.size(); i++)
    {
        req_body.add_servers(servers[i]);
    }

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestTcpSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 500);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    nest_sched_node_init_rsp rsp_body;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    } 

    printf("Rsp pkg, head[%s], body[%s]\n", head.ShortDebugString().c_str(),
           rsp_body.ShortDebugString().c_str());

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("rsp failed, result %u - %s\n", head.result(), head.err_msg().c_str());
    }
    else
    {
        printf("rsp success, result %u\n", head.result());
    }

    return 0;

}


int DoDelNode(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 2 agentip setid
    if (argc < 4)
    {
        UsageInfo();
        return -1;
    }

    // 2. 获取信息
    string nodeip = argv[2];
    struct sockaddr_in dest = {0};
    dest.sin_family         = AF_INET;
    dest.sin_addr.s_addr    = inet_addr(argv[2]);
    dest.sin_port           = htons(NEST_AGENT_PORT);

    uint32_t setid = strtoul(argv[3], NULL, 10);

    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_ADMIN_CMD_DEL_NODE);
    head.set_set_id(setid);
    head.set_sequence(++seq);

    nest_sched_node_term_req req_body;
    req_body.set_node_ip(nodeip);
    req_body.set_task_id((uint32_t)random());
    req_body.set_set_id(setid);

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestTcpSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 500);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    } 

    printf("Rsp pkg, head[%s], body[-]\n", head.ShortDebugString().c_str());

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("rsp failed, result %u - %s\n", head.result(), head.err_msg().c_str());
    }
    else
    {
        printf("rsp success, result %u\n", head.result());
    }

    return 0;

}


int DoAddProc(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 3 [agentip] [setid] [sname] [path] [sid] [type] [pno] [proxy] [port] [proxyno]
    if (argc < 10)
    {
        UsageInfo();
        return -1;
    }

    // 2. 获取信息
    string nodeip = argv[2];
    struct sockaddr_in dest = {0};
    dest.sin_family         = AF_INET;
    dest.sin_addr.s_addr    = inet_addr(argv[2]);
    dest.sin_port           = htons(NEST_AGENT_PORT);

    uint32_t setid = strtoul(argv[3], NULL, 10);
    string service_name = argv[4];
    string path_name    = argv[5];
    uint32_t service_id = strtoul(argv[6], NULL, 10);
    uint32_t proc_type  = strtoul(argv[7], NULL, 10);

    string proxy[10];
    uint32_t proc_no[10];
    uint16_t proxy_port[10];
    uint32_t proxy_no[10];
    int32_t proc_cnt = 0;
    for (int32_t i = 8; i < argc; i+=4)
    {
        proc_no[proc_cnt] = strtoul(argv[i], NULL, 10);

        if ((i + 3) < argc) {
            proxy[proc_cnt] = argv[i+1];
            proxy_port[proc_cnt] = strtoul(argv[i+2], NULL, 10);
            proxy_no[proc_cnt] = strtoul(argv[i+3], NULL, 10);
        }
        
        proc_cnt++;
    }
    
    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_SCHED_CMD_ADD_PROC);
    head.set_set_id(setid);
    head.set_sequence(++seq);

    nest_sched_add_proc_req req_body;
    req_body.set_task_id((uint32_t)random());
    req_body.set_service_name(service_name);
    req_body.set_service_id(service_id);
    req_body.set_proc_type(proc_type);
    req_body.set_package_name(path_name);
    
    for (int32_t i = 0; i < proc_cnt; i++)
    {
        nest_proc_base* proc_item = req_body.add_proc_conf();
        proc_item->set_service_id(service_id);
        proc_item->set_proc_no(proc_no[i]);
        proc_item->set_proc_type(proc_type);
        proc_item->set_proxy_ip(proxy[i]);
        proc_item->set_proxy_port(proxy_port[i]);
        proc_item->set_proxy_proc_no(proxy_no[i]);
    }

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestTcpSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 500);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    nest_sched_add_proc_rsp rsp_body;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    } 

    printf("Rsp pkg, head[%s], body[%s]\n", head.ShortDebugString().c_str(),
           rsp_body.ShortDebugString().c_str());

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("rsp failed, result %u - %s\n", head.result(), head.err_msg().c_str());
    }
    else
    {
        printf("rsp success, result %u\n", head.result());
    }

    return 0;

}




int DoDelProc(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 3 [agentip] [setid] [sid] [type] [pno] [pid]
    if (argc < 8)
    {
        UsageInfo();
        return -1;
    }

    // 2. 获取信息
    string nodeip = argv[2];
    struct sockaddr_in dest = {0};
    dest.sin_family         = AF_INET;
    dest.sin_addr.s_addr    = inet_addr(argv[2]);
    dest.sin_port           = htons(NEST_AGENT_PORT);

    uint32_t setid = strtoul(argv[3], NULL, 10);
    uint32_t service_id = strtoul(argv[4], NULL, 10);
    uint32_t proc_type  = strtoul(argv[5], NULL, 10);
    uint32_t proc_no = strtoul(argv[6], NULL, 10);
    uint32_t proc_id = strtoul(argv[7], NULL, 10);
    
    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_SCHED_CMD_DEL_PROC);
    head.set_set_id(setid);
    head.set_sequence(++seq);

    nest_sched_del_proc_req req_body;
    req_body.set_task_id((uint32_t)random());
    
    {
        nest_proc_base* proc_item = req_body.add_proc_list();
        proc_item->set_service_id(service_id);
        proc_item->set_proc_no(proc_no);
        proc_item->set_proc_pid(proc_id);
        proc_item->set_proc_type(proc_type);
    }

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestTcpSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 500);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    nest_sched_del_proc_rsp rsp_body;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    } 

    printf("Rsp pkg, head[%s], body[%s]\n", head.ShortDebugString().c_str(),
           rsp_body.ShortDebugString().c_str());

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("rsp failed, result %u - %s\n", head.result(), head.err_msg().c_str());
    }
    else
    {
        printf("rsp success, result %u\n", head.result());
    }

    return 0;

}


int DoRestartProc(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 5 [agentip] [setid] [sname] [path] [[sid] [type] [pno]]
    if (argc < 9)
    {
        UsageInfo();
        return -1;
    }

    // 2. 获取信息
    string nodeip = argv[2];
    struct sockaddr_in dest = {0};
    dest.sin_family         = AF_INET;
    dest.sin_addr.s_addr    = inet_addr(argv[2]);
    dest.sin_port           = htons(NEST_AGENT_PORT);

    uint32_t setid = strtoul(argv[3], NULL, 10);
    string service_name = argv[4];
    string path_name    = argv[5];

    uint32_t service_id[10];
    uint32_t proc_type[10];
    uint32_t proc_no[10];
    
    int32_t proc_cnt = 0;
    for (int32_t i = 6; i < argc; i+=3)
    {
        if ((i + 2) < argc) {
            service_id[proc_cnt] = strtoul(argv[i], NULL, 10);
            proc_type[proc_cnt] = strtoul(argv[i+1], NULL, 10);
            proc_no[proc_cnt] = strtoul(argv[i+2], NULL, 10);
            proc_cnt++;
        }
    }
    
    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_SCHED_CMD_RESTART_PROC);
    head.set_set_id(setid);
    head.set_sequence(++seq);

    nest_sched_restart_proc_req req_body;
    req_body.set_task_id((uint32_t)random());
    req_body.set_service_name(service_name);
    req_body.set_package_name(path_name);
    
    for (int32_t i = 0; i < proc_cnt; i++)
    {
        nest_proc_base* proc_item = req_body.add_proc_conf();
        proc_item->set_service_id(service_id[i]);
        proc_item->set_proc_no(proc_no[i]);
        proc_item->set_proc_type(proc_type[i]);
    }

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestTcpSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 500);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    nest_sched_restart_proc_rsp rsp_body;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    } 

    printf("Rsp pkg, head[%s], body[%s]\n", head.ShortDebugString().c_str(),
           rsp_body.ShortDebugString().c_str());

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("rsp failed, result %u - %s\n", head.result(), head.err_msg().c_str());
    }
    else
    {
        printf("rsp success, result %u\n", head.result());
    }

    return 0;

}


int DoRestartAll(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 7 [agentip] [setid] [sname] [path]
    if (argc < 6)
    {
        UsageInfo();
        return -1;
    }

    // 2. 获取信息
    string nodeip = argv[2];
    struct sockaddr_in dest = {0};
    dest.sin_family         = AF_INET;
    dest.sin_addr.s_addr    = inet_addr(argv[2]);
    dest.sin_port           = htons(NEST_AGENT_PORT);

    uint32_t setid = strtoul(argv[3], NULL, 10);
    string service_name = argv[4];
    string path_name    = argv[5];
    
    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_SCHED_CMD_RESTART_PROC);
    head.set_set_id(setid);
    head.set_sequence(++seq);

    nest_sched_restart_proc_req req_body;
    req_body.set_task_id((uint32_t)random());
    req_body.set_service_name(service_name);
    req_body.set_package_name(path_name);
    req_body.set_proc_all(1);

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestTcpSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 60*1000);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    nest_sched_restart_proc_rsp rsp_body;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    } 

    printf("Rsp pkg, head[%s], body[%s]\n", head.ShortDebugString().c_str(),
           rsp_body.ShortDebugString().c_str());

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("rsp failed, result %u - %s\n", head.result(), head.err_msg().c_str());
    }
    else
    {
        printf("success, proc num: %u\n", rsp_body.proc_info_size());
    }

    return 0;

}



int DoInitProc(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 6 [sid] [type] [pno] [pid] [proxyip] [port] [sname] [pname]
    if (argc < 8)
    {
        UsageInfo();
        return -1;
    }

    
    uint32_t service_id = strtoul(argv[2], NULL, 10);
    uint32_t proc_type  = strtoul(argv[3], NULL, 10);
    uint32_t proc_no    = strtoul(argv[4], NULL, 10);
    uint32_t proc_pid   = strtoul(argv[5], NULL, 10);
    string   proxyip    = argv[6];
    uint16_t proxyport  = strtoul(argv[7], NULL, 10);

    // 2. 获取信息
    struct sockaddr_un dest = {0};
    dest.sun_family = AF_UNIX;
    snprintf(dest.sun_path, sizeof(dest.sun_path), NEST_PROC_UNIX_PATH"_%u", proc_pid);

    
    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_PROC);
    head.set_sub_cmd(NEST_PROC_CMD_INIT_INFO);
    head.set_sequence(++seq);

    nest_proc_init_req req_body;
    req_body.set_service_name(argv[8]);
    req_body.set_package_name(argv[9]);    
    nest_proc_base* proc_conf = req_body.mutable_proc();
    proc_conf->set_service_id(service_id);
    proc_conf->set_proc_type(proc_type);
    proc_conf->set_proc_no(proc_no);
    proc_conf->set_proc_pid(proc_pid);
    proc_conf->set_proxy_ip(proxyip);
    proc_conf->set_proxy_port(proxyport);

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestUnixSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 500);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    nest_proc_init_rsp rsp_body;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    } 

    printf("Rsp pkg, head[%s], body[%s]\n", head.ShortDebugString().c_str(),
           rsp_body.ShortDebugString().c_str());

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("rsp failed, result %u - %s\n", head.result(), head.err_msg().c_str());
    }
    else
    {
        printf("rsp success, result %u\n", head.result());
    }

    return 0;

}

int DoGetServiceInfo(int argc, char* argv[])
{
    static char req_buff[65535];
    static char rsp_buff[65535];
    static uint32_t seq = 0;
    
    // 1./nest_test 8 agentip setid sname
    if (argc < 5)
    {
        UsageInfo();
        return -1;
    }

    // 2. 获取信息
    string nodeip = argv[2];
    struct sockaddr_in dest = {0};
    dest.sin_family         = AF_INET;
    dest.sin_addr.s_addr    = inet_addr(argv[2]);
    dest.sin_port           = htons(NEST_AGENT_PORT);

    uint32_t setid = strtoul(argv[3], NULL, 10);
    
    // 3. 封装请求
    nest_msg_head  head;
    head.set_msg_type(NEST_PROTO_TYPE_SCHEDULE);
    head.set_sub_cmd(NEST_SCHED_CMD_SERVICE_INFO);
    head.set_set_id(setid);
    head.set_sequence(++seq);

    nest_sched_service_info_req req_body;
    req_body.set_service_name(argv[4]);
    req_body.set_task_id((uint32_t)random());

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        printf("pb format string pack failed, error!\n");
        return -2;
    }
    
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = req_buff;
    rsp_pkg.buff_len    = 65535;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        printf("resp msg pack failed, ret %d, err: %s\n", 
                ret_len, err_msg.c_str());
        return -3;
    }

    printf("Req pkg, head[%s], body[%s]", head.ShortDebugString().c_str(),
           req_body.ShortDebugString().c_str());

    // 4. nest tcp send recv
    uint32_t rsp_len = 65535;
    int32_t ret = NestTcpSendRcv(&dest, req_buff, ret_len, rsp_buff, rsp_len, 500);
    if (ret < 0)
    {
        printf("send recv failed, ret %d\n", ret);
        return -4;
    }

    // 5. 解析应答结果
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = rsp_buff;
    pkg_info.buff_len  = rsp_len;
    
    ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        printf("rsp check failed, ret %d\n", ret);
        return -5;
    }

    nest_sched_service_info_rsp rsp_body;
    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        printf("rsp parse failed, error\n");
        return -6;
    }

    // 6. 结果打印
    if (head.result() != 0)
    {
        printf("Rsp pkg, head[%s], body[%s]\n", head.ShortDebugString().c_str(),
               rsp_body.ShortDebugString().c_str());
    }
    else
    {
        printf("service %s process count: %u\n", argv[4], rsp_body.proc_cnt());
    }

    return 0;

}



/**
 *  功能：主函数
 *  描述：主函数
 *  参数：IN   argc  - 参数个数
 *             argv  - 参数数组
 *  返回：退出的信息
 */
int main(int argc, char* argv[])
{
    // 1. 分析参数信息，失败打印帮助
    int iRet = ParseArgs(argc, argv);
    if (iRet < 0)
    {
        return -1;
    }

    // 2. 按操作处理
    switch (g_config.cmd)
    {
        case SET_NODE:
            DoSetNode(argc, argv);
            break;

        case DEL_NODE:
            DoDelNode(argc, argv);
            break;

        case ADD_PROC:
            DoAddProc(argc, argv);
            break;

        case DEL_PROC:
            DoDelProc(argc, argv);
            break;

        case RESTART_PROC:
            DoRestartProc(argc, argv);
            break;

        case INIT_PROC:
            DoInitProc(argc, argv);
            break;

        case RESTART_ALL:
            DoRestartAll(argc, argv);
            break;

        case SERVICE_INFO:
            DoGetServiceInfo(argc, argv);
            break;

        default:
            printf("Unknown cmd (%u) ok!\n", g_config.cmd);
            break;
        
    }

	return 0;
}


