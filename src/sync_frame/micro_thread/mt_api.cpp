/**
 *  @filename mt_sys_call.cpp
 *  @info  ΢�̷߳�װϵͳapi, ͬ������΢�߳�API��ʵ���첽����
 */

#include "epoll_proxy.h"
#include "micro_thread.h"
#include "mt_connection.h"
#include "mt_api.h"

namespace NS_MICRO_THREAD {

/**
 * @brief ��������˿ڵ�socket�շ��ӿ�, ��socket������������, ҵ������֤������
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param rcv_buf -����Ӧ�����buff
 * @param buf_size -modify-����Ӧ�����buff��С, �ɹ�����ʱ, �޸�ΪӦ�������
 * @param timeout -��ʱʱ��, ��λms
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -3 ����Ӧ��ʧ��, �ɴ�ӡerrno, -10 ������Ч
 */
int mt_udpsendrcv(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, int timeout)
{
    int ret = 0;
    int rc  = 0;
    int flags = 1;
    struct sockaddr_in from_addr = {0};
    int addr_len = sizeof(from_addr);

    if(len<1 || buf_size<1 ||!dst || !pkg || !rcv_buf)
    {
        MTLOG_ERROR("mt_udpsendrcv input params invalid, dst[%p], pkg[%p], rcv_buf[%p], len[%d], buf_size[%d]",
            dst, pkg, rcv_buf, len, buf_size);
        return -10;  	
    }
    
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if ((sock < 0) || (ioctl(sock, FIONBIO, &flags) < 0))
    {
        MT_ATTR_API(320842, 1); // socketʧ��
        MTLOG_ERROR("mt_udpsendrcv new sock failed, sock: %d, errno: %d (%m)", sock, errno);
        ret = -1;
        goto EXIT_LABEL;
    }
    
    rc = MtFrame::sendto(sock, pkg, len, 0, (struct sockaddr*)dst, (int)sizeof(*dst), timeout);
    if (rc < 0)
    {
        MT_ATTR_API(320844, 1); // ����ʧ��
        MTLOG_ERROR("mt_udpsendrcv send failed, rc: %d, errno: %d (%m)", rc, errno);
        ret = -2;
        goto EXIT_LABEL;
    }

    rc = MtFrame::recvfrom(sock, rcv_buf, buf_size, 0, (struct sockaddr*)&from_addr, (socklen_t*)&addr_len, timeout);
    if (rc < 0)
    {
        MT_ATTR_API(320845, 1); // ����δ��ȫ�ɹ�
        MTLOG_ERROR("mt_udpsendrcv recv failed, rc: %d, errno: %d (%m)", rc, errno);
        ret = -3;
        goto EXIT_LABEL;
    }
    buf_size = rc;

EXIT_LABEL:

    if (sock > 0)
    {
        close(sock);
        sock = -1;
    }

    return ret; 
}

/**
 * @brief  ����TCP�׽��֣�������Ϊ������
 * @return >=0 �ɹ�, <0 ʧ��
 */
int mt_tcp_create_sock(void)
{
    int fd;
    int flag;

    // ����socket
    fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        MTLOG_ERROR("create tcp socket failed, error: %m");
        return -1;
    }

    // ����socket������
    flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
    {
        ::close(fd);
        MTLOG_ERROR("get fd flags failed, error: %m");
        return -2;
    }

    if (flag & O_NONBLOCK)
        return fd;

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK | O_NDELAY) == -1)
    {
        ::close(fd);
        MTLOG_ERROR("set fd flags failed, error: %m");
        return -3;
    }

    return fd;
}

/**
 * @brief TCP��ȡ������֪ͨ������socket
 */
static TcpKeepConn* mt_tcp_get_keep_conn(struct sockaddr_in* dst, int& sock)
{
    // 1. ��ȡ�߳�֪ͨע�����
    EpollerObj* ntfy_obj = NtfyObjMgr::Instance()->GetNtfyObj(NTFY_OBJ_THREAD, 0);
    if (NULL == ntfy_obj)
    {
        MTLOG_ERROR("get notify failed, logit");
        return NULL;
    }

    // 2. ��ȡ���Ӷ���, ����֪ͨ��Ϣ
    TcpKeepConn* conn = dynamic_cast<TcpKeepConn*>(ConnectionMgr::Instance()->GetConnection(OBJ_TCP_KEEP, dst));
    if (NULL == conn)
    {
        MTLOG_ERROR("get connection failed, dst[%p]", dst);
        NtfyObjMgr::Instance()->FreeNtfyObj(ntfy_obj);
        return NULL;
    }
    conn->SetNtfyObj(ntfy_obj);

    // 3. ��������socket���
    int osfd = conn->CreateSocket();
    if (osfd < 0)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, true);
        MTLOG_ERROR("create socket failed, ret[%d]", osfd);
        return NULL;
    }

    // 4. �ɹ���������
    sock = osfd;
    return conn;
}

