/**
 * @brief nest channel ͨ������ 
 */

#ifndef __NEST_CHANNEL_H_
#define __NEST_CHANNEL_H_

#include <stdint.h>
#include <time.h>
#include <list>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "global.h"
#include "tcache.h"
#include "tcommu.h"
#include "poller.h"
#include "nest_address.h"
#include "weight_schedule.h"


using std::vector;
using std::map;

using namespace tbase::tcommu;
using namespace tbase::tcommu::tsockcommu;


namespace nest
{
    class TNestChannel;     
    class TNestAddress;    

    /**
     * @brief ��Ϣ�ṹ�ַ�����
     */
    #define NEST_BLOB_TYPE_DGRAM            1   // ���ݱ�����    
    #define NEST_BLOB_TYPE_STREAM           2   // ����������
    struct TNestBlob
    {
        uint32_t           _type;               // TCP/UDP ����
        void*              _buff;               // ��Ϣbuff, ��ʱbuffָ��
        uint32_t           _len;                // ��Ϣ����
        uint32_t           _recv_fd;            // ������Ϣ��fd
        uint64_t           _time_sec;           // ʱ����
        uint64_t           _time_usec;          // ΢��ʱ��
        TNestAddress       _remote_addr;        // �Զ˵�ַ
        TNestAddress       _local_addr;         // ���ε�ַ
    };
    

    /**
     * @brief ͨ�������ַ�����, ����������
     */
    class TNestDispatch
    {
    public:
        
        TNestDispatch() {};
        virtual ~TNestDispatch() {};

        // ʵʱ�����հ�, һ�δ�����, �ɲ�ʵ��recvcache
        // ����==0��������δ��ȫ, >0 ��ʾ�ɹ�������, <0 ʧ��
        virtual int32_t DoRecv(TNestBlob* blob)   { return 0;};
        
        // ��������հ�, ��һ�δ�����, Ҳ��ÿ�ζ�������
        // ����==0��������δ��ȫ, >0 ��ʾ�ɹ�������, <0 ʧ��
        virtual int32_t DoRecvCache(TNestBlob* blob, TNestChannel* channel, bool block)   { return 0;};

        // ������������ӽӿ�, ����0�ɹ�, ����ʧ��
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr) { return 0;};

        // �������ӳɹ���֪ͨ
        virtual int32_t DoConnected(TNestChannel* channel) { return 0;};
 
