/**
 * @brief Nest agent process 
 */
#ifndef _NEST_AGENT_PROCESS_H_
#define _NEST_AGENT_PROCESS_H_

#include <stdint.h>
#include <set>
#include <map>
#include "frame.h"
#include "serverbase.h"
#include "comm_def.h"
#include "global.h"
#include "timerlist.h"
#include "nest_channel.h"
#include "nest_proto.h"
#include "agent_process.h"
#include "nest_thread.h"
#include "agent_msg.h"
#include "nest_timer.h"
#include "agent_cgroups.h"

using namespace tbase::tlog;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace spp::comm;
using namespace std;
using namespace nest;


namespace nest
{



    /**
     * @brief  Agentִ�������������, ��¼������Դ����Ϣ
     */
    class CTaskCtx : public CTimerObject
    {
    public:

        ///<  ���캯��
        CTaskCtx() {
            _taskid = 0;
        };

        ///<  ��������
        virtual ~CTaskCtx() {
            DisableTimer();
        };

        ///<  ��ʱ������
        virtual void TimerNotify();

        ///<  ������ʱ����
        void EnableTimer(uint32_t wait_time);

        ///<  ������������Ϣ
        void SetBlobInfo(TNestBlob* blob) {
            _blob = *blob;
        };

        ///<  ��ȡ��������Ϣ
        TNestBlob& GetBlobInfo() {
            return _blob;
        };

        ///<  ��������id��Ϣ
        void SetTaskId(uint32_t taskid) {
            _taskid = taskid;
        };

        ///<  ��ȡ����id��Ϣ
        uint32_t GetTaskId() {
            return _taskid;
        };

    protected:

        uint32_t                  _taskid;          ///< ����id, ȫ��Ψһ
        TNestBlob                 _blob;            ///< ��������Ϣ
    };
    typedef std::map<uint32_t, CTaskCtx*>  NestTaskMap;


    /**
     * @brief Agentִ�д�����
     */
    class CAgentProxy
    {
    public:
    
        ///< ��������ʺ���
        static CAgentProxy* Instance();

        ///< ȫ�ֵ����ٽӿ�
        static void Destroy();

        ///< ����������
        virtual ~CAgentProxy();

        ///< ʵ�ʳ�ʼ������
        int32_t Init();

        ///< ���õ��ȵ�������Ϣ
        int32_t DumpSetInfo();

        ///< ��ȡ���ȵ�������Ϣ
        void LoadSetInfo();

        ///< ���µ��ȵ�set��Ϣ
        int32_t UpdateSetInfo(uint32_t setid, TNestAddrArray& servers);

        ///< ������ȵ�set��Ϣ
        int32_t ClearSetInfo();

        ///< ��ȡ��ַ ת����ַ�ṹ
        void ExtractServerList(TNestAddrArray& svr_list);
        
        ///< ��Ϣ�ַ�������
        int32_t DispatchMsg(void* args);

        ///< ��Ϣ������ע�����ѯ����
        void RegistMsgHandler(uint32_t main_cmd, uint32_t sub_cmd, TNestMsgHandler* handle);
        TNestMsgHandler* GetMsgHandler(uint32_t main_cmd, uint32_t sub_cmd);

        ///< Ӧ�����뷢�ͽӿ�
        int32_t SendRspMsg(TNestBlob* blob, string& head, string& body); 
        int32_t SendToAgent(string& head, string& body); 

        ///< �������ӿں���, ���������
        void AddTaskCtx(uint32_t taskid, CTaskCtx* ctx);

        ///< �������ӿں���, ɾ��������
        void DelTaskCtx(uint32_t taskid);

        ///< �������ӿں���, ����������
        CTaskCtx* FindTaskCtx(uint32_t taskid);

        ///< ȫ�ֵķ���taskid, ����Ψһ
        uint32_t GetTaskSeq();

        ///< stat�ϱ���Ϻ�����װ
        static void ProcStatSet(nest_proc_base& base, StatUnit& stat, LoadUnit& load, nest_proc_stat* report);

        /**
         * @brief ��ʱ����ص���������
         */
        static int32_t KeepConnectionTimer(void* args);
        static int32_t HeartbeatTimer(void* args);
        static int32_t LoadReportTimer(void* args);
        static int32_t DcToolTimer(void* args);
        static int32_t TaskCheckTimer(void* args);

        ///< ��ȡsetid
        uint32_t GetSetId() {
            return _set_id;
        };

        ///< ��ȡcgroups������
        CAgentCGroups& GetCGroupHandle() {
            return _cgroups;
        };



    private:

        ///< ���캯��
        CAgentProxy();
        
        static CAgentProxy*                 _instance;    // ����ʵ��
        
        uint32_t                            _set_id;      // set��Ϣ
        std::vector<string>                 _set_servers; // set��������Ϣ
        NestTaskMap                         _task_map;    // �����б�
        MsgDispMap                          _msg_map;     // ��Ϣע�����
        CAgentCGroups                       _cgroups;     // cgroups����

    public:
        static string                       _cluster_type;// ��Ⱥ���� spp or sf2 

    };

    /**
     * @brief �񳲴����ܶ���
     */
    class NestAgent : public CServerBase, public CFrame
    {
    public:

        /**
         * @brief ��������������
         */
        NestAgent():_clone_proc_pid(-1) {};
        ~NestAgent() {};

        /**
         * @brief ҵ����������, ������ʼ��
         */
        virtual void run(int argc, char* argv[]);

        /**
         * @brief ҵ�����к���, ��ѭ��
         */
        virtual void realrun(int argc, char* argv[]);

        /**
         * @brief ���ó�ʼ������
         */
        int32_t initconf(int argc, char* argv[]);
	private:
		///< ���� clone ����
		int32_t startCloneProc(int argc, char* argv[]);

		///< ֹͣ clone ����
		int32_t stopCloneProc();
		

	private:
		pid_t								_clone_proc_pid; // clone ���̵�pid
    };

}




#endif


