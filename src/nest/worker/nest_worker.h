/**
 * @brief WORKER NEST 
 * @info  ����޶ȼ���֧��spp worker
 */

#ifndef _NEST_WORKER_DEFAULT_H_
#define _NEST_WORKER_DEFAULT_H_
#include "frame.h"
#include "serverbase.h"
#include "tshmcommu.h"
#include "tsockcommu.h"
#include "proc_comm.h"

using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;
using namespace tbase::tcommu::tsockcommu;

using namespace nest;

extern CServerBase* g_worker;

namespace nest
{

    class CWorkerCtrl;       // worker ���ƴ�����
    class CNestWorker;       // worker �����

    /**
     * @brief �������̶�����
     */
    class CWorkerCtrl : public CProcCtrl
    {
    public:

        ///< ��������������
        CWorkerCtrl() {};
        virtual ~CWorkerCtrl() {};

        ///< ��ӿ�, �����������ӿ�ʵ��
        virtual int32_t DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< ������Ϣ-��ʼ��������Ϣ����
        int32_t RecvProcInit(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< ����ȫ�ֽ��̶���
        void SetServerBase(CNestWorker* base) {
            _server_base = base;    
        };

    protected:

        CNestWorker*           _server_base;     // ��������ָ��
    };


    /**
     * @brief ����ඨ��, ˽��ʵ��
     */
    class CNestWorker : public CServerBase, public CFrame
    {
    public:

        ///< ��������������
        CNestWorker();
        ~CNestWorker();

        ///< ��������fork����, �����ʼ��
        virtual void startup(bool bg_run = true);
        
        ///< ������ʵִ�к���
        void realrun(int32_t argc, char* argv[]);
        
        ///< Ĭ�ϵĽ�������
        int32_t servertype() {
            return SERVER_TYPE_WORKER;
        }

        ///< ע��spp�źŴ�����
        void assign_signal(int signo);
        
        ///< ���ó�ʼ��
        int32_t initconf(bool reload = false);

        ///< �ص���������
        static int32_t ator_recvdata(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_recvdata_v2(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_senddata(uint32_t flow, void* arg1, void* arg2);	
        static int32_t ator_overload(uint32_t flow, void* arg1, void* arg2);	
        static int32_t ator_senderror(uint32_t flow, void* arg1, void* arg2);	

        ///< ��ȡtos�ӿں���
		inline int32_t get_TOS(){return TOS_;}

        ///< ִ�������е���������
        void handle_routine();
        
        ///< ִ��ҵ������
        void handle_service();
        
        ///< ִ���˳�ǰ������
        void handle_before_quit();
        
        ///< ��ʼ�����ƽӿ�, ������������
        bool init_ctrl_channel();

        ///< ����Ƿ�����΢�߳�
        bool micro_thread_enable();

        ///< �л���΢�̵߳ĵ���
        void micro_thread_switch(bool block);

    public:

        CTCommu*         ator_;                 // ǰ�˽��������
        CWorkerCtrl      ctrl_;                 // ���ƿ����
        uint32_t         initiated_;            // �Ƿ��ʼ��, 0 δ��ʼ��
		
	private:
	
        uint32_t         msg_timeout_;          // ��ʱʱ��
		int32_t          TOS_;                  // TOS�������
		int32_t          notify_fd_;            // socket֪ͨ��� 
		bool             mt_flag_;              // �Ƿ�ʹ��΢�߳�
    };
}

#endif