/**
 * @brief TCPѭ������, ֱ������OK��ʱ
 *       [ע��] �����߲�Ҫ�����޸ĺ�������ֵ����֤��Ҫ��mt_tcpsendrcv�ȵ��ýӿڳ�ͻ [��Ҫ]
 */
static int mt_tcp_check_recv(int sock, char* rcv_buf, int &len, int flags, int timeout, MtFuncTcpMsgLen func)
{
    int recv_len = 0;
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    do
    {
        utime64_t cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
        if (cost_time > (utime64_t)timeout)
        {
            errno = ETIME;            
            MTLOG_ERROR("tcp socket[%d] recv not ok, timeout", sock);
            return -3;
        }

        int rc = MtFrame::recv(sock, (rcv_buf + recv_len), (len - recv_len), 0, (timeout - (int)cost_time));
        if (rc < 0)
        {
            MTLOG_ERROR("tcp socket[%d] recv failed ret[%d][%m]", sock, rc);
            return -3;
        }
		else if (rc == 0)
        {
        	len = recv_len;
            MTLOG_ERROR("tcp socket[%d] remote close", sock);
            return -7;
        }
        recv_len += rc;

        /* ��鱨�������� */
        rc = func(rcv_buf, recv_len);
        if (rc < 0)
        {
            MTLOG_ERROR("tcp socket[%d] user check pkg error[%d]", sock, rc);
            return -5;
        }
        else if (rc == 0) // ����δ������
        { 
            if (len == recv_len) // û�ռ��ٽ�����, ����
            {
                MTLOG_ERROR("tcp socket[%d] user check pkg not ok, but no more buff", sock);
                return -6;
            }
            continue;
        }
        else    // �ɹ����㱨�ĳ���
        { 
            if (rc > recv_len) // ���Ļ�δ��ȫ
            {
                continue;
            }
            else
            {
                len = rc;
                break;
            }
        }
    } while (true);

    return 0;
}

/**
 * @brief TCP��������ӳصķ�ʽ����IP/PORT����, ���ӱ���Ĭ��10����
 *        [ע��] tcp���շ���buff, ��������static����, ����������Ĵ��� [��Ҫ]
 *        [ע��] �޸Ľӿڣ���ע�ⲻҪ����޸ķ���ֵ������֤��mt_tcpsendrcv_ex����ֵƥ�� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param rcv_buf -����Ӧ�����buff
 * @param buf_size -modify-����Ӧ�����buff��С, �ɹ�����ʱ, �޸�ΪӦ�������
 * @param timeout -��ʱʱ��, ��λms
 * @param check_func -��ⱨ���Ƿ�ɹ����ﺯ��
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -3 ����Ӧ��ʧ��, 
 *          -4 ����ʧ��, -5 ��ⱨ��ʧ��, -6 ���տռ䲻��, -7 ��������ر����ӣ�-10 ������Ч
 */
int mt_tcpsendrcv(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, int timeout, MtFuncTcpMsgLen func)
{
    if (!dst || !pkg || !rcv_buf || !func || len<1 || buf_size<1) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p], len[%d], buf_size[%d]",
            dst, pkg, rcv_buf, func, len, buf_size);
        return -10;
    }

    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. ��ȡTCP���ӳض���, �ҽ�֪ͨ����
    int sock = -1;
    TcpKeepConn* conn = mt_tcp_get_keep_conn(dst, sock);
    if ((conn == NULL) || (sock < 0))
    {
        MTLOG_ERROR("socket[%d] get conn failed, ret[%m]", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. ���Լ����½�����
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    // 4. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = mt_tcp_check_recv(sock, (char*)rcv_buf, buf_size, 0, time_left, func);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] rcv failed, ret[%d][%m]", sock, rc);
        ret = rc;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    // ʧ����ǿ���ͷ�����, ����ʱ����
    if (conn != NULL)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, (ret < 0));
    }

    return ret;
}

/**
 * @brief TCP�������շ�����
 *        [ע��] tcp���շ���buff, ��������static����, ����������Ĵ��� [��Ҫ]
 *        [ע��] �޸Ľӿڣ���ע�ⲻҪ����޸ķ���ֵ������֤��mt_tcpsendrcv_ex����ֵƥ�� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param rcv_buf -����Ӧ�����buff
 * @param buf_size -modify-����Ӧ�����buff��С, �ɹ�����ʱ, �޸�ΪӦ�������
 * @param timeout -��ʱʱ��, ��λms
 * @param check_func -��ⱨ���Ƿ�ɹ����ﺯ��
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -3 ����Ӧ��ʧ��, 
 *          -4 ����ʧ��, -5 ��ⱨ��ʧ��, -6 ���տռ䲻��, -7 ��������ر����ӣ�-10 ������Ч
 */
