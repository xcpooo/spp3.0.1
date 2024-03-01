/**
 * @file nest_proc.h
 * @info ���̹�����
 */

#ifndef _NEST_PROC_H__
#define _NEST_PROC_H__


#include <stdint.h>
#include <pthread.h>
#include "nest_proto.h"
#include "nest.pb.h"
#include "sys_info.h"
#include <map>

using std::map;

namespace nest
{

    // ϵͳ���ص�Ԫ����
    struct LoadUnit
    {
        uint64_t  _load_time;       // ���ص���ʼ����
        uint32_t  _cpu_max;         // cpu����*100      
        uint32_t  _mem_max;         // ʵ�ʿ����ڴ�M
        uint32_t  _net_max;         // �����������M
        uint32_t  _cpu_used;        // cpuʵ��ռ��
        uint32_t  _mem_used;        // �ڴ�ʵ��ռ��
        uint32_t  _net_used;        // ����ʵ��ռ��
    };

    // ҵ�����ͳ�ƶ���
    struct StatUnit
    {
        uint64_t  _stat_time;       // ���ӵ���ʼ����
        uint64_t  _total_req;       // �ܵ�������
        uint64_t  _total_cost;      // �ܵ�ʱ��
        uint64_t  _total_rsp;       // �ܵĳɹ�������
        uint32_t  _max_cost;        // ���ʱ��
        uint32_t  _min_cost;        // ��Сʱ��
    };

    // ����ȫ��Ψһkey��Ϣ����
    class TProcKey
    {
    public:

        // stl ���������ȽϺ���
        bool operator < (const TProcKey& key) const {
            if (this->_service_id != key._service_id) {
                return this->_service_id < key._service_id;
            } 
            
            if (this->_proc_type != key._proc_type) {
                return this->_proc_type < key._proc_type;
            } 

            return this->_proc_no < key._proc_no;
        };

        // ��׼�ȽϺ���
        bool operator == (const TProcKey& key) const {
            if (this->_service_id != key._service_id) {
                return false;
            }             
            if (this->_proc_type != key._proc_type) {
                return false;
            } 
            if (this->_proc_no != key._proc_no) {
                return false;
            }
            return true;
        };

    public:
    
        uint32_t  _service_id;      // ҵ��id
        uint32_t  _proc_type;       // ������id
        uint32_t  _proc_no;         // ����ȫ�ֱ��
    };

    

    // ���̹���ṹ����
    class TNestProc
    {
    public:

        // ����ö�ٶ���
        enum _proc_magic
        {
            MAX_LOAD_NUM   = 60,            // һ���Ӹ�������
            MAX_STAT_NUM   = 5,             // �����ҵ��ͳ��
        };

        ///< ��������������
        TNestProc() ;
        virtual ~TNestProc() {};      

        ///< ��ȡ���һ�����ڵ�ҵ����
        StatUnit& GetLastStat();

        ///< ��ȡ���µ�ҵ����
        StatUnit& GetCurrStat();

        ///< ����ʵʱ�ĸ�����Ϣ
        void UpdateStat(StatUnit& stat);

        ///< ��ȡ���µ�ϵͳ����
        LoadUnit& GetLastLoad();

        ///< ����ϵͳ�ĸ���
        void SetLoad(LoadUnit& load);

        ///< ��ȡ������conf��Ϣ
        nest_proc_base& GetProcConf() {
            return _conf;
        };

        ///< ���»�����������Ϣ
        void SetProcConf(const nest_proc_base& conf) {
            _conf = conf;
        };

        ///< ��ȡ��ǰ��pid��Ϣ
        uint32_t GetProcPid() {
            return _conf.proc_pid();
        };

        ///< ���õ�ǰ��pid��Ϣ
        void SetProcPid(uint32_t pid) {
            _conf.set_proc_pid(pid);
        };       

        ///< ��ȡҵ����
        string& GetServiceName() {
            return _service;
        };

        ///< ����ҵ����
        void SetServiceName(const string& name) {
            _service = name;
        };

