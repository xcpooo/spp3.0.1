/**
 * @brief Nest agent thread 
 */
 
#include <stdint.h>
#include <vector>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"
#include "nest_thread.h"
#include "agent_msg.h"
#include "sys_info.h"


#ifndef __NEST_AGENT_THREAD_H_
#define __NEST_AGENT_THREAD_H_

using std::map;
using std::vector;

namespace nest
{

    /**
     * @brief ����ɨ���߳�
     */
    class CThreadSysload : public CNestThread
    {
    public:

        // �߳�ִ�нӿں���
        virtual void Run();

        // ��Դ������սӿ�
        virtual ~CThreadSysload();

        // ��ȡ���½�����Ϣ
        void ExtractProc(vector<uint32_t>& pids);

        // ��������б�
        void CleanProcMap(proc_map_t& proc_map);

        // ���²ɼ���Ϣ
        void UpdateLoad(uint32_t time_interval);

        // ��������ϱ���Ϣ
        int32_t PackReportPkg(void* buff, uint32_t buff_len);

        // ���زɼ��ϱ����ؽ��
        int32_t UnPackReportRsp(void* buff, uint32_t buff_len);

        // ͬ�������ϱ�����
        int32_t ReportSendRecv();

    protected:

        CSysInfo           _sys_info;           // ϵͳ������Ϣ
        proc_map_t         _proc_map;           // ���̸�����Ϣ
    };

    /**
     * @brief ���̹���Ĺ�������, �������
     */
    class CThreadProcBase 
    {
    public:

        /**
         * @brief �޸��ӽ��̵��û���
         * @return =0 �ɹ� <0 ʧ��
         */
        static int32_t ChgUser(const char *user);

        /**
         * @brief ���path�Ƿ���ڣ�����������򴴽�
         */
        static bool CheckAndMkdir(char *path);
        
        
        /**
         * @brief �ݹ鴴��Ŀ¼
         * @return =0 �ɹ� <0 ʧ��
         */
        static int32_t MkdirRecursive(char *path, size_t len);

        /**
         * @brief ��鲢��ӳ��/dataĿ¼��˽�е�path
         */
        static bool MountPriPath(string& package);

        
        /**
         * @brief cloneִ�н��̳�ʼ������ں���
         */
        static int32_t CloneEntry(void* args);

        /**
         * @brief ִ�и���Ŀ¼�Ľ���clone����
         * @return >0 �ӽ�������OK, ����PID, ����ʧ��
         */
        pid_t DoFork(void* args);

    
    public:

        // ���캯��
        CThreadProcBase(){};   

        // ��������
        virtual ~CThreadProcBase(){};

        // ��ȡspp����׼Ŀ¼
        char* GetBinBasePath();

        // �����������̵�ִ�к���
        int32_t StartProc(const nest_proc_base& conf, nest_proc_base& proc);

        // ֹͣ������������
        void DelProc(const nest_proc_base& proc);

        // �ȴ���������ִ�е�����unix�˿�
        void CheckWaitStart(uint32_t pid, uint32_t wait_ms);
        
        // �ȴ�����ִֹͣ��
        void CheckWaitStop(uint32_t pid, uint32_t wait_ms);

        // ��װ������������
        int32_t PackInitReq(void* buff, uint32_t buff_len, nest_proc_base& proc);

        // ������������Ӧ��
        int32_t UnPackInitRsp(void* buff, uint32_t buff_len, nest_proc_base& proc, uint32_t& worker_type);

        // �������̳�ʼ��ִ��
        int32_t InitSendRecv(nest_proc_base& proc, uint32_t& worker_type);

        // ����ҵ������, һ�ν�����һ��ҵ��
        void SetServiceName(const string& service_name) {
            _service_name = service_name;
        };
        
        // ����·������, һ�ν�����һ��·��
        void SetPackageName(const string& pkg_name) {
            _package_name = pkg_name;
        };

    protected:

        string          _service_name;          // ҵ����
        string          _package_name;          // ·����

    };
    

    /**
     * @brief ���̴��������̲߳���
     */
    class CThreadAddProc : public CNestThread, public CThreadProcBase
    {
    public:

        // �߳�ִ�нӿں���
        virtual void Run();

        // ��Դ������սӿ�
        virtual ~CThreadAddProc() {};

        // ���캯��
        CThreadAddProc() {
            _restart_flag = false;
        };

        // �½����̴���, ��ҪӦ����
        void RunStart();

        // �������̽ӿ�, ����Ӧ����Ϣ
        void RunRestart();

        // �½�����, ��¼��������Ϣ
        void SetCtx(TNestBlob* blob, TMsgAddProcReq* req);

        // ��������, ��¼��������Ϣ
        void SetRestartCtx(nest_sched_add_proc_req* req);

    protected:

        TNestBlob                 _blob;                // ��������Ϣ��
        nest_msg_head             _msg_head;            // ͨ����Ϣͷ
        nest_sched_add_proc_req   _req_body;            // ͨ�ý����������ӿ�
        nest_sched_add_proc_rsp   _rsp_body;            // ͨ�õĽ������Ӧ��
        bool                      _restart_flag;        // �Ƿ�����������    
    };

    
    /**
     * @brief ����ɾ�������̲߳���
     */
    class CThreadDelProc : public CNestThread, public CThreadProcBase
    {
    public:

        // �߳�ִ�нӿں���
        virtual void Run();
        
        // ��Դ������սӿ�
        virtual ~CThreadDelProc() {};

        // ɾ������, ��¼��������Ϣ
        void SetCtx(TNestBlob* blob, TMsgDelProcReq* req);

        // ִ�н���ɾ��ʵ��
        int32_t DeleteProcess(const nest_proc_base& proc);

    protected:

        TNestBlob                 _blob;                // ��������Ϣ
        nest_msg_head             _msg_head;            // ��Ϣͷ
        nest_sched_del_proc_req   _req_body;            // ������Ϣ
        nest_sched_del_proc_rsp   _rsp_body;            // Ӧ����Ϣ
    };

    
    /**
     * @brief ����������Ϣ�����̲߳���
     */
    class CThreadRestartProc : public CNestThread, public CThreadProcBase
    {
    public:

        // �߳�ִ�нӿں���
        virtual void Run();

        // ��Դ������սӿ�
        virtual ~CThreadRestartProc() {};

        // ���캯��
        CThreadRestartProc() {};

        // �½�����, ��¼��������Ϣ
        void SetCtx(TNestBlob* blob, TMsgRestartProcReq* req);

    protected:

        TNestBlob                   _blob;                // ��������Ϣ��
        nest_msg_head               _msg_head;            // ͨ����Ϣͷ
        nest_sched_restart_proc_req _req_body;            // ͨ�ý����������ӿ�
        nest_sched_restart_proc_rsp _rsp_body;            // ͨ�õĽ������Ӧ��
    };

}


#endif