int mt_tcpsendrcv_short(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, int timeout, MtFuncTcpMsgLen func)
{
    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. �������
    if (!dst || !pkg || !rcv_buf || !func || len<1 || buf_size<1) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p], len[%d], buf_size[%d]",
            dst, pkg, rcv_buf, func, len, buf_size);
        return -10;
    }

    // 2. ����TCP socket
    int sock;
    sock = mt_tcp_create_sock();
    if (sock < 0)
    {
        MTLOG_ERROR("create tcp socket failed, ret: %d", sock);
        return -1;
    }

    // 3. ���Լ����½�����
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 4. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    // 5. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = mt_tcp_check_recv(sock, (char*)rcv_buf, buf_size, 0, time_left, func);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] rcv failed, ret[%d][%m]", sock, rc);
        ret = rc;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:
    if (sock >= 0)
        ::close(sock);

    return ret;
}


/**
 * @brief TCP��������ӳصķ�ʽ����IP/PORT����, ���ӱ���Ĭ��10����
 *        [ע��] tcp����buff, ��������static����, ����������Ĵ��� [��Ҫ]
 *        [ע��] �޸Ľӿڣ���ע�ⲻҪ����޸ķ���ֵ������֤��mt_tcpsendrcv_ex����ֵƥ�� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param timeout -��ʱʱ��, ��λms
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -4 ����ʧ��, -10 ������Ч
 */
int mt_tcpsend(struct sockaddr_in* dst, void* pkg, int len, int timeout)
{
    if (!dst || !pkg || len<1) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], len[%d]", dst, pkg, len);
        return -10;
    }

    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. ��ȡTCP���ӳض���, �ҽ�֪ͨ����
    int sock = -1;
    TcpKeepConn* conn = mt_tcp_get_keep_conn(dst, sock);
    if ((conn == NULL) || (sock < 0))
    {
        MTLOG_ERROR("socket[%d] get conn failed, ret[%m]", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. ���Լ����½�����
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    // ʧ����ǿ���ͷ�����, ����ʱ����
    if (conn != NULL)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, (ret < 0));
    }

    return ret;
}

/**
 * @brief TCP������ֻ�����սӿ�
 *        [ע��] tcp����buff, ��������static����, ����������Ĵ��� [��Ҫ]
 *        [ע��] �޸Ľӿڣ���ע�ⲻҪ����޸ķ���ֵ������֤��mt_tcpsendrcv_ex����ֵƥ�� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param timeout -��ʱʱ��, ��λms
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -4 ����ʧ��, -10 ������Ч
 */
int mt_tcpsend_short(struct sockaddr_in* dst, void* pkg, int len, int timeout)
{
    // 1. ��μ��
    if (!dst || !pkg || len<1) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], len[%d]", dst, pkg, len);
        return -10;
    }

    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 2. ����TCP socket
    int sock = -1;
    sock = mt_tcp_create_sock();
    if (sock < 0)
    {
        MTLOG_ERROR("create tcp socket failed, ret: %d", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. ���Լ����½�����
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    if (sock >= 0)
        ::close(sock);

    return ret;
}


/**
 * @brief TCP�շ��ӿڣ�����ѡ���˱������ӻ��߶�����
 *        [ע��] tcp���շ���buff, ��������static����, ����������Ĵ��� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param rcv_buf -����Ӧ�����buff��ֻ�����տ�������ΪNULL
 * @param buf_size -modify-����Ӧ�����buff��С, �ɹ�����ʱ, �޸�ΪӦ������ȣ�ֻ�����գ�����ΪNULL
 * @param timeout -��ʱʱ��, ��λms
 * @param check_func -��ⱨ���Ƿ�ɹ����ﺯ��
 * @param type - ��������
 *               MT_TCP_SHORT: һ��һ�������ӣ�
 *               MT_TCP_LONG : һ��һ�ճ����ӣ�
 *               MT_TCP_LONG_SNDONLY : ֻ�����ճ����ӣ� 
 *               MT_TCP_SHORT_SNDONLY: ֻ�����ն����ӣ�
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -3 ����Ӧ��ʧ��, 
 *          -4 ����ʧ��, -5 ��ⱨ��ʧ��, -6 ���տռ䲻��, -7 ��������ر�����, -10 ������Ч
 */
int mt_tcpsendrcv_ex(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int* buf_size, int timeout, MtFuncTcpMsgLen func, MT_TCP_CONN_TYPE type)
{
	if(!dst || !pkg || len<1)
	{
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p], len[%d], buf_size[%p]type[%d]",
                    dst, pkg, rcv_buf, func, len, buf_size,type);
        return -10;		
	}

    switch (type)
    {
        // TCP�����ӵ�������
        case MT_TCP_LONG:
        {
            return mt_tcpsendrcv(dst, pkg, len, rcv_buf, *buf_size, timeout, func);
        }

        // TCP������ֻ������
        case MT_TCP_LONG_SNDONLY:
        {
            return mt_tcpsend(dst, pkg, len, timeout);
        }

        // TCP�����ӵ�������
        case MT_TCP_SHORT:
        {
            return mt_tcpsendrcv_short(dst, pkg, len, rcv_buf, *buf_size, timeout, func);
        }

        // TCP������ֻ������
        case MT_TCP_SHORT_SNDONLY:
        {
            return mt_tcpsend_short(dst, pkg, len, timeout);
        }

        default:
        {
            MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p], len[%d], buf_size[%p]type[%d]",
                        dst, pkg, rcv_buf, func, len, buf_size,type);
            return -10;
        }
    }

    return 0;
}



