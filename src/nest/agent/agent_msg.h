/**
 * @brief Nest agent msg 
 */
 
#include <stdint.h>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"
#include "agent_msg_incl.h"

#ifndef __NEST_MSG_H_
#define __NEST_MSG_H_

using std::map;
using std::vector;

namespace nest
{

    /**
     * @brief �ڵ������Ϣ����ʵ��
     */
    class TMsgNodeSet :     public TNestMsgHandler
    {
    public:

        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ��ȡagent server��ַ��Ϣ
        void ExtractServers(TNestAddrArray& servers);

        // ��ȡ��ǰѡ�е�server��Ϣ
        char* GetCurrServer();

    protected:
    
        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_sched_node_init_req  _req_body;        // ������ṹ
        nest_sched_node_init_rsp  _rsp_body;        // Ӧ����ṹ
    };

    /**
     * @brief �ڵ�ɾ����Ϣ����ʵ��
     */
    class TMsgNodeDel :     public TNestMsgHandler
    {
    public:
    
        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

    protected:

        nest_msg_head             _msg_head;         // ͬ��������Ϣͷ��Ϣ
        nest_sched_node_term_req  _req_body;         // ������ṹ
    };

    /**
     * @brief ���̴�����Ϣ����ʵ��
     */
    class TMsgAddProcReq :     public TNestMsgHandler
    {
    public:

        // ��ֹ�ظ������ļ�麯��
        int32_t CheckProcExist(string& errmsg);

        // ���ط���proxy�Ķ˿�, ��ʱ�߼�����
        int32_t AllocProxyPort(string& errmsg);
        
        // ����������ķ����˷�0��port,����ʹ��, ����hash����
        int32_t CheckAllocPort(nest_proc_base* proc_conf, string& errmsg);
    
        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ��ȡ��Ϣͷ�ӿ�
        nest_msg_head& GetMsgHead() {
            return _msg_head;
        };

        // ��ȡ��Ϣ�������
        nest_sched_add_proc_req& GetMsgReqBody() {
            return _req_body;
        };
        

    protected:

        nest_msg_head             _msg_head;         // ͬ��������Ϣͷ��Ϣ
        nest_sched_add_proc_req   _req_body;         // ������ṹ
        nest_sched_add_proc_rsp   _rsp_body;         // Ӧ����ṹ
    };
    

    /**
     * @brief ���̴�����ϢӦ����
     */
    class TMsgAddProcRsp :      public TNestMsgHandler
    {
    public:

        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ����ɹ����½�����Ϣ
        void UpdateProc();

        // ����ʧ��,ɾ������
        void DeleteProc();

        // ʧ��ʱ, �ع�����ռ�õĶ˿���Ϣ
        void FreeProxyPort();

    protected:

        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_sched_add_proc_rsp   _rsp_body;        // ������ṹ
    };
    

    /**
     * @brief ����ɾ����Ϣ����ʵ��
     */
    class TMsgDelProcReq :     public TNestMsgHandler
    {
    public:
    
        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ����ɾ���ӿ�ʱ, ������ֹɾ�����������
        void SetProcDeleteFlag();

        // ��ȡ��Ϣͷ�ӿ�
        nest_msg_head& GetMsgHead() {
            return _msg_head;
        };

        // ��ȡ��Ϣ�������
        nest_sched_del_proc_req& GetMsgReqBody() {
            return _req_body;
        };

    protected:

        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_sched_del_proc_req   _req_body;        // ������ṹ
        nest_sched_del_proc_rsp   _rsp_body;        // Ӧ����ṹ
    };

    /**
     * @brief ����ɾ����ϢӦ����
     */
    class TMsgDelProcRsp :      public TNestMsgHandler
    {
    public:

        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ����ɹ����½�����Ϣ
        void DeleteProc();

    protected:

        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_sched_del_proc_rsp   _rsp_body;        // ������ṹ
    };

    /**
     * @brief ��ȡҵ����Ϣ
     */
    class TMsgSrvInfoReq :       public TNestMsgHandler
    {
    public:

        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

    protected:

        nest_msg_head                 _msg_head;         // ͬ��������Ϣͷ��Ϣ
        nest_sched_service_info_req   _req_body;         // ������ṹ
        nest_sched_service_info_rsp   _rsp_body;         // Ӧ����ṹ
    };


    /**
     * @brief ����������Ϣ����ʵ��
     */
    class TMsgRestartProcReq :     public TNestMsgHandler
    {
    public:

        // ��鲢������ȷ��pid��Ϣ
        int32_t CheckProcPids(string& errmsg);

        // ��ҵ������ȡproc�б�, �޽��̷���С��0
        int32_t GetServiceProc(string& errmsg);

        // ���������Ϣ����Ч��
        int32_t CheckRestart(string& errmsg);
    
        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ��ȡ��Ϣͷ�ӿ�
        nest_msg_head& GetMsgHead() {
            return _msg_head;
        };

        // ��ȡ��Ϣ�������
        nest_sched_restart_proc_req& GetMsgReqBody() {
            return _req_body;
        };

    protected:

        nest_msg_head                 _msg_head;         // ͬ��������Ϣͷ��Ϣ
        nest_sched_restart_proc_req   _req_body;         // ������ṹ
        nest_sched_restart_proc_rsp   _rsp_body;         // Ӧ����ṹ
    };
    

    /**
     * @brief ���̴�����ϢӦ����
     */
    class TMsgRestartProcRsp :      public TNestMsgHandler
    {
    public:

        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

    protected:

        nest_msg_head                 _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_sched_restart_proc_rsp   _rsp_body;        // ������ṹ
    };

    /**
     * @brief �ڲ������ϱ��ӿ�
     */
    class TMsgSysLoadReq :      public TNestMsgHandler
    {
    public:
    
        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ����ϵͳ������Ϣ�����̹���ṹ
        int32_t UpdateSysLoad();

        // ���û�Ծ��pid�б�Ӧ����
        void SetRspPidList();

    protected:

        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_agent_sysload_req    _req_body;        // ������ṹ
        nest_agent_sysload_rsp    _rsp_body;        // Ӧ����ṹ
    };


    /**
     * @brief �ڲ������ϱ��ӿ�
     */
    class TMsgProcHeartReq :      public TNestMsgHandler
    {
    public:

        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ���½��̵�����ʱ��
        void UpdateHeartbeat();
        
        // �����������ֽ��̵��߼�
        void AddProc();

    protected:

        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_proc_heartbeat_req   _req_body;        // ������ṹ
    };

    
    /**
     * @brief ���̸����ϱ��ӿ�
     */
    class TMsgProcLoadReq :      public TNestMsgHandler
    {
    public:

        // ��Ϣ����ӿں���
        virtual int32_t Handle(void* args);

        // ���½��̵ĸ�����Ϣ
        int32_t UpdateProcLoad();

    protected:

        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_proc_stat_report_req _req_body;        // ������ṹ
    };
    
}

#endif


