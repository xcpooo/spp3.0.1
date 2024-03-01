/**
 * @filename sys_info.cpp
 * @info 获取系统的与进程的CPU与负载信息
 */

#include "nest_log.h"       // for log
#include "sys_info.h"
#include <sys/types.h>
#include <unistd.h>

using namespace nest;


/**
 * @brief 构造函数
 */
CSysInfo::CSysInfo()
{
    _cpu_num        = 0;
    _cpu_mhz        = 0;
    _cpu_idle       = 0;
    _mem_total      = 0;
    _mem_free       = 0;
    memset(_records, 0, sizeof(_records));

    this->Init();
}

/**
 * @brief 初始化获取信息
 */
void CSysInfo::Init()
{
    // 1. 获取cpu的数目与mhz
    ExtractCpuInfo();

    // 2. 获取CPU占用快照
    ExtractCpuStat(_records[0]);
    ExtractCpuStat(_records[1]);

    // 3. 获取内存信息
    CalcMemFree();
}

/**
 * @brief 更新获取信息
 */
void CSysInfo::Update()
{
    // 1. 计算cpu信息
    CalcCpuIdle();

    // 2. 计算内存信息
    CalcMemFree();
}

/**
 * @brief 提取内存统计信息
 */   
int32_t CSysInfo::ExtractMemInfo(mem_stat_t& stat)
{
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        NEST_LOG(LOG_ERROR, "open /proc/meminfo failed!");
        return -1;
    }
    
    char name[256];
    char line[256];
    int32_t finish_num = 0;
    unsigned long long value;

    while (fgets(line, sizeof(line)-1, fp))
    {
        if (sscanf(line, "%s%llu", name, &value) != 2) 
        {
            continue;
        }

        if (!strcmp(name, "MemTotal:"))
        {
            finish_num++;
            stat._mem_total = (uint64_t)value;
        }
        else if (!strcmp(name, "MemFree:"))
        {
            finish_num++;
            stat._mem_free = (uint64_t)value;
        }
        else if (!strcmp(name, "Buffers:"))
        {
            finish_num++;
            stat._buffers = (uint64_t)value;
        }
        else if (!strcmp(name, "Cached:"))
        {
            finish_num++;
            stat._cached = (uint64_t)value;
        }
        /*
        else if (! strcmp(name, "Inactive:"))  // todo
        {
            finish_num++;
            stat._inactive_file = (uint64_t)value;
        }
        */
        else if (!strcmp(name, "Mapped:"))
        {
            finish_num++;
            stat._mapped = (uint64_t)value;
        }
    }

    fclose(fp);
    
    // 所有字段都提取到了么?
    if (finish_num != 5)
    {
        NEST_LOG(LOG_ERROR, "extract /proc/meminfo failed!");
        return -1;
    }

    return 0;

}

/**
 * @brief 提取CPU个数
 */
int32_t CSysInfo::ExtractCpuNum()
{
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        NEST_LOG(LOG_ERROR, "open /proc/stat failed!");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line)-1, fp))
    {
        // cpu[0-9]
        if (strstr(line, "cpu"))
        {
            if (line[3] <= '9' && line[3] >= '0')
            {
                ++_cpu_num;
                continue;
            }
        }
        else // 只关注最前面几行带cpu的信息
        {
            break;
        }
    }

    if (!_cpu_num)
    {
        NEST_LOG(LOG_ERROR, "extract cpu num failed!");
        fclose(fp);
        return -2;
    }

    NEST_LOG(LOG_DEBUG, "extract cpu num, %u!", _cpu_num);

    fclose(fp);

    return 0;
}

/**
 * @brief 获取CPU个数
 */
uint32_t CSysInfo::GetCpuNums()
{
    static uint32_t cpu_num = 0;

    if (cpu_num)
    {
        return cpu_num;
    }

    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        NEST_LOG(LOG_ERROR, "open /proc/stat failed!");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line)-1, fp))
    {
        // cpu[0-9]
        if (strstr(line, "cpu"))
        {
            if (line[3] <= '9' && line[3] >= '0')
            {
                ++cpu_num;
                continue;
            }
        }
        else // 只关注最前面几行带cpu的信息
        {
            break;
        }
    }

    if (!cpu_num)
    {
        NEST_LOG(LOG_ERROR, "get cpu num failed!");
        fclose(fp);
        return 0;
    }

    NEST_LOG(LOG_DEBUG, "get cpu num, %u!", cpu_num);
    fclose(fp);

    return cpu_num;
}