/**
 * @brief ������ص���������
 */
static void mt_task_process(void* arg)
{
    int rc = 0;
    IMtTask* task = (IMtTask*)arg;
    if (!task)
    {
        MTLOG_ERROR("Invalid arg, error");
        return;
    }

    rc = task->Process();
    if (rc != 0)
    { 
        MTLOG_DEBUG("task process failed(%d), log", rc);
    }

    task->SetResult(rc);

    return;
};

/**
 * @brief ��·IO�Ĵ���, ��������̹߳���
 * @param req_list - �����б�
 * @return 0 �ɹ�, <0ʧ��
 */
int mt_exec_all_task(IMtTaskList& req_list)
{
    MtFrame* mtframe    = MtFrame::Instance();
    MicroThread* thread = mtframe->GetActiveThread();
    IMtTask* task       = NULL;
    MicroThread* sub    = NULL;
    MicroThread* tmp    = NULL;
    int rc              = -1;

    MicroThread::SubThreadList list;
    TAILQ_INIT(&list);

    // ��ֹû��task������΢�߳�һֱ����ס
    if (0 == req_list.size())
    {
        MTLOG_DEBUG("no task for execult");
        return 0;
    }

    // 1. �����̶߳���
    for (IMtTaskList::iterator it = req_list.begin(); it != req_list.end(); ++it)
    {
        task = *it;
        sub = MtFrame::CreateThread(mt_task_process, task, false);
        if (NULL == sub) 
        {
            MTLOG_ERROR("create sub thread failed");
            goto EXIT_LABEL;
        }
        
        sub->SetType(MicroThread::SUB_THREAD);
        TAILQ_INSERT_TAIL(&list, sub, _sub_entry);
    }

    // 2. ����ִ������
    TAILQ_FOREACH_SAFE(sub, &list, _sub_entry, tmp)
    {
        TAILQ_REMOVE(&list, sub, _sub_entry);
        thread->AddSubThread(sub);
        mtframe->InsertRunable(sub);
    }

    // 3. �ȴ����߳�ִ�н���
    thread->Wait();
    rc = 0;
    
EXIT_LABEL:

    TAILQ_FOREACH_SAFE(sub, &list, _sub_entry, tmp)
    {
        TAILQ_REMOVE(&list, sub, _sub_entry);
        mtframe->FreeThread(sub);
    }

    return rc;

}

/**
 * @brief ���õ�ǰIMtMsg��˽�б���
 * @info  ֻ����ָ�룬�ڴ���Ҫҵ�����
 */
void mt_set_msg_private(void *data)
{
    MicroThread *msg_thread = MtFrame::Instance()->GetRootThread();
    if (msg_thread != NULL)
        msg_thread->SetPrivate(data);
}

/**
 * @brief  ��ȡ��ǰIMtMsg��˽�б���
 * @return ˽�б���ָ��
 */
void* mt_get_msg_private()
{
    MicroThread *msg_thread = MtFrame::Instance()->GetRootThread();
    if (NULL == msg_thread)
    {
        return NULL;
    }

    return msg_thread->GetPrivate();
}

/**
 * @brief  ΢�߳̿�ܳ�ʼ��
 * @info   ҵ��ʹ��spp������΢�̣߳���Ҫ���øó�ʼ������
 * @return false:��ʼ��ʧ��  true:��ʼ���ɹ�
 */
bool mt_init_frame(void)
{
    return MtFrame::Instance()->InitFrame();
}

/**
 * @brief ����΢�̶߳���ջ�ռ��С
 * @info  �Ǳ������ã�Ĭ�ϴ�СΪ128K
 */
void mt_set_stack_size(unsigned int bytes)
{
    ThreadPool::SetDefaultStackSize(bytes);
}

