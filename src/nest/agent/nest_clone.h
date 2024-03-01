/**
 * @brief Nest agent clone
 *  fork proxy/worker
 */
#ifndef _NEST_AGENT_CLONE_H_
#define _NEST_AGENT_CLONE_H_

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
#include "agent_msg_incl.h"
#include "nest_timer.h"
#include "agent_cgroups.h"
#include "agent_net_dispatch.h"

using namespace tbase::tlog;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace spp::comm;
using namespace std;
using namespace nest;


namespace nest
{

	class TMsgProcFork:public TNestMsgHandler
	{
	public:
        // ʵ�ʵ���Ϣ����ӿں���, �̳�ʵ��
        int32_t Handle(void* args);	
	private:
		static int32_t CloneEntry(void* args);
		pid_t DoFork(void* args);
	protected:
        nest_msg_head             _msg_head;        // ͬ��������Ϣͷ��Ϣ
        nest_sched_proc_fork_req  _req_body;        // ������ṹ
        nest_sched_proc_fork_rsp  _rsp_body;        // Ӧ����ṹ	
	};


    /**
     * @brief Agentִ�д�����
     */
    class CCloneProxy:public TMsgDispatch
    {
    public:

        ///< ��������ʺ���
        static CCloneProxy* Instance();

        ///< ȫ�ֵ����ٽӿ�
        static void Destroy();

        ///< ����������
        virtual ~CCloneProxy();

        ///< ʵ�ʳ�ʼ������
        int32_t Init();

        ///< ��Ϣ�ַ�������
        int32_t dispatch(void* args);

        ///< ��Ϣ������ע�����ѯ����
        void RegistMsgHandler(uint32_t main_cmd, uint32_t sub_cmd, TNestMsgHandler* handle);
        TNestMsgHandler* GetMsgHandler(uint32_t main_cmd, uint32_t sub_cmd);

        ///< Ӧ�����뷢�ͽӿ�
        int32_t SendRspMsg(TNestBlob* blob, string& head, string& body);

        ///< ��ȡ���ȵ�������Ϣ
        void LoadSetInfo();

        ///< ��ȡ��ַ ת����ַ�ṹ
        void ExtractServerList(TNestAddrArray& svr_list);
        ///< ��ȡcgroups������
        CAgentCGroups& GetCGroupHandle() {
            return _cgroups;
        };
    private:

        ///< ���캯��
        CCloneProxy();

        static CCloneProxy*                 _instance;    // ����ʵ��

        uint32_t                            _set_id;      // set��Ϣ
        std::vector<string>                 _set_servers; // set��������Ϣ
        MsgDispMap                          _msg_map;     // ��Ϣע�����

        CAgentCGroups                       _cgroups;     // cgroups����
    public:
        static std::string                         _cluster_type;
    };




    /**
     * @brief �䳲clone��ܶ���
     */
    class NestClone : public CServerBase, public CFrame
    {
    public:

        /**
         * @brief ��������������
         */
        NestClone() {};
        ~NestClone() {};

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

    };

}




#endif



