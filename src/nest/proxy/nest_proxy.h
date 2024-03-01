/**
 * @brief PROXY NEST 
 * @info  ����޶ȼ���֧��spp proxy
 */


#ifndef _NEST_PROXY_DEFAULT_H_
#define _NEST_PROXY_DEFAULT_H_

#include "stdint.h"
#include "frame.h"
#include "serverbase.h"
#include "tshmcommu.h"
#include "tsockcommu.h"
#include "benchapiplus.h"
#include "singleton.h"
#include "comm_def.h"
#include <vector>
#include <set>
#include <dlfcn.h>
#include <map>
#include "proc_comm.h"

#define IPLIMIT_DISABLE		  0x0
#define IPLIMIT_WHITE_LIST    0x1
#define IPLIMIT_BLACK_LIST    0x2
#define NEED_ROUT_FLAG        -10000

using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;
using namespace tbase::tcommu::tsockcommu;
using namespace std;
using namespace nest;

namespace nest
{
    class CProxyCtrl;       // proxy ���ƴ�����
    class CNestProxy;       // proxy �����

    /**
     * @brief �������̶�����
     */
    class CProxyCtrl : public CProcCtrl
    {
    public:

        ///< ��������������
        CProxyCtrl() {};
        virtual ~CProxyCtrl() {};

        ///< ��ӿ�, �����������ӿ�ʵ��
        virtual int32_t DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< ������Ϣ-��ʼ��������Ϣ����
        int32_t RecvProcInit(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        ///< ����ȫ�ֽ��̶���
        void SetServerBase(CNestProxy* base) {
            _server_base = base;    
        };

    protected:

        CNestProxy*           _server_base;     // ��������ָ��
    };


    /**
     * @brief ����ඨ��, ˽��ʵ��
     */
    class CNestProxy : public CServerBase, public CFrame
    {
    public:

        ///< ��������������
        CNestProxy();
        ~CNestProxy();

        ///< ��������fork����, �����ʼ��
        virtual void startup(bool bg_run = true);

        ///< ������ʵִ�к���
        void realrun(int argc, char* argv[]);

        ///< Ĭ�ϵĽ�������
        int32_t servertype() {
            return SERVER_TYPE_PROXY;
        }
        
        ///< ���ó�ʼ��
        int32_t initconf(bool reload = false);

        ///< �ص���������
        static int32_t ator_recvdata(uint32_t flow, void* arg1, void* arg2);
        static int32_t ctor_recvdata(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_overload(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_connected(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_senderror(uint32_t flow, void* arg1, void* arg2);	
        static int32_t ator_recvdata_v2(uint32_t flow, void* arg1, void* arg2);
        static int32_t ator_disconnect(uint32_t flow, void* arg1, void* arg2);

        
        ///< ִ�������е���������
        void handle_routine();
        
        ///< ִ��ҵ������
        void handle_service();

        ///< ִ���˳�ǰ������
        void handle_before_quit();

        ///< ���պ�˻ذ�
        int32_t recv_ctor_rsp();

        ///< ��ʼ�����ƽӿ�, ������������
        bool init_ctrl_channel();

    public:
        
        CTCommu*                    ator_;              ///< ǰ�˽�����
        map<int32_t, CTCommu*>      ctor_;              ///< ���������
        uint8_t                     iplimit_;           ///< ip�������� ����-������-������
        set<uint32_t>               iptable_;           ///< ip����
        uint32_t                    initiated_;         ///< �Ƿ��ʼ��, 0 δ��ʼ��

        spp_handle_process_t        local_handle;
        
        CProxyCtrl                  ctrl_;                         
    };



    
}
#endif
