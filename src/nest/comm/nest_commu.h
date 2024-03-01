/**
 * @brief net commu for proxy worker
 */

#ifndef _TNEST_COMMU_H_
#define _TNEST_COMMU_H_

#include <stdint.h>
#include <time.h>
#include "nest_stat.h"
#include "nest_channel.h"
#include "nest_context.h"

#define NEST_MAGIC_NUM          0x7473656e      // У����� magic num 'nest'
#define NEST_PROXY_BASE_PORT    61001           // ��׼�����˿�
#define NEST_PROXY_PRIMER       3989            // ��׼��ӳ������
#define NEST_HEARBEAT_TIMES     6000            // 6K������ Լ1���� ����һ��

namespace nest
{

    /**
     * @brief �ڲ���Ϣ����ϸ�ֶ���
     */
    enum 
    {
        NEST_MSG_TYPE_USER_DATA       = 0,      // �û�������Ϣ
        NEST_MSG_TYPE_REGIST          = 1,      // ע����Ϣ, ����ʱ��һ��
        NEST_MSG_TYPE_HEARTBEAT       = 2,      // ������Ϣ, ����ʶ��Ͽ�
        NEST_MSG_TYPE_UNREGIST        = 3,      // ȡ��ע��, �����˳�
    };


    /**
     * @brief �ڲ����������ýṹ
     */
    struct TNetCoummuConf
    {
        int32_t             route_id;   // ·��id    
        TNestAddress        address;    // ������ַ
    };
    
    /**
     * @brief ͨ�������ַ�����, ����������
     */
    class TCommuDisp : public TNestDispatch, public CTCommu
    {
    public:

        ///< ����������
        virtual ~TCommuDisp() {};

        ///< �̳�Commu����Ľӿں���
        virtual int init(const void* config) { return 0;};
        virtual int poll(bool block = false) { return 0;};
        virtual int sendto(unsigned flow, void* arg1, void* arg2){ return 0;};
        virtual int ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2){ return 0;};
        virtual int clear() { return 0;};
        virtual void fini() { return;};

        ///< ������Ϣͷ���ֽ���ת������
        static void ntoh_msg_head(TNetCommuHead* head_info);
        static void hton_msg_head(TNetCommuHead* head_info);

        ///< CRC���ɼ���麯��
        static bool check_crc(TNetCommuHead* head_info);
        static uint32_t generate_crc(TNetCommuHead* head_info);

        ///< Ĭ������, ��ʼʱ��Ϊ0, ������ʱ����Ӧ��ͳ��
        void default_msg_head(TNetCommuHead* head_info, uint32_t flow, uint32_t group_id);

        /** 
         * @brief �����Ϣ�ĸ�ʽ, ���ر��ĳ���, ͷ����
         * @param blob ��Ϣ��
         * @param head_len  ͷ����, ����>0ʱ��Ч
         * @return ��Ϣ����, 0 -����������; >0 ��Ϣ����; <0��Ч����
         */
        int32_t CheckMsg(void* pos, uint32_t left, TNetCommuHead& head_info);

        /**
         * @brief  ����ԭ�еĴ�������, �Ƚ���, ����
         */
        virtual int32_t DoRecvCache(TNestBlob* blob, TNestChannel* channel, bool block);

