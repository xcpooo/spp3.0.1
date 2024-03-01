/**
 * @filename sys_info.h
 * @info ��ȡϵͳ������̵�CPU�븺����Ϣ
 */

#ifndef _SYS_INFO_H__
#define _SYS_INFO_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <map>

namespace nest
{

    // �����궨��
    enum 
    {
        STD_CPU_MHZ         = 2000,
        CPU_PER_PROCESSOR   = 10000, // story #57564145 ���cpuͳ�ƽ��ȣ���100�޸�Ϊ10000
    };
    typedef unsigned long long   llu64_t;
        

    // CPU ʵʱ���սṹ����
    typedef struct _cpu_stat
    {
        uint64_t    _total;                 // �ܵļ���ֵ, ��λjiff 10ms
        uint64_t    _user;                  // �ۼƵ� �û�̬
        uint64_t    _nice;                  // �ۼƵ� niceΪ��
        uint64_t    _system;                // ϵͳ̬����ʱ��
        uint64_t    _idle;                  // IO�ȴ�����������ȴ�ʱ��
        uint64_t    _iowait;                // IO�ȴ�ʱ��    
        uint64_t    _irq;                   // Ӳ�ж�
        uint64_t    _softirq;               // ���ж�
        uint64_t    _stealstolen;           // ���������������ϵͳ
        uint64_t    _guest;                 // ���⻯cpu��
    }cpu_stat_t;

    // �ڴ����ݽṹ����
    typedef struct _mem_stat
    {
        uint64_t    _mem_total;             // �ܵ��ڴ� KB    
        uint64_t    _mem_free;              // ���������ڴ�    
        uint64_t    _buffers;               // buffer
        uint64_t    _cached;                // �ļ�cache��    
        uint64_t    _mapped;                // ��ӳ��
        uint64_t    _inactive_file;         // ����Ծ���ļ�cache

    }mem_stat_t;


    /**
     * @brief ȫ�ֵ�ϵͳ��Դ��Ϣ
     */
    class CSysInfo
    {
    public:

        /**
         * @brief ���캯������������
         */
        CSysInfo();
        ~CSysInfo() {};

        /**
         * @brief ��ʼ����ȡ��Ϣ
         */
        void Init();

        /**
         * @brief ���»�ȡ��Ϣ
         */
        void Update();

        /**
         * @brief ��ȡ��Ե�CPU��ֵ, 2Gÿ����100
         */        
        uint32_t GetCpuTotal() {
            return CPU_PER_PROCESSOR * _cpu_num * _cpu_mhz / STD_CPU_MHZ; 
        };
        
        /**
         * @brief ��ȡCPU�Ŀ���ֵ, ���б���*��ֵ
         */            
        uint32_t GetCpuIdle() {
            return _cpu_idle * _cpu_num * _cpu_mhz / STD_CPU_MHZ; ; 
        };
        uint32_t GetCpuIdleRatio() {
            return _cpu_idle;
        };

        /**
         * @brief ��ȡCPU�ĸ�����Ϣ
         */            
        uint32_t GetCpuUsed() {
            return GetCpuTotal() - GetCpuIdle();   
        };

        /**
         * @brief ��ȡ�ڴ�����
         */            
        uint32_t GetMemTotal() {
            return _mem_total;
        };

        /**
         * @brief ��ȡ�ڴ��ʹ����
         */            
        uint32_t GetMemFree() {
            return _mem_free;
        };

        /**
         * @brief ��ȡCPU��Ŀ
         */            
        uint32_t GetCpuNum() {
            return _cpu_num;
        };

        /**
         * @brief ��ȡCPUMHZ, �����cpuһ��
         */            
        uint32_t GetCpuMhz() {
            return _cpu_mhz;
        };
                
        /**
         * @brief ��ȡ�ڴ�ͳ����Ϣ
         */            
        static int32_t ExtractMemInfo(mem_stat_t& stat);        

        /**
         * @brief ��ȡCPUͳ����Ϣ
         */            
        static void ExtractCpuStat(cpu_stat_t& stat);

        /**
         * @brief  ��ȡCPU����
         */
        static uint32_t GetCpuNums();

        /**
         * @brief ����CPU���б��� * 100
         */            
        void CalcCpuIdle();

        /**
         * @brief ��ȡ�ڴ���д�С
         */            
        void CalcMemFree();

        /**
         * @brief ��ȡCPU����
         */
        int32_t ExtractCpuNum();

        /**
         * @brief ��ȡCPU HZƵ��
         */
        int32_t ExtractCpuHz();

        /**
         * @brief ��ȡCPU����, CPU HZƵ��
         */            
        int32_t ExtractCpuInfo();            

    private:

        uint32_t        _cpu_num;               // cpu ��Ŀ
        uint32_t        _cpu_mhz;               // cpu mhz
        uint32_t        _cpu_idle;              // cpu ���б���
        uint32_t        _mem_total;             // �ڴ����� KB 
        uint32_t        _mem_free;              // �ڴ���� KB    
        cpu_stat_t      _records[2];            // cpuͳ��ֵ, 2��ȡ��ֵ
    };

    // ����cpu���ݶ���
    typedef struct _proc_cpu
    {
        uint64_t        _utime;             // �û�̬ʱ��
        uint64_t        _stime;             // ϵͳ̬ʱ��
        uint64_t        _cutime;            // ���߳��û�̬ʱ��
        uint64_t        _cstime;            // ���߳�ϵͳ̬ʱ��
        uint64_t        _proc_total;        // ���̻���ʱ��
        uint64_t        _sys_total;         // ϵͳ��Ƭʱ��
    }proc_cpu_t;


    /**
     * @brief ���̵���Դ��Ϣ����
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
         * @brief ��ʼ����ȡ��Ϣ
         */
        void Init();

        /**
         * @brief ���»�ȡ��Ϣ
         */
        void Update();

        /**
         * @brief ��ȡ�ڴ�ʹ��ֵ
         */
        uint64_t GetMemUsed() {
            return _mem_used;
        };

        /**
         * @brief ��ȡ�ڴ�ʹ��ֵ, ��cpu����*cpu��Ŀ
         */
        uint32_t GetCpuUsed() {
            return _cpu_used;
        };

        /**
         * @brief ��ȡpid
         */
        uint32_t GetPid()   {
            return _pid;
        };

        /**
         * @brief ����CPU���б��� * 100, ��δ����cpu��Ŀ
         */            
        void CalcCpuUsed();

        /**
         * @brief �ָ��ַ���, ȡ��N����Ƭ�Ժ������
         * @param left_str - ���ص�N+1��ʼ���ַ���
         * @return ��N����Ƭ
         */
        static char* SkipToken(char* src, const char* token, uint32_t num, char** left_str);

        /**
         * @brief ��ȡָ��pid�Ľ���cpuռ��
         */
        static int32_t ExtractProcCpu(uint32_t pid, proc_cpu_t& stat);

        /**
         * @brief ��ȡָ��pid�Ľ���memռ��
         */
        static uint64_t ExtractProcMem(uint32_t pid);   

    private:
        
        uint32_t        _pid;               // ����pid
        uint64_t        _mem_used;          // �����ڴ�kb
        uint32_t        _cpu_used;          // ������Ե�CPU����
        proc_cpu_t      _result[2];         // ������Ϣ
        
    };

    typedef std::map<uint32_t, CProcInfo*>  proc_map_t;


};


#endif

