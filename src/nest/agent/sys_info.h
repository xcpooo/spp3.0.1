/**
 * @filename sys_info.h
 * @info 获取系统的与进程的CPU与负载信息
 */

#ifndef _SYS_INFO_H__
#define _SYS_INFO_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <map>

namespace nest
{

    // 常量宏定义
    enum 
    {
        STD_CPU_MHZ         = 2000,
        CPU_PER_PROCESSOR   = 10000, // story #57564145 提高cpu统计进度，有100修改为10000
    };
    typedef unsigned long long   llu64_t;
        

    // CPU 实时快照结构定义
    typedef struct _cpu_stat
    {
        uint64_t    _total;                 // 总的计数值, 单位jiff 10ms
        uint64_t    _user;                  // 累计的 用户态
        uint64_t    _nice;                  // 累计的 nice为负
        uint64_t    _system;                // 系统态运行时间
        uint64_t    _idle;                  // IO等待以外的其他等待时间
        uint64_t    _iowait;                // IO等待时间    
        uint64_t    _irq;                   // 硬中断
        uint64_t    _softirq;               // 软中断
        uint64_t    _stealstolen;           // 虚拟机等其他操作系统
        uint64_t    _guest;                 // 虚拟化cpu等
    }cpu_stat_t;

    // 内存数据结构定义
    typedef struct _mem_stat
    {
        uint64_t    _mem_total;             // 总的内存 KB    
        uint64_t    _mem_free;              // 可用物理内存    
        uint64_t    _buffers;               // buffer
        uint64_t    _cached;                // 文件cache等    
        uint64_t    _mapped;                // 已映射
        uint64_t    _inactive_file;         // 不活跃的文件cache

    }mem_stat_t;


    /**
     * @brief 全局的系统资源信息
     */
    class CSysInfo
    {
    public:

        /**
         * @brief 构造函数与析构函数
         */
        CSysInfo();
        ~CSysInfo() {};

        /**
         * @brief 初始化获取信息
         */
        void Init();

        /**
         * @brief 更新获取信息
         */
        void Update();

        /**
         * @brief 获取相对的CPU总值, 2G每核算100
         */        
        uint32_t GetCpuTotal() {
            return CPU_PER_PROCESSOR * _cpu_num * _cpu_mhz / STD_CPU_MHZ; 
        };
        
        /**
         * @brief 获取CPU的空闲值, 空闲比例*总值
         */            
        uint32_t GetCpuIdle() {
            return _cpu_idle * _cpu_num * _cpu_mhz / STD_CPU_MHZ; ; 
        };
        uint32_t GetCpuIdleRatio() {
            return _cpu_idle;
        };

        /**
         * @brief 获取CPU的负载信息
         */            
        uint32_t GetCpuUsed() {
            return GetCpuTotal() - GetCpuIdle();   
        };

        /**
         * @brief 获取内存总量
         */            
        uint32_t GetMemTotal() {
            return _mem_total;
        };

        /**
         * @brief 获取内存可使用量
         */            
        uint32_t GetMemFree() {
            return _mem_free;
        };

        /**
         * @brief 获取CPU数目
         */            
        uint32_t GetCpuNum() {
            return _cpu_num;
        };

        /**
         * @brief 获取CPUMHZ, 假设各cpu一致
         */            
        uint32_t GetCpuMhz() {
            return _cpu_mhz;
        };
                
        /**
         * @brief 提取内存统计信息
         */            
        static int32_t ExtractMemInfo(mem_stat_t& stat);        

        /**
         * @brief 提取CPU统计信息
         */            
        static void ExtractCpuStat(cpu_stat_t& stat);

        /**
         * @brief  获取CPU个数
         */
        static uint32_t GetCpuNums();

        /**
         * @brief 计算CPU空闲比例 * 100
         */            
        void CalcCpuIdle();

        /**
         * @brief 获取内存空闲大小
         */            
        void CalcMemFree();

        /**
         * @brief 提取CPU个数
         */
        int32_t ExtractCpuNum();

        /**
         * @brief 提取CPU HZ频率
         */
        int32_t ExtractCpuHz();

        /**
         * @brief 提取CPU个数, CPU HZ频率
         */            
        int32_t ExtractCpuInfo();            

    private:

        uint32_t        _cpu_num;               // cpu 数目
        uint32_t        _cpu_mhz;               // cpu mhz
        uint32_t        _cpu_idle;              // cpu 空闲比例
        uint32_t        _mem_total;             // 内存总数 KB 
        uint32_t        _mem_free;              // 内存可用 KB    
        cpu_stat_t      _records[2];            // cpu统计值, 2次取差值
    };

    // 进程cpu数据定义
    typedef struct _proc_cpu
    {
        uint64_t        _utime;             // 用户态时间
        uint64_t        _stime;             // 系统态时间
        uint64_t        _cutime;            // 子线程用户态时间
        uint64_t        _cstime;            // 子线程系统态时间
        uint64_t        _proc_total;        // 进程汇总时间
        uint64_t        _sys_total;         // 系统切片时间
    }proc_cpu_t;


    /**
     * @brief 进程的资源信息定义
     */
    class CProcInfo
    {
    public:

        CProcInfo(uint32_t pid) {
            _pid = pid;
            this->Init();
        };
        ~CProcInfo() {};

        /**
         * @brief 初始化获取信息
         */
        void Init();

        /**
         * @brief 更新获取信息
         */
        void Update();

        /**
         * @brief 获取内存使用值
         */
        uint64_t GetMemUsed() {
            return _mem_used;
        };

        /**
         * @brief 获取内存使用值, 多cpu还需*cpu数目
         */
        uint32_t GetCpuUsed() {
            return _cpu_used;
        };

        /**
         * @brief 获取pid
         */
        uint32_t GetPid()   {
            return _pid;
        };

        /**
         * @brief 计算CPU空闲比例 * 100, 还未考虑cpu数目
         */            
        void CalcCpuUsed();

        /**
         * @brief 分割字符串, 取第N个切片以后的数据
         * @param left_str - 返回第N+1开始的字符串
         * @return 第N个切片
         */
        static char* SkipToken(char* src, const char* token, uint32_t num, char** left_str);

        /**
         * @brief 获取指定pid的进程cpu占用
         */
        static int32_t ExtractProcCpu(uint32_t pid, proc_cpu_t& stat);

        /**
         * @brief 获取指定pid的进程mem占用
         */
        static uint64_t ExtractProcMem(uint32_t pid);   

    private:
        
        uint32_t        _pid;               // 进程pid
        uint64_t        _mem_used;          // 物理内存kb
        uint32_t        _cpu_used;          // 比例相对单CPU比例
        proc_cpu_t      _result[2];         // 快照信息
        
    };

    typedef std::map<uint32_t, CProcInfo*>  proc_map_t;


};


#endif