/**
 * @brief ΢�̰߳�����ϵͳIO���� recvfrom
 * @param fd ϵͳsocket��Ϣ
 * @param buf ������Ϣ������ָ��
 * @param len ������Ϣ����������
 * @param from ��Դ��ַ��ָ��
 * @param fromlen ��Դ��ַ�Ľṹ����
 * @param timeout ��ȴ�ʱ��, ����
 * @return >0 �ɹ����ճ���, <0 ʧ��
 */
int mt_recvfrom(int fd, void *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen, int timeout)
{
    return MtFrame::recvfrom(fd, buf, len, flags, from, fromlen, timeout);
}

/**
 * @brief ΢�̰߳�����ϵͳIO���� sendto
 * @param fd ϵͳsocket��Ϣ
 * @param msg �����͵���Ϣָ��
 * @param len �����͵���Ϣ����
 * @param to Ŀ�ĵ�ַ��ָ��
 * @param tolen Ŀ�ĵ�ַ�Ľṹ����
 * @param timeout ��ȴ�ʱ��, ����
 * @return >0 �ɹ����ͳ���, <0 ʧ��
 */
int mt_sendto(int fd, const void *msg, int len, int flags, const struct sockaddr *to, int tolen, int timeout)
{
    return MtFrame::sendto(fd, msg, len, flags, to, tolen, timeout);
}

/**
 * @brief ΢�̰߳�����ϵͳIO���� connect
 * @param fd ϵͳsocket��Ϣ
 * @param addr ָ��server��Ŀ�ĵ�ַ
 * @param addrlen ��ַ�ĳ���
 * @param timeout ��ȴ�ʱ��, ����
 * @return >0 �ɹ����ͳ���, <0 ʧ��
 */
int mt_connect(int fd, const struct sockaddr *addr, int addrlen, int timeout)
{
    return MtFrame::connect(fd, addr, addrlen, timeout);
}

/**
 * @brief ΢�̰߳�����ϵͳIO���� accept
 * @param fd �����׽���
 * @param addr �ͻ��˵�ַ
 * @param addrlen ��ַ�ĳ���
 * @param timeout ��ȴ�ʱ��, ����
 * @return >=0 accept��socket������, <0 ʧ��
 */
int mt_accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int timeout)
{
    return MtFrame::accept(fd, addr, addrlen, timeout);
}


/**
 * @brief ΢�̰߳�����ϵͳIO���� read
 * @param fd ϵͳsocket��Ϣ
 * @param buf ������Ϣ������ָ��
 * @param nbyte ������Ϣ����������
 * @param timeout ��ȴ�ʱ��, ����
 * @return >0 �ɹ����ճ���, <0 ʧ��
 */
ssize_t mt_read(int fd, void *buf, size_t nbyte, int timeout)
{
    return MtFrame::read(fd, buf, nbyte, timeout);
}

/**
 * @brief ΢�̰߳�����ϵͳIO���� write
 * @param fd ϵͳsocket��Ϣ
 * @param buf �����͵���Ϣָ��
 * @param nbyte �����͵���Ϣ����
 * @param timeout ��ȴ�ʱ��, ����
 * @return >0 �ɹ����ͳ���, <0 ʧ��
 */
ssize_t mt_write(int fd, const void *buf, size_t nbyte, int timeout)
{
    return MtFrame::write(fd, buf, nbyte, timeout);
}

/**
 * @brief ΢�̰߳�����ϵͳIO���� recv
 * @param fd ϵͳsocket��Ϣ
 * @param buf ������Ϣ������ָ��
 * @param len ������Ϣ����������
 * @param timeout ��ȴ�ʱ��, ����
 * @return >0 �ɹ����ճ���, <0 ʧ��
 */
ssize_t mt_recv(int fd, void *buf, int len, int flags, int timeout)
{
    return MtFrame::recv(fd, buf, len, flags, timeout);
}

/**
 * @brief ΢�̰߳�����ϵͳIO���� send
 * @param fd ϵͳsocket��Ϣ
 * @param buf �����͵���Ϣָ��
 * @param nbyte �����͵���Ϣ����
 * @param timeout ��ȴ�ʱ��, ����
 * @return >0 �ɹ����ͳ���, <0 ʧ��
 */
ssize_t mt_send(int fd, const void *buf, size_t nbyte, int flags, int timeout)
{
    return MtFrame::send(fd, buf, nbyte, flags, timeout);
}

/**
 * @brief ΢�߳�����sleep�ӿ�, ��λms
 */
void mt_sleep(int ms)
{
    MtFrame::sleep(ms); 
}

/**
 * @brief ΢�̻߳�ȡϵͳʱ�䣬��λms
 */
unsigned long long mt_time_ms(void)
{
    return MtFrame::Instance()->GetLastClock();
}

/**
 * @brief ΢�̵߳ȴ�epoll�¼��İ�������
 */