        ///< ��ȡ��װ����(·����)
        string& GetPackageName() {
            return _package;
        };

        ///< ����·����
        void SetPackageName(const string& name) {
            _package = name;
        };

        ///< ��������ʱ��
        void UpdateHeartbeat(uint64_t time) {
            _heartbeat = time;
        };

        ///< ��ѯ����ʱ��
        uint64_t GetHeartbeat() {
            return _heartbeat;
        };

    protected:

        uint32_t        _load_pos;                // �����±�
        LoadUnit        _load[MAX_LOAD_NUM];      // ����һ���ӵĸ���, ÿ��һ�θ���
        uint32_t        _stat_pos;                // ҵ��ͳ���±�  
        StatUnit        _stat[MAX_STAT_NUM];      // ��������ӵ�ͳ�ƣ�ÿ��һ�θ���
        nest_proc_base  _conf;                    // ������������Ϣ  
        string          _service;                 // ҵ����  
        string          _package;                 // ����  
        uint64_t        _heartbeat;               // ���һ������
    };

    typedef std::vector<TNestProc*>         TProcList; // �����б���Ϣ
    typedef std::map<TProcKey, TNestProc*>  TProcMap;   // ��ҵ�����IDΪkey��map
    typedef std::map<uint32_t, TNestProc*>  TPidMap;    // ��PIDΪKEY��map, ��Ҫ����

    typedef std::map<uint32_t, TProcKey>    TPortMap;   // ���ؼ��������map, ����ͻ
    typedef std::map<uint32_t, TPidMap*>    TGroupMap;  // ��group, ��pid��map, dc���߿���

    /**
     * @brief ���ؽ�������DCͨ����ع���
     */
    class CPkgProcMap
    {
    public:

        // ���캯��
        CPkgProcMap(string& pkg_name, string& service_name);

        // ��������
        ~CPkgProcMap();    

        // ����pid��·��ӳ���ϵ
        void add_pid_proc(TNestProc* proc);

        // ɾ��pid��·��ӳ���ϵ
        void del_pid_proc(TNestProc* proc);         

        // ���pid�б��Ƿ���
        bool check_pids_change();

        // ����pid�б���ַ���
        void dump_pids_string(string& pids);

        // ��ȡpid����Ŀ, ����Чpid, ��ͣreport
        uint32_t get_pid_count();

        // ��pid�б���Ϣ, д��pid�ļ�
        void write_pids_file();

        // ���report�����Ƿ����
        int32_t check_report_tool();

        // ȷ����־�ļ�����
        void touch_log_file();

        // ����report��������
        bool start_report_tool();
        
        // ֹͣreport��������
        void stop_report_tool();

        // ���½����������쳣��¼
        void write_proc_dead_log(uint32_t group, uint32_t pid);

    public:

        uint32_t    _change_flag;                // �Ƿ����½��̱��
        string      _pkg_name;                   // ·����ӳ��
        string      _service_name;               // ҵ������Ϣ
        TGroupMap   _group_map;                  // ��������map
    };
    typedef std::map<string, CPkgProcMap*>  TPkgMap;  // ������map


    /**
     * @brief ȫ�ֵİ�����ӿ�, ����������
     */
    class CPkgMng
    {
    public:

        // ���캯��
        CPkgMng(){};

        // ��������
        ~CPkgMng();

        // ��������ӽ���
        void add_proc_pkg(TNestProc* proc);

        // ������ɾ������
        void del_proc_pkg(TNestProc* proc);

        // ����������־
        void write_dead_log(TNestProc* proc);

        // ���pid�������ϱ����ߵ�����
        void check_run();
        
    private:

        TPkgMap     _pkg_map;                    // ����ӳ��            

    };
    
    
    /** 
     * @brief ����Զ�̽�����������ȫ�ֹ���
     */
    class  TNestProcMng
    {
    public:

        /** 
         * @brief ��������
         */
        ~TNestProcMng();

        /** 
         * @brief �����������ʽӿ�
         */
        static TNestProcMng* Instance();