        // �쳣����ӿ�, ��ֱ�ӻ���channelָ��
        virtual int32_t DoError(TNestChannel* channel) { return 0;};
    };    
    

    /**
     * @brief ������ channel����״̬����
     */
    enum TNestConnState {
        UNCONNECTED = 0,
        CONNECTING  = 1,
        CONNECTED   = 2,
    };

    /**
     * @brief ������ channel class ����
     */
    class TNestChannel : public CPollerObject, public IDispatchAble
    {
    public:

        /**
         * @brief ��������������
         */
        TNestChannel();
        virtual ~TNestChannel();

        /**
         * @brief CPollerObject �����¼�������
         */
        virtual int32_t InputNotify();
        virtual int32_t OutputNotify();
        virtual int32_t HangupNotify();

        /**
         * @brief ��Ϊ�ͻ���, ������������, ��װ����ϸ��
         * @return >0 ���ӳɹ�, <0 ����ʧ��, =0 ���ӽ�����
         */
        int32_t Connect();

        /**
         * @brief ������ӽ���, ��ִ�����ӹ���, �����¼�����
         * @return < 0 ʧ��, > 0 ������, =0 ������
         */
        int32_t DoConnect();                

        /**
         * @brief ��TCPͨ����ȡ����, ��ʱ�������ݽ����������
         * @return >0 �ɹ�,���ؽ��յ��ֽ�; <0 ����ʧ��, =0 �Զ˹ر�
         */
        int32_t Recv();

        /**
         * @brief ����ÿ��ֻ����һ�����ĵ�����, �ṩ�����ȡ�ӿ�
         * @param  block �Ƿ�����, true��һ��ȡ��, false ��ÿ��ȡһ��
         * @return ����ı�����Ŀ, С��0ʧ��
         */
        int32_t RecvCache(bool block);

        /**
         * @brief ��TCPͨ��д������, ����ʣ����ֶ�, ����д������
         * @return >=0 �ɹ�,���ط��͵��ֽ�; <0 ʧ��
         */
        int32_t Send(const char* data, uint32_t data_len);
        int32_t SendEx(const char* ctrl, uint32_t ctrl_len, const char* data, uint32_t data_len);

        /**
         * @brief ��������д�����������ݲ���
         * @return >=0 �ɹ�,���ط��͵��ֽ�; <0 ʧ��
         */
        int32_t SendCache();

        /**
         * @brief ȷ���Ƿ�������
         */
        bool CheckConnected() {
            return (_conn_state == CONNECTED);
        };

        void SetConnState(TNestConnState state) {
            _conn_state = state;    
        };

        /**
         * @brief ����Զ�˵�ַ
         */
        void SetRemoteAddr(struct sockaddr_in* addr) {
            _remote_addr.SetAddress(addr);
        };
        
        void SetRemoteAddr(TNestAddrType type, struct sockaddr* addr, socklen_t len) {
            _remote_addr.SetSockAddress(type, addr, len);
        }; 

        void SetRemoteAddr(TNestAddress* addr) {
            _remote_addr = *addr;
        };

        /**
         * @brief ��ȡԶ��IP�ַ���
         */
        char* GetRemoteIP() {
            return _remote_addr.IPString(NULL, 0);
        };

        TNestAddress& GetRemoteAddr() {
            return _remote_addr;
        };
        
        /**
         * @brief ����֪ͨ�ַ�����
         */
        void SetDispatch(TNestDispatch* disp) {
            _dispatch = disp;
        };

        /**
         * @brief �ڲ��������
         */
        ConnCache* GetRwCache() {
            return _cc;
        }; 
        
        CPollerUnit* GetPollerUnit() {
            return ownerUnit;
        };

        /**
         * @brief ����IO����, socket�������
         */
        void SetSockFd(int32_t fd) {
            netfd = fd;
        };        
        int32_t GetSockFd() {
            return netfd;
        };

        /**
         * @brief cookie����
         */
        void SetCookie(void* arg) {
            _cookie = arg;
        };        
        void* GetCookie() {
            return _cookie;
        };
        

        /**
         * @brief ����socket���, Ĭ��Ϊ��ʽ
         */
        int32_t CreateSock(int32_t domain, int32_t type = SOCK_STREAM);

    protected:
    
        ConnCache*              _cc;           // send recv cache mng
        TNestConnState          _conn_state;   // connect state
        TNestAddress            _remote_addr;  // Զ�˵�ַ
        TNestDispatch*          _dispatch;     // �ַ�֪ͨ
        void*                   _cookie;       // ˽��ָ����Ϣ
        
    };
    
    typedef std::map<uint32_t, TNestChannel*>  TNestChannelMap;
    typedef std::vector<TNestChannel*>         TNestChannelList;

    /**
     * @brief ������ listen class ����
     */
    class TNestListen : public CPollerObject
    {
    public:

        /**
         * @brief ��������������
         */
        TNestListen() {
            _dispatch = NULL;    
			_tcp = true;
        };
        virtual ~TNestListen() {};

        /**
         * @brief CPollerObject �����¼�������
         */
        virtual int32_t InputNotify();
        virtual int32_t OutputNotify();
        virtual int32_t HangupNotify();
        
        /**
         * @brief ��ʼ������ֹ����
         */
        virtual int32_t Init();
        virtual void Term() { return;};

        /**
         * @brief ��ʼ��listen socket
         */
        virtual int32_t CreateSock();  

        /**
         * @brief ���õ�ַ������Ϣ
         */
        void SetListenAddr(struct sockaddr_in* addr) {
            _listen_addr.SetAddress(addr);
        };
        void SetListenAddr(struct sockaddr_un* addr, bool abstract = false) {
            _listen_addr.SetAddress(addr, abstract);
        };
        void SetListenAddr(TNestAddress* addr) {
            _listen_addr = *addr;
        };   
        TNestAddress& GetListenAddr() {
            return _listen_addr;
        };

        /**
         * @brief ����֪ͨ�ַ�����
         */
        void SetDispatch(TNestDispatch* disp) {
            _dispatch = disp;
        };  

        /**
         * @brief ����IO����, socket�������
         */
        int32_t GetSockFd() {
            return netfd;
        };

		bool IsTcp()
		{ 
			return _tcp;
		}
    protected:
    
        TNestAddress     _listen_addr;        // listen address
        TNestDispatch*   _dispatch;           // �ַ�֪ͨ 
        bool             _tcp;                   
    };


    /**
     * @brief udp listen class ����
     */
    class TNestUdpListen : public TNestListen
    {
    public:

        /**
         * @brief ��������������
         */
        TNestUdpListen();
        virtual ~TNestUdpListen();
    
        /**
         * @brief CPollerObject �����¼�������
         */
        virtual int32_t InputNotify();

        /**
         * @brief ��ʼ��listen socket
         */
        virtual int32_t CreateSock();  

    protected:

        char*               _msg_buff;              // ��ʱ���ջ���
        uint32_t            _buff_size;             // �����С

    };
   

    /** 
     * @brief ����Զ�̽�����������ȫ�ֹ���
     */
    class  TNestNetComm
    {
    public:

        /** 
         * @brief ��������
         */
        ~TNestNetComm();

        /** 
         * @brief �����������ʽӿ�
         */
        static TNestNetComm* Instance();

        /** 
         * @brief ȫ�ֵ����ٽӿ�
         */
        static void Destroy();

        /** 
         * @brief �ڴ����epoll������ʽӿ�
         */
        CMemPool* GetMemPool() {
            return _mem_pool;
        };
        
        CPollerUnit* GetPollUnit() {
            return _poll_unit;
        };
        
        void SetPollUnit(CPollerUnit* poll) {
            _poll_unit = poll;
        };

        /**
         * @brief ����������һ��poll unit����
         */
        CPollerUnit* CreatePollUnit() ;   
        
        void DestroyPollUnit(CPollerUnit* unit);

        /** 
         * @brief ȫ�ֵ�epoll unitִ�нӿ�
         */
        void RunPollUnit(uint32_t wait_time);


    private:

        /** 
         * @brief ���캯��
         */
        TNestNetComm();
    
        static TNestNetComm *_instance;         // �������� 
        
        CPollerUnit*         _poll_unit;        // epoll�������
        CMemPool*            _mem_pool;         // �ڴ�ض���
    };

}

#endif