int mt_wait_events(int fd, int events, int timeout)
{
    return MtFrame::Instance()->WaitEvents(fd, events, timeout);
}

void* mt_start_thread(void* entry, void* args)
{
    return MtFrame::Instance()->CreateThread((ThreadStart)entry, args, true);
}

#define BUF_ALIGNMENT_SIZE 4096
#define BUF_ALIGN_SIZE(x) (((x)+BUF_ALIGNMENT_SIZE-1)&~(BUF_ALIGNMENT_SIZE-1))
#define BUF_DEFAULT_SIZE 4096

class ScopedBuf
{
public:
	ScopedBuf(void*& buf_keeper, bool keep)
	:buf_keeper_(buf_keeper),buf_(0),len_(0),len_watermark_(0),keep_(keep)
	{}

	int Alloc(int len)
	{
		if(len<len_)
		{
			return -1; // �ռ��СԽ��
		}

        if(len==0)
        {
            len = BUF_ALIGNMENT_SIZE;
        }
		if(len_==len)
		{
			return 0;
		}
	
		len_ = BUF_ALIGN_SIZE(len);
		if(len_==0)
		{
			len_ = BUF_DEFAULT_SIZE;
		}	
		len_watermark_ = len_-BUF_ALIGNMENT_SIZE;
		char* tmp = (char*)realloc(buf_, len_);
		if(tmp==NULL)
		{
	        return -2; // �����ڴ�ʧ��		
		}

		buf_ = tmp;
		return 0;
	}

	void reset()
	{
		if(keep_)
		{
			buf_keeper_ = (void*)buf_;
			buf_ = NULL;
		}
	}
		
	~ScopedBuf()
	{
		if(buf_!=NULL)
		{
			free(buf_);
			buf_ = NULL;
		}
	}

public:
	void* &buf_keeper_;
	char* buf_;
	int   len_;
    int   len_watermark_;
	bool  keep_;
	
};

/**
 * @brief TCPѭ������, ֱ������OK��ʱ
 *       [ע��] �����߲�Ҫ�����޸ĺ�������ֵ����֤��Ҫ��mt_tcpsendrcv�ȵ��ýӿڳ�ͻ [��Ҫ]
 */
static int mt_tcp_check_recv(int sock, void*& rcv_buf, int &len, int flags, 
                     int timeout, MtFuncTcpMsgChecker check_func, void* msg_ctx, bool keep_rcv_buf)
{
    int recv_len = 0;
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();

	int rc = 0;
	int ret = 0;
    int pkg_len = 0;
    bool msg_len_detected = false;

	ScopedBuf sbuf(rcv_buf, keep_rcv_buf);
	ret = sbuf.Alloc(len);

	if(ret!=0)
	{
        MTLOG_ERROR("tcp socket[%d] recv failed ret[%d], alloc rcv buf failed, [%m]", sock, ret);
		return -11;
	}
	    
    do
    {
        utime64_t cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
        if (cost_time > (utime64_t)timeout)
        {
            errno = ETIME;            
            MTLOG_ERROR("tcp socket[%d] recv not ok, timeout", sock);
            return -3;
        }

        rc = MtFrame::recv(sock, (sbuf.buf_ + recv_len), (sbuf.len_ - recv_len), 0, (timeout - (int)cost_time));
        if (rc < 0)
        {
            MTLOG_ERROR("tcp socket[%d] recv failed ret[%d][%m]", sock, rc);
            return -3;
        }
		else if (rc == 0) // Զ�˹ر�
        {

			if(recv_len==0) // δ�ذ���ֱ�ӷ���Զ�˹ر�
			{
	            MTLOG_ERROR("tcp socket[%d] remote close", sock);
	            return -7;				
			}
        	
	        /* ��鱨�������� */
		    rc = check_func(sbuf.buf_, recv_len, true, msg_ctx, msg_len_detected);
            
			if(rc!=recv_len) // ҵ�����Զ�˹رգ�Ӧ�÷������������ȣ�����<=0,��ʾ����������
			{
	            MTLOG_ERROR("tcp socket[%d] remote close", sock);
    	        return -7;				
			}
        	len = recv_len; 
			break;
        }
        recv_len += rc;

        /* ��鱨�������� */
        if((!msg_len_detected)||recv_len==pkg_len)
        {
            rc = check_func(sbuf.buf_, recv_len, false, msg_ctx,msg_len_detected);
            if(msg_len_detected)
            {
                pkg_len = rc;
            }
        }
        else
        {
            rc = pkg_len;
        }
        
        if (rc < 0)
        {
            MTLOG_ERROR("tcp socket[%d] user check pkg error[%d]", sock, rc);
            return -5;
        }
        else if (rc == 0) // ����δ������,�Ҳ�ȷ����С
        { 
        	if(sbuf.len_ > recv_len)
        	{
        		continue;
        	}
            // û�ռ��ٽ�����, 2����С��չbuf
			ret = sbuf.Alloc(sbuf.len_<<1);

			if(ret!=0)
			{
		        MTLOG_ERROR("tcp socket[%d] recv failed ret[%d], alloc rcv buf failed, [%m]", sock, ret);
				return -11;
			}	
        }
        else    // �ɹ����㱨�ĳ���
        { 
            if (rc > recv_len) // ���Ļ�δ��ȫ
            {
            	if(sbuf.len_ > recv_len) // recv buf���пռ�.��δ����ˮλ
            	{
            		continue;
            	}
            	
	            // û�ռ��ٽ�����, ����ҵ��ָʾ��С��չ�ڴ�
				ret = sbuf.Alloc(rc);

				if(ret!=0)
				{
			        MTLOG_ERROR("tcp socket[%d] recv failed ret[%d], alloc rcv buf failed, [%m]", sock, ret);
					return -11;
				}	
            }
            else if(rc==recv_len) // �հ�����
            {
                len = rc;
                break;
            }
            else // ΢�߳����ģʽ�£�������ճ��
            {
	            MTLOG_ERROR("tcp socket[%d] user check pkg error, pkg len < recv_len", sock);
	            return -5;
            }
        }
    } while (true);

	sbuf.reset();

    return 0;
}