/**
 * @brief 提取CPU HZ频率
 */
int32_t CSysInfo::ExtractCpuHz()
{
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        NEST_LOG(LOG_ERROR, "open /proc/cpuinfo failed!");
        return -1;
    }

    float cpu_mhz = 0;
    char line[256];
    while (fgets(line, sizeof(line)-1, fp))
    {
        // cpu MHz         : 2000.118
        if (!strstr(line, "MHz") || !strstr(line, ":"))
        {
            continue;
        }

        char tmp[64];        
        if (sscanf(line, "%s%s%s%f", tmp, tmp, tmp, &cpu_mhz) == 4) 
        {
            _cpu_mhz = (uint32_t)cpu_mhz;
            continue;
        }
    }

    fclose(fp);

    return 0;
}

/**
 * @brief 提取CPU个数, CPU HZ频率
 */            
int32_t CSysInfo::ExtractCpuInfo()
{
    int32_t ret = 0;

    ret = ExtractCpuNum();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "extract cpu num failed!");
        return -1;
    }

    ret = ExtractCpuHz();
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "extract cpu hz failed!");
        return -2;
    }
    
    return 0;
}

/**
 * @brief 提取CPU统计信息
 */            
void CSysInfo::ExtractCpuStat(cpu_stat_t& stat)
{
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        NEST_LOG(LOG_ERROR, "open /proc/stat failed!");
        return;
    }
    memset(&stat, 0, sizeof(stat));

    char name[64];
    char line[512];
    while (fgets(line, sizeof(line)-1, fp))
    {
        // cpu  155328925 640355 14677305 9174748668 331430975 0 457328 7242581
        if (sscanf(line, "%s%llu%llu%llu%llu%llu%llu%llu", name, 
                   (llu64_t*)&stat._user, (llu64_t*)&stat._nice,
                   (llu64_t*)&stat._system, (llu64_t*)&stat._idle,
                   (llu64_t*)&stat._iowait, (llu64_t*)&stat._irq,
                   (llu64_t*)&stat._softirq) != 8)
        {
            continue;
        }

        // 仅处理cpu平均值
        if (!strncmp(name, "cpu", 3) && (strlen(name) == 3))
        {
            stat._total = stat._user + stat._nice + stat._system + stat._idle 
                          + stat._iowait + stat._irq + stat._softirq;
            break;
        }
    }

    fclose(fp);

    return;
}

/**
 * @brief 计算CPU空闲比例 * 100
 */            
void CSysInfo::CalcCpuIdle()
{
    _records[0] = _records[1];
    this->ExtractCpuStat(_records[1]);

    if (!_records[1]._total || !_records[0]._total
        || !_records[0]._idle || !_records[1]._idle
        || (_records[1]._total <= _records[0]._total)
        || (_records[1]._idle < _records[0]._idle))
    {
        NEST_LOG(LOG_ERROR, "cpu data invalid, total: %llu, %llu, idle: %llu, %llu", 
            _records[1]._total, _records[0]._total,
            _records[1]._idle, _records[0]._idle);
            
        _cpu_idle = CPU_PER_PROCESSOR; // 无法采集, 默认初始空闲100%
        return;
    }
    
    uint64_t delta_total = _records[1]._total - _records[0]._total;
    uint64_t delta_idle  = _records[1]._idle - _records[0]._idle;

    _cpu_idle = (delta_idle) * CPU_PER_PROCESSOR / delta_total;

}

/**
 * @brief 获取内存空闲大小
 */            
void CSysInfo::CalcMemFree()
{
    mem_stat_t stat;
    int32_t ret = this->ExtractMemInfo(stat);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "mem data invalid");
        return;
    }
    _mem_total = stat._mem_total;
    _mem_free  = stat._mem_free + stat._cached - stat._mapped; // todo
}

/**
 * @brief 初始化获取信息
 */
void CProcInfo::Init()
{
    // 1. 获取内存信息
    _mem_used = this->ExtractProcMem(_pid);
    
    // 2. 获取cpu信息
    memset(&_result, 0, sizeof(_result));
    _cpu_used = 0;
    this->ExtractProcCpu(_pid, _result[0]);
    this->ExtractProcCpu(_pid, _result[1]);
}

/**
 * @brief 更新获取信息
 */
