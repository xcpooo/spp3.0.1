/**
 * @file nest_proc.h
 * @info 进程管理部分
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

    // 系统负载单元定义
    struct LoadUnit
    {
        uint64_t  _load_time;       // 负载的起始秒数
        uint32_t  _cpu_max;         // cpu总数*100      
        uint32_t  _mem_max;         // 实际可用内存M
        uint32_t  _net_max;         // 最大网卡流量M
        uint32_t  _cpu_used;        // cpu实际占用
        uint32_t  _mem_used;        // 内存实际占用
        uint32_t  _net_used;        // 网络实际占用
    };

    // 业务进程统计定义
    struct StatUnit
    {
        uint64_t  _stat_time;       // 分钟的起始秒数
        uint64_t  _total_req;       // 总的请求数
        uint64_t  _total_cost;      // 总的时延
        uint64_t  _total_rsp;       // 总的成功处理数
        uint32_t  _max_cost;        // 最大时延
        uint32_t  _min_cost;        // 最小时延
    };

    // 进程全局唯一key信息定义
    class TProcKey
    {
    public:

        // stl 关联容器比较函数
        bool operator < (const TProcKey& key) const {
            if (this->_service_id != key._service_id) {
                return this->_service_id < key._service_id;
            } 
            
            if (this->_proc_type != key._proc_type) {
                return this->_proc_type < key._proc_type;
            } 

            return this->_proc_no < key._proc_no;
        };

        // 标准比较函数
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
    
        uint32_t  _service_id;      // 业务id
        uint32_t  _proc_type;       // 进程组id
        uint32_t  _proc_no;         // 进程全局编号
    };

    

    // 进程管理结构定义
    class TNestProc
    {
    public:

        // 常量枚举定义
        enum _proc_magic
        {
            MAX_LOAD_NUM   = 60,            // 一分钟负载走势
            MAX_STAT_NUM   = 5,             // 五分钟业务统计
        };

        ///< 构造与析构函数
        TNestProc() ;
        virtual ~TNestProc() {};      

        ///< 获取最近一个周期的业务负载
        StatUnit& GetLastStat();

        ///< 获取最新的业务负载
        StatUnit& GetCurrStat();

        ///< 更新实时的负载信息
        void UpdateStat(StatUnit& stat);

        ///< 获取最新的系统负载
        LoadUnit& GetLastLoad();

        ///< 更新系统的负载
        void SetLoad(LoadUnit& load);

        ///< 获取基本的conf信息
        nest_proc_base& GetProcConf() {
            return _conf;
        };

        ///< 更新基本的配置信息
        void SetProcConf(const nest_proc_base& conf) {
            _conf = conf;
        };

        ///< 获取当前的pid信息
        uint32_t GetProcPid() {
            return _conf.proc_pid();
        };

        ///< 设置当前的pid信息
        void SetProcPid(uint32_t pid) {
            _conf.set_proc_pid(pid);
        };       

        ///< 获取业务名
        string& GetServiceName() {
            return _service;
        };

        ///< 设置业务名
        void SetServiceName(const string& name) {
            _service = name;
        };

        ///< 获取安装包名(路径名)
        string& GetPackageName() {
            return _package;
        };

        ///< 设置路径名
        void SetPackageName(const string& name) {
            _package = name;
        };

        ///< 更新心跳时间
        void UpdateHeartbeat(uint64_t time) {
            _heartbeat = time;
        };

        ///< 查询心跳时间
        uint64_t GetHeartbeat() {
            return _heartbeat;
        };

    protected:

        uint32_t        _load_pos;                // 负载下标
        LoadUnit        _load[MAX_LOAD_NUM];      // 保存一分钟的负载, 每秒一次更新
        uint32_t        _stat_pos;                // 业务统计下标  
        StatUnit        _stat[MAX_STAT_NUM];      // 保存五分钟的统计，每分一次更新
        nest_proc_base  _conf;                    // 配置与运行信息  
        string          _service;                 // 业务名  
        string          _package;                 // 包名  
        uint64_t        _heartbeat;               // 最近一次心跳
    };

    typedef std::vector<TNestProc*>         TProcList; // 进程列表信息
    typedef std::map<TProcKey, TNestProc*>  TProcMap;   // 以业务分配ID为key的map
    typedef std::map<uint32_t, TNestProc*>  TPidMap;    // 以PID为KEY的map, 需要更新

    typedef std::map<uint32_t, TProcKey>    TPortMap;   // 本地监听管理的map, 防冲突
    typedef std::map<uint32_t, TPidMap*>    TGroupMap;  // 分group, 分pid的map, dc工具控制

    /**
     * @brief 本地进程组与DC通道监控管理
     */
    class CPkgProcMap
    {
    public:

        // 构造函数
        CPkgProcMap(string& pkg_name, string& service_name);

        // 析构函数
        ~CPkgProcMap();    

        // 管理pid与路径映射关系
        void add_pid_proc(TNestProc* proc);

        // 删除pid与路径映射关系
        void del_pid_proc(TNestProc* proc);         

        // 检查pid列表是否变更
        bool check_pids_change();

        // 生成pid列表的字符串
        void dump_pids_string(string& pids);

        // 获取pid的数目, 无有效pid, 可停report
        uint32_t get_pid_count();

        // 将pid列表信息, 写入pid文件
        void write_pids_file();

        // 检查report工具是否存在
        int32_t check_report_tool();

        // 确保日志文件存在
        void touch_log_file();

        // 启动report工具运行
        bool start_report_tool();
        
        // 停止report工具运行
        void stop_report_tool();

        // 更新进程重启的异常记录
        void write_proc_dead_log(uint32_t group, uint32_t pid);

    public:

        uint32_t    _change_flag;                // 是否有新进程变更
        string      _pkg_name;                   // 路径的映射
        string      _service_name;               // 业务名信息
        TGroupMap   _group_map;                  // 分组管理的map
    };
    typedef std::map<string, CPkgProcMap*>  TPkgMap;  // 包管理map


    /**
     * @brief 全局的包管理接口, 按包名索引
     */
    class CPkgMng
    {
    public:

        // 构造函数
        CPkgMng(){};

        // 析构函数
        ~CPkgMng();

        // 包管理添加进程
        void add_proc_pkg(TNestProc* proc);

        // 包管理删除进程
        void del_proc_pkg(TNestProc* proc);

        // 进程死机日志
        void write_dead_log(TNestProc* proc);

        // 检查pid更新与上报工具的运行
        void check_run();
        
    private:

        TPkgMap     _pkg_map;                    // 包名映射            

    };
    
    
    /** 
     * @brief 网络远程交互连接器的全局管理
     */
    class  TNestProcMng
    {
    public:

        /** 
         * @brief 析构函数
         */
        ~TNestProcMng();

        /** 
         * @brief 单例类句柄访问接口
         */
        static TNestProcMng* Instance();

        /** 
         * @brief 全局的销毁接口
         */
        static void Destroy();

        /** 
         * @brief 添加进程接口
         */
        int32_t AddProc(TNestProc* proc);

        /** 
         * @brief 删除进程接口
         */
        void DelProc(TNestProc* proc);

        /** 
         * @brief 查询进程接口
         */
        TNestProc* FindProc(TProcKey& key);
        TNestProc* FindProc(uint32_t pid);

        /** 
         * @brief 获取有效的pid信息列表
         */
        void GetPidList(std::vector<uint32_t>& pid_list);

        /** 
         * @brief 获取指定service name的业务进程列表
         */
        void GetServiceProc(const string& service, TProcList& proc_list);

        /** 
         * @brief 获取全局的进程信息列表
         */
        TProcMap& GetProcMap() {
            return _pno_map;
        };

        /** 
         * @brief 获取全局的包管理列表
         */
        CPkgMng& GetPkgMng() {
            return _pkg_mng;
        };

        /**
         * @brief 重启进程, 更新进程pid
         */
        void UpdateProcPid(TNestProc* proc, uint32_t pid);

        /**
         * @brief 更新系统负载
         */
        void UpdateSysLoad(LoadUnit& load) {
            memcpy(&_sys_load, &load, sizeof(_sys_load));
        };

        /**
         * @brief 获取系统负载
         */
        LoadUnit& GetSysLoad() {
            return _sys_load;
        };   

        /**
         * @brief 检查进程是否心跳超时
         */
        void CheckHeartbeat(uint64_t now);

        /**
         * @brief 重启业务进程
         */
        void RestartProc(TNestProc* proc);

        /**
         * @brief 获取可用的port信息
         */
        uint32_t GetUnusedPort(TProcKey& proc);

        /**
         * @brief 释放分配的port信息
         */
        void FreeUsedPort(TProcKey& proc, uint32_t port);  

        /**
         * @brief 更新占用port信息, 冲突也以更新为准
         */
        void UpdateUsedPort(TProcKey& proc, uint32_t port);

        /**
         * @brief 尝试注册port, 冲突直接返回失败
         */
        bool RegistProxyPort(TProcKey& proc, uint32_t port);

    private:

        /** 
         * @brief 构造函数
         */
        TNestProcMng(){
            memset(&_sys_load, 0, sizeof(_sys_load));
        };
    
        static TNestProcMng *_instance;         // 单例类句柄 
        LoadUnit             _sys_load;         // 系统的负载信息        
        TProcMap             _pno_map;          // TProcKey为key的管理map
        TPidMap              _pid_map;          // pid 为key的负载管理map
        TPortMap             _port_map;         // port监听map管理
        
        CPkgMng              _pkg_mng;          // pkg路径本地管理           
    };


    struct ServiceInfo
    {
        string    name;         // 业务名
        uint32_t  type;         // 业务类型: 3->微线程 2->异步 1->同步
        uint32_t  report_times; // 上报调度中心次数
    };

    typedef std::map<std::string, ServiceInfo>  SrvMap;

    class CServiceMng
    {
    public:

        /// 构造函数
        CServiceMng();

        /// 更新worker类型
        void UpdateType(std::string& name, uint32_t type);

        /// 生成pb格式的上报数据
        void GenPbReport(nest_sched_load_report_req &proto);

        /// 获取单例指针
        static CServiceMng* Instance();

    public:
        static CServiceMng*             _instance;  // 单例指针
        pthread_mutex_t                 _lock;      // 线程锁
        SrvMap                          _srv_map;   // 业务MAP
    };

    
};

#endif