/**
 * @brief TCP��������ӳصķ�ʽ����IP/PORT����, ���ӱ���Ĭ��10����
 *        [ע��] tcp���շ���buff, ��������static����, ����������Ĵ��� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param rcv_buf -���������������ο����� keep_rcv_buf��
 * @param recv_pkg_size -����Ӧ�����buff�ĳ�ʼ��С����������0ʱ�����ر���ʵ�ʳ��ȡ�
 * @param timeout -��ʱʱ��, ��λms
 * @param check_func -��ⱨ���Ƿ�ɹ����ﺯ��
 * @param msg_ctx -�������ĵ������ı�����
 * @param keep_rcv_buf -true,��ܽ�����rcv_buf��ֵ������������ҵ�����ͷŸ�buf���������ڵ��ý���ǰ�ͷŸ�buf����ע�⡿
 *               ҵ����Ҫ�Լ���msg_ctx�б��������Ϣ�������ͨ��malloc�����ڴ桿     
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -3 ����Ӧ��ʧ��, 
 *          -4 ����ʧ��, -5 ��ⱨ��ʧ��, -6 ���տռ䲻��, -7 ��������ر�����, -10 ������Ч
 */
int mt_tcpsendrcv(struct sockaddr_in* dst, void* pkg, int len, void*& rcv_buf, int& recv_pkg_size, 
                     int timeout, MtFuncTcpMsgChecker check_func, void* msg_ctx, bool keep_rcv_buf)
{
	if(!dst || !pkg || len<1)
	{
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], len[%d], fun[%p]",
                    dst, pkg, len, check_func);
        return -10;		
	}


    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. ��ȡTCP���ӳض���, �ҽ�֪ͨ����
    int sock = -1;
    TcpKeepConn* conn = mt_tcp_get_keep_conn(dst, sock);
    if ((conn == NULL) || (sock < 0))
    {
        MTLOG_ERROR("socket[%d] get conn failed, ret[%m]", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. ���Լ����½�����
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    // 4. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;

    rc = mt_tcp_check_recv(sock, rcv_buf, recv_pkg_size, 0, time_left, check_func, msg_ctx, keep_rcv_buf);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] rcv failed, ret[%d][%m]", sock, rc);
        ret = rc;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    // ʧ����ǿ���ͷ�����, ����ʱ����
    if (conn != NULL)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, (ret < 0));
    }

    return ret;
}


/**
 * @brief TCP�������շ�����
 *        [ע��] tcp���շ���buff, ��������static����, ����������Ĵ��� [��Ҫ]
 *        [ע��] �޸Ľӿڣ���ע�ⲻҪ����޸ķ���ֵ������֤��mt_tcpsendrcv_ex����ֵƥ�� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param rcv_buf -���������������ο����� keep_rcv_buf��
 * @param recv_pkg_size -����Ӧ�����buff�ĳ�ʼ��С����������0ʱ�����ر���ʵ�ʳ��ȡ�
 * @param timeout -��ʱʱ��, ��λms
 * @param check_func -��ⱨ���Ƿ�ɹ����ﺯ��
 * @param msg_ctx -�������ĵ������ı�����
 * @param keep_rcv_buf -true,��ܽ�����rcv_buf��ֵ������������ҵ�����ͷŸ�buf���������ڵ��ý���ǰ�ͷŸ�buf����ע�⡿
 *               ҵ����Ҫ�Լ���msg_ctx�б��������Ϣ�������ͨ��malloc�����ڴ桿     
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -3 ����Ӧ��ʧ��, 
 *          -4 ����ʧ��, -5 ��ⱨ��ʧ��, -6 ���տռ䲻��, -7 ��������ر�����, -10 ������Ч
 */