void CProcInfo::Update()
{
    // 1. 计算cpu信息
    CalcCpuUsed();

    // 2. 计算内存信息
    _mem_used = ExtractProcMem(_pid);
}


/**
 * @brief 分割字符串, 取第N个切片以后的数据
 */
char* CProcInfo::SkipToken(char* src, const char* token, uint32_t num, char** left_str)
{
    char* str = src;
    char* sub_str = NULL;
    char* save_ptr = src;
    
    for (uint32_t i = 0; i < num; i++, str = NULL)
    {
        sub_str = strtok_r(str, token, &save_ptr);
        if (sub_str == NULL) {
            break;
        }
    }

    if (left_str) {
        *left_str = save_ptr;
    }

    return sub_str;
}

/**
 * @brief 获取指定pid的进程cpu占用
 */
int32_t CProcInfo::ExtractProcCpu(uint32_t pid, proc_cpu_t& stat)
{
    char name[64];
    snprintf(name, sizeof(name), "/proc/%u/stat", pid);
    FILE* fp = fopen(name, "r");
    if (!fp) {
        NEST_LOG(LOG_ERROR, "open %s failed!", name);
        return -1;
    }

    char line[1024];
    if (!fgets(line, sizeof(line)-1, fp))
    {
        NEST_LOG(LOG_ERROR, "read %s failed!", name);
        fclose(fp);
        return -2;
    }
    fclose(fp);

    memset(&stat, 0, sizeof(stat));
    
    // pid name ... 14
    char* left_str = NULL;
    char* sub_str = CProcInfo::SkipToken(line, " ", 13, &left_str);
    if (!left_str) {
        NEST_LOG(LOG_ERROR, "file check %s failed, substr %p!", name, sub_str);
        return -3;
    }

    //  155328925 640355 0 0
    if (sscanf(left_str, "%llu%llu%llu%llu",
               (llu64_t*)&stat._utime, (llu64_t*)&stat._stime,
               (llu64_t*)&stat._cutime, (llu64_t*)&stat._cstime) != 4)
    {
        NEST_LOG(LOG_ERROR, "line check %s failed!", left_str);
        return -4;
    }

    //  计算total
    stat._proc_total = stat._utime + stat._stime + stat._cutime + stat._cstime;

    // 获取操作系统的当前切片值
    cpu_stat_t sys_cpu;
    CSysInfo::ExtractCpuStat(sys_cpu);
    stat._sys_total = sys_cpu._total;
    
    return 0;

}


/**
 * @brief 获取指定pid的进程mem占用
 */
uint64_t CProcInfo::ExtractProcMem(uint32_t pid)
{
    uint64_t used_mem = 0;
    char name[64];
    snprintf(name, sizeof(name), "/proc/%u/status", pid);
    FILE* fp = fopen(name, "r");
    if (!fp) {
        NEST_LOG(LOG_ERROR, "open %s failed!", name);
        return used_mem;
    }

    char line[1024];
    while (fgets(line, sizeof(line)-1, fp))
    {
        if (strncmp(line, "VmRSS", 5) != 0)
        {
            continue;
        }
        
        if (sscanf(line, "%s%llu", name, (llu64_t*)&used_mem) != 2)
        {
            NEST_LOG(LOG_ERROR, "match vmrss from %s failed!", line);
            break;
        }
    }
    fclose(fp);
    
    return used_mem;
}

/**
 * @brief 计算CPU空闲比例 * 100, 还未考虑cpu数目
 */            
void CProcInfo::CalcCpuUsed()
{
    _result[0] = _result[1];
    this->ExtractProcCpu(_pid, _result[1]);

    if (!_result[1]._sys_total || !_result[0]._sys_total
        // || !_result[0]._proc_total || !_result[1]._proc_total  // 进程启动一分钟, 无数据
        || (_result[1]._sys_total <= _result[0]._sys_total)
        || (_result[1]._proc_total < _result[0]._proc_total))
    {
        NEST_LOG(LOG_ERROR, "cpu data invalid, sys total: %llu, %llu, proc: %llu, %llu", 
            _result[1]._sys_total, _result[0]._sys_total,
            _result[1]._proc_total, _result[0]._proc_total);
        return;
    }
    
    uint64_t delta_total = _result[1]._sys_total - _result[0]._sys_total;
    uint64_t delta_proc  = _result[1]._proc_total - _result[0]._proc_total;

    _cpu_used = (delta_proc) * CPU_PER_PROCESSOR / delta_total;

}