        /** 
         * @brief ȫ�ֵ����ٽӿ�
         */
        static void Destroy();

        /** 
         * @brief ��ӽ��̽ӿ�
         */
        int32_t AddProc(TNestProc* proc);

        /** 
         * @brief ɾ�����̽ӿ�
         */
        void DelProc(TNestProc* proc);

        /** 
         * @brief ��ѯ���̽ӿ�
         */
        TNestProc* FindProc(TProcKey& key);
        TNestProc* FindProc(uint32_t pid);

        /** 
         * @brief ��ȡ��Ч��pid��Ϣ�б�
         */
        void GetPidList(std::vector<uint32_t>& pid_list);

        /** 
         * @brief ��ȡָ��service name��ҵ������б�
         */
        void GetServiceProc(const string& service, TProcList& proc_list);

        /** 
         * @brief ��ȡȫ�ֵĽ�����Ϣ�б�
         */
        TProcMap& GetProcMap() {
            return _pno_map;
        };

        /** 
         * @brief ��ȡȫ�ֵİ������б�
         */
        CPkgMng& GetPkgMng() {
            return _pkg_mng;
        };

        /**
         * @brief ��������, ���½���pid
         */
        void UpdateProcPid(TNestProc* proc, uint32_t pid);

        /**
         * @brief ����ϵͳ����
         */
        void UpdateSysLoad(LoadUnit& load) {
            memcpy(&_sys_load, &load, sizeof(_sys_load));
        };

        /**
         * @brief ��ȡϵͳ����
         */
        LoadUnit& GetSysLoad() {
            return _sys_load;
        };   

        /**
         * @brief �������Ƿ�������ʱ
         */
        void CheckHeartbeat(uint64_t now);

        /**
         * @brief ����ҵ�����
         */
        void RestartProc(TNestProc* proc);

        /**
         * @brief ��ȡ���õ�port��Ϣ
         */
        uint32_t GetUnusedPort(TProcKey& proc);

        /**
         * @brief �ͷŷ����port��Ϣ
         */
        void FreeUsedPort(TProcKey& proc, uint32_t port);  

        /**
         * @brief ����ռ��port��Ϣ, ��ͻҲ�Ը���Ϊ׼
         */
        void UpdateUsedPort(TProcKey& proc, uint32_t port);

        /**
         * @brief ����ע��port, ��ͻֱ�ӷ���ʧ��
         */
        bool RegistProxyPort(TProcKey& proc, uint32_t port);

    private:

        /** 
         * @brief ���캯��
         */
        TNestProcMng(){
            memset(&_sys_load, 0, sizeof(_sys_load));
        };
    
        static TNestProcMng *_instance;         // �������� 
        LoadUnit             _sys_load;         // ϵͳ�ĸ�����Ϣ        
        TProcMap             _pno_map;          // TProcKeyΪkey�Ĺ���map
        TPidMap              _pid_map;          // pid Ϊkey�ĸ��ع���map
        TPortMap             _port_map;         // port����map����
        
        CPkgMng              _pkg_mng;          // pkg·�����ع���           
    };


    struct ServiceInfo
    {
        string    name;         // ҵ����
        uint32_t  type;         // ҵ������: 3->΢�߳� 2->�첽 1->ͬ��
        uint32_t  report_times; // �ϱ��������Ĵ���
    };

    typedef std::map<std::string, ServiceInfo>  SrvMap;

    class CServiceMng
    {
    public:

        /// ���캯��
        CServiceMng();

        /// ����worker����
        void UpdateType(std::string& name, uint32_t type);

        /// ����pb��ʽ���ϱ�����
        void GenPbReport(nest_sched_load_report_req &proto);

        /// ��ȡ����ָ��
        static CServiceMng* Instance();

    public:
        static CServiceMng*             _instance;  // ����ָ��
        pthread_mutex_t                 _lock;      // �߳���
        SrvMap                          _srv_map;   // ҵ��MAP
    };

    
};

#endif