int mt_tcpsendrcv_short(struct sockaddr_in* dst, void* pkg, int len, void*& rcv_buf, int& recv_pkg_size, 
                     int timeout, MtFuncTcpMsgChecker check_func, void* msg_ctx, bool keep_rcv_buf)
{
    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. �������
	if(!dst || !pkg || len<1)
	{
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], len[%d], fun[%p]",
                    dst, pkg, len, check_func);
        return -10;		
	}

    // 2. ����TCP socket
    int sock;
    sock = mt_tcp_create_sock();
    if (sock < 0)
    {
        MTLOG_ERROR("create tcp socket failed, ret: %d", sock);
        return -1;
    }

    // 3. ���Լ����½�����
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 4. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    // 5. �������ݴ���
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = mt_tcp_check_recv(sock, rcv_buf, recv_pkg_size, 0, time_left, check_func, msg_ctx, keep_rcv_buf);

    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] rcv failed, ret[%d][%m]", sock, rc);
        ret = rc;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:
    if (sock >= 0)
        ::close(sock);

    return ret;
}



/**
 * @brief TCP�շ��ӿڣ�����ѡ���˱������ӻ��߶�����
 *        [ע��] tcp���շ���buff, ��������static����, ����������Ĵ��� [��Ҫ]
 * @param dst -�����͵�Ŀ�ĵ�ַ
 * @param pkg -�������װ�İ���
 * @param len -�������װ�İ��峤��
 * @param rcv_buf -���������������ο����� keep_rcv_buf��
 * @param recv_pkg_size -����Ӧ�����buff�ĳ�ʼ��С����������0ʱ�����ر���ʵ�ʳ��ȡ�
 * @param timeout -��ʱʱ��, ��λms
 * @param check_func -��ⱨ���Ƿ�ɹ����ﺯ��
 * @param msg_ctx -�������ĵ������ı�����
 *
 * @param type - ��������
 *               MT_TCP_SHORT: һ��һ�������ӣ�
 *               MT_TCP_LONG : һ��һ�ճ����ӣ�
 *               MT_TCP_LONG_SNDONLY : ֻ�����ճ����ӣ� 
 *               MT_TCP_SHORT_SNDONLY: ֻ�����ն����ӣ�
 * @param keep_rcv_buf -true,��ܽ�����rcv_buf��ֵ������������ҵ�����ͷŸ�buf���������ڵ��ý���ǰ�ͷŸ�buf����ע�⡿
 *               ҵ����Ҫ�Լ���msg_ctx�б��������Ϣ�������ͨ��malloc�����ڴ桿     
 * @return  0 �ɹ�, -1 ��socketʧ��, -2 ��������ʧ��, -3 ����Ӧ��ʧ��, 
 *          -4 ����ʧ��, -5 ��ⱨ��ʧ��, -6 ���տռ䲻��, -7 ��������ر�����, -10 ������Ч
 */
int mt_tcpsendrcv_ex(struct sockaddr_in* dst, void* pkg, int len, void*& rcv_buf, int& rcv_pkg_size, 
                     int timeout, MtFuncTcpMsgChecker check_func, void* msg_ctx, 
                     MT_TCP_CONN_TYPE type, bool keep_rcv_buf)
{
	if(!dst || !pkg || len<1)
	{
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], len[%d], fun[%p], msg_ctx[%p], type[%d]",
                    dst, pkg, len, check_func, msg_ctx, type);
        return -10;		
	}

    switch (type)
    {
        // TCP�����ӵ�������
        case MT_TCP_LONG:
        {
            return mt_tcpsendrcv(dst, pkg, len, rcv_buf, rcv_pkg_size, timeout, check_func, msg_ctx, keep_rcv_buf);
        }

        // TCP������ֻ������
        case MT_TCP_LONG_SNDONLY:
        {
            return mt_tcpsend(dst, pkg, len, timeout);
        }

        // TCP�����ӵ�������
        case MT_TCP_SHORT:
        {
            return mt_tcpsendrcv_short(dst, pkg, len, rcv_buf, rcv_pkg_size, timeout, check_func, msg_ctx, keep_rcv_buf);
        }

        // TCP������ֻ������
        case MT_TCP_SHORT_SNDONLY:
        {
            return mt_tcpsend_short(dst, pkg, len, timeout);
        }

        default:
        {
	        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], len[%d], fun[%p], msg_ctx[%p], type[%d]",
                    dst, pkg, len, check_func, msg_ctx, type);
            return -10;
        }
    }

    return 0;
}



}