        /**
         * @brief ҵ����Ϣ���պ�����, ��ӿں���
         */
        virtual void DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info){};

        /**
         * @brief ҵ����Ϣ������, ��ӿں���
         */
        virtual void HandleService(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body);

        /**
         * @brief ������Ϣ������, ��ӿں���
         */
        virtual void HandleControl(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body) = 0;
        
    };   
    

    /** 
     * @brief �������������ķַ�������
     */
    class TCommuSvrDisp : public TCommuDisp
    {
    public:

        ///< ����������
        virtual ~TCommuSvrDisp();

        /**
         * @brief �̳���disp�Ľӿں���ʵ��
         */
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr);
        virtual int32_t DoError(TNestChannel* channel);

        /**
         * @brief ҵ����Ϣ���պ�����, ��ӿں���
         */
        virtual void DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info);

        /**
         * @brief ������Ϣ������, ��ӿں���
         */
        virtual void HandleControl(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body);
        
        /**
         * @brief ���������ӵ�ͨ����Ϣ
         */
        void AddChannel(TNestChannel* channel);

        /**
         * @brief �������ӵ�ͨ����Ϣ
         */
        void DelChannel(TNestChannel* channel);
        
        /**
         * @brief ���������ӵ�ͨ����Ϣ
         */
        TNestChannel* FindChannel(uint32_t fd);

        /**
         * @brief ע�����ӵ�ͨ����Ϣ
         */
        int32_t RegistChannel(TNestChannel* channel, uint32_t group);

        /**
         * @brief �����������, δ�����ע�����
         */
        uint32_t CheckUnregisted();

        /**
         * @brief ����ͨ�õ�ַ����, �ɹ�����0, ʧ��С��0
         */
        int32_t StartListen(TNestAddress& address);

        /**
         * @brief ������������, ���������˿�
         */
        uint32_t StartListen(uint32_t sid, uint32_t pno, uint32_t addr);

    protected:
    
        TNestListen            _listen_obj;     // listen obj
        TNestChannelMap        _channel_map;    // ȫ�ֵ����ӹ���map
        TNestChannelMap        _un_rigsted;     // ��ʱmap��¼δע�������
    };


    /** 
     * @brief ����Զ�̽����������ķ�������
     */
    class TNetCommuServer : public CTCommu, public CWeightSchedule, public CDynamicCtrl
    {
    public:
    
        /**
         * @brief ��������������
         */
        TNetCommuServer(uint32_t group) : _groupid(group) {};
        virtual ~TNetCommuServer(){};
    
        /**
         * @brief �̳���CTCommu���麯��
         */
        int32_t init(const void* config);
        int32_t clear(){return 0;};
        int32_t poll(bool block = false);
        int32_t sendto(uint32_t flow, void* arg1, void* arg2);
        int32_t ctrl(uint32_t flow, ctrl_type type, void* arg1, void* arg2){return 0;};
        void fini() {};
    
        /**
         * @brief ���Ӷ���channel������
         */
        void AddChannel(TNestChannel* channel);
        void DelChannel(TNestChannel* channel);

        /**
         * @brief ��ѭ�㷨����
         */
        TNestChannel* FindNextChannel();

        /**
         * @brief ��ȡgroupid��Ϣ
         */
        uint32_t GetGroupId() {
            return _groupid;
        };
        

    protected:

        uint32_t                 _groupid;           // ����id
        TNestChannelList         _channel_list;      // channel list
    };
    
    typedef std::map<int32_t, TNetCommuServer*> TNetCommuSvrMap;


    /** 
     * @brief �ͻ��������ķַ�������
     */
    class TCommuCltDisp : public TCommuDisp
    {
    
    public:
    
        ///< ����������
        virtual ~TCommuCltDisp() {};

        /**
         * @brief �̳���disp�Ľӿں���ʵ��
         */
        virtual int32_t DoError(TNestChannel* channel);

        /**
         * @brief ���������ӵ�֪ͨ�ӿ�
         */
        virtual int32_t DoConnected(TNestChannel* channel);

        /**
         * @brief ҵ����Ϣ���պ�����, ��ӿں���
         */
        virtual void DoRecvFinished(TNestBlob* blob, TNestChannel* channel, TNetCommuHead& head_info);
        
        /**
         * @brief ҵ����Ϣ������, ��ӿں���
         */
        virtual void HandleService(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body);
        
        /**
         * @brief ������Ϣ������, ��ӿں���
         */
        virtual void HandleControl(TNestBlob* blob, TNetCommuHead& head_info, blob_type& body) {};

        /**
         * @brief �洢��Ϣ��Ϣ������
         */
        static bool SaveMsgCtx(TNetCommuHead& head_info);

        /**
         * @brief ��ȡ��Ϣ��Ϣ������
         */
        static bool RestoreMsgCtx(uint32_t flow, TNetCommuHead& head_info, uint64_t& save_time);

    };


    /** 
     * @brief ����Զ�̽����������Ŀͻ���
     */
    class TNetCommuClient : public CTCommu
    {
    public:
    
        /**
         * @brief ��������������
         */
        TNetCommuClient();
        virtual ~TNetCommuClient();
    
        /**
         * @brief �̳���CTCommu���麯��
         */
        int32_t init(const void* config);
        int32_t clear() {return 0;};
        int32_t poll(bool block = false);
        int32_t sendto(uint32_t flow, void* arg1, void* arg2);
        int32_t ctrl(uint32_t flow, ctrl_type type, void* arg1, void* arg2){return 0;};
        void fini() {};

        /**
         * @brief ���ӱ������
         */
        int32_t CheckConnect();
        int32_t CreateSock();

        /**
         * @brief ��ʼ������
         */
        TNestChannel* InitChannel();

        /**
         * @brief ��������
         */
        bool RestartChannel();

        /**
         * @brief ͨ������Ŀͻ������������߼�
         */
        int32_t Heartbeat();
        void CheckHeartbeat();
        
        /**
         * @brief ����ID����
         */
        uint32_t GetGroupId() {
            return _net_conf.route_id;
        };

    protected:

        TNestChannel*             _conn_obj;      // n:1 ���Ӷ���
        TNetCoummuConf            _net_conf;      // ������Ϣ

    };


    /** 
     * @brief ����Զ�̽�����������ȫ�ֹ���
     */
    class  TNetCommuMng
    {
    public:

        /** 
         * @brief ��������
         */
        ~TNetCommuMng();

        /** 
         * @brief �����������ʽӿ�
         */
        static TNetCommuMng* Instance();

        /** 
         * @brief ȫ�ֵ����ٽӿ�
         */
        static void Destroy();

        /** 
         * @brief �ڲ�ͨѶ�ӿڲ�ѯ����ʽӿ�
         */
        TNetCommuServer* GetCommuServer(int32_t route_id);
        
        TNetCommuClient* GetCommuClient();

        /** 
         * @brief �ڲ�ͨѶ�ӿڹ���ע��ӿ�
         */
        void RegistClient(TNetCommuClient* client);
        
        void RegistServer(int32_t route, TNetCommuServer* server);

        /**
         * @brief ��ȡ�ַ�֪ͨ�ӿ�
         */
        TCommuSvrDisp& GetCommuSvrDisp() {
            return _svr_disp;
        };

        TCommuCltDisp& GetCommuCltDisp() {
            return _clt_disp;
        };

        /** 
         * @brief �ڲ�����ͨ��������ӿ�
         */
        void CheckChannel();

        /** 
         * @brief �ڲ���Ϣ�����Ķ��м��
         */
        void CheckMsgCtx(uint64_t now, uint32_t wait);

        /**
         * @brief ����֪ͨ����, ����ִ֪ͨ��
         */
        void SetNotifyObj(void* obj) {
            _notify_obj = obj;
        };

        /**
         * @brief ���epoll����֪ͨ����
         */
        void AddNotifyFd(int32_t fd);
        void DelNotifyFd(int32_t fd);

        /**
         * @brief ��ȡͳ�ƵĽӿڶ���
         */
        CMsgStat* GetMsgStat() {
            return &_stat_obj;
        }; 

        /**
         * @brief ��ȡ��Ϣ�����Ĺ���Ľӿڶ���
         */
        CMsgCtxMng& GetMsgCtxMng() {
            return _msg_ctx;
        }; 

        /**
         * @brief ��ȡclient�����Ӿ��
         */
        int32_t GetClientFd() {
            return _client_fd;
        };
        
        /**
         * @brief ����client�����Ӿ��
         */
        void SetClientFd(int32_t fd) {
            _client_fd = fd;
        };

    private:

        /** 
         * @brief ���캯��
         */
        TNetCommuMng() ;
    
        static TNetCommuMng *_instance;         // �������� 
        
        TNetCommuSvrMap        _server_map;     // ������Ӷ���·��ע��
        TNetCommuClient*       _client;         // ÿworkerһ�����Ӷ���
        
        TCommuSvrDisp          _svr_disp;       // server�ķַ�֪ͨ���� 
        TCommuCltDisp          _clt_disp;       // client�ķַ�֪ͨ����
        int32_t                _client_fd;      // ��Ϊ�ͻ���, ����fd������Ϊ֪ͨfd

        void*                  _notify_obj;     // ע���֪ͨ���, server��Ҫ
        CMsgStat               _stat_obj;       // ͳ�ƽӿ� 
        CMsgCtxMng             _msg_ctx;        // ��Ϣ�����Ĺ���
        
    };


}

#endif

