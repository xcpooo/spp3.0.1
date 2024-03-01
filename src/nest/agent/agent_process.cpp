/**
 * @file nest_process.cpp
 * @info 进程管理部分
 */
#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "misc.h"
#include "agent_process.h"
#include "agent_thread.h"
#include "nest_commu.h"
#include "nest_log.h"
#include "nest_agent.h"


using namespace nest;

/**
 * @brief 构造函数
 */
TNestProc::TNestProc()
{
    _load_pos  = 0;
    _stat_pos  = 0;
    _heartbeat = 0;
    memset(_load, 0, sizeof(_load));
    memset(_stat, 0, sizeof(_stat));
}

///< 获取最新的业务负载
StatUnit& TNestProc::GetCurrStat()
{
    return _stat[_stat_pos % MAX_STAT_NUM];
}


/**
 * @brief 获取最新的统计数据
 */
StatUnit& TNestProc::GetLastStat()
{
    uint32_t lst_idx = (_stat_pos > 0) ? (_stat_pos - 1) : (MAX_STAT_NUM - 1);
    return _stat[lst_idx % MAX_STAT_NUM];
}

/**
 * @brief 更新最新的统计数据
 */
void TNestProc::UpdateStat(StatUnit& stat)
{
    StatUnit& last = this->GetCurrStat();
    stat._stat_time = stat._stat_time / 60 * 60;  ///<  分钟边界对齐
    if (last._stat_time != stat._stat_time)
    {
        _stat_pos++;
        _stat[_stat_pos % MAX_STAT_NUM] = stat;
    }
    else
    {
        last._total_req   += stat._total_req;
        last._total_cost  += stat._total_cost;
        last._total_rsp   += stat._total_rsp;
        if (last._max_cost < stat._max_cost) {
            last._max_cost = stat._max_cost;
        }
        if (last._min_cost > stat._min_cost) {
            last._min_cost = stat._min_cost;
        }
    }
}


/**
 * @brief 获取最新的负载数据
 */
LoadUnit& TNestProc::GetLastLoad()
{
    return _load[_load_pos % MAX_LOAD_NUM];
}

/**
 * @brief 更新最新的负载数据
 */
void TNestProc::SetLoad(LoadUnit& load)
{
    _load_pos++;
    _load[_load_pos % MAX_LOAD_NUM] = load;
}



// 构造函数
CPkgProcMap::CPkgProcMap(string& pkg_name, string& service)
{
    _pkg_name       = pkg_name;
    _service_name   = service;
    _change_flag    = 1;       // 初始需要写入文件    
}


// 析构函数
CPkgProcMap::~CPkgProcMap()
{
    for (TGroupMap::iterator it = _group_map.begin(); it != _group_map.end(); ++it)
    {
        delete it->second;     // 不持有proc的句柄, 仅管理pid, 不用深度清理
    }
    _group_map.clear();
}

// 管理pid与路径映射关系
void CPkgProcMap::add_pid_proc(TNestProc* proc)
{
    TPidMap* pid_map = NULL;
    uint32_t group_id = proc->GetProcConf().proc_type();
    TGroupMap::iterator it = _group_map.find(group_id);
    if (it == _group_map.end())
    {
        pid_map = new TPidMap;
        if (!pid_map) {
            NEST_LOG(LOG_ERROR, "alloc memory failed, log");
            return;
        }
        _group_map[group_id] = pid_map;
    }
    else
    {
        pid_map = it->second;
        if (!pid_map) {
            NEST_LOG(LOG_ERROR, "pid map null, error");
            return;
        }
    }

    uint32_t pid = proc->GetProcConf().proc_pid();
    (*pid_map)[pid] = proc;
    _change_flag = 1;       // 更新完毕, 开启标记  
}

// 删除pid与路径映射关系
void CPkgProcMap::del_pid_proc(TNestProc* proc)
{
    TPidMap* pid_map = NULL;
    uint32_t group_id = proc->GetProcConf().proc_type();
    TGroupMap::iterator it = _group_map.find(group_id);
    if (it == _group_map.end()) {
        return;
    }
    
    pid_map = it->second;
    if (!pid_map) {
        return;
    }
    pid_map->erase(proc->GetProcConf().proc_pid());
    _change_flag = 1;       // 更新完毕, 开启标记  
}


// 检查pid列表是否变更
bool CPkgProcMap::check_pids_change()
{
    return (_change_flag != 0);
}

// 生成pid列表的字符串
void CPkgProcMap::dump_pids_string(string& pids)
{
    static char buf[2048*50]; // 最大5000个进程
    int32_t len = (int32_t)sizeof(buf);
    int32_t used = 0;

    buf[0] = '\0';
    for (TGroupMap::iterator it = _group_map.begin(); it != _group_map.end(); ++it)
    {
        used += snprintf(buf + used, len - used, "\nGROUPID[%d]\n", it->first);
        if (used >= len) {
            break;
        }
        TPidMap* pid_map = it->second;
        if (!pid_map) {
            continue;
        }
        
        for (TPidMap::iterator iter = pid_map->begin(); iter != pid_map->end(); ++iter)
        {
            used += snprintf(buf + used, len - used, "%d\n", iter->first);
            if (used >= len) {
                break;
            }
        }
    }
    
    pids = buf;
}

// 获取pid个数, 无进程可停止dc运行
uint32_t CPkgProcMap::get_pid_count()
{
    uint32_t count = 0;
    
    for (TGroupMap::iterator it = _group_map.begin(); it != _group_map.end(); ++it)
    {
        TPidMap* pid_map = it->second;
        if (!pid_map) {
            continue;
        }
        
        for (TPidMap::iterator iter = pid_map->begin(); iter != pid_map->end(); ++iter)
        {
            count++;   
        }
    }
    
    return count;
}


// 将pid列表信息, 写入pid文件
void CPkgProcMap::write_pids_file()
{
    // 1. 组合pid文件的路径名
    char pid_file[1024];
    snprintf(pid_file, sizeof(pid_file), NEST_BIN_PATH"%s/bin/spp.pid", _pkg_name.c_str()); 
    string pids_info;
    dump_pids_string(pids_info);

    // 2. 打开文件描述符
    int32_t pid_file_fd = open(pid_file, O_WRONLY | O_CREAT, 0666);
    if (pid_file_fd == -1)
    {
        NEST_LOG(LOG_ERROR, "open pid file %s failed, log(%m)", pid_file);
        return;
    }
    fcntl(pid_file_fd, F_SETFD, FD_CLOEXEC);

    // 3. 加锁写入文件信息
    flock(pid_file_fd, LOCK_EX);
    ftruncate(pid_file_fd, 0);
    lseek(pid_file_fd, 0, SEEK_SET);
    write(pid_file_fd, pids_info.data(), pids_info.size());
    flock(pid_file_fd, LOCK_UN);

    // 4. 关闭文件
    close(pid_file_fd);
    _change_flag = 0;       // 更新完毕, 重置开关   
}


// 检查report工具是否存在
int32_t CPkgProcMap::check_report_tool()
{
    // 1. 组合pid文件的路径名
    char buff[1024];
    snprintf(buff, sizeof(buff), NEST_BIN_PATH"%s/bin/dc.pid", _pkg_name.c_str()); 
    FILE* file = fopen(buff, "r");
    if (file == NULL) {
        NEST_LOG(LOG_DEBUG, "pid file not exist, log");
        return -1;
    }

    // 2. 加锁获取pid
    flock(fileno(file), LOCK_EX);

    size_t len = 0;
    char* line = NULL;
    int32_t pid = 0;
    if (getline(&line, &len, file) != -1)
    {
        // line = "xxx [DC_REPORT]"
        char* pos = strstr(line, "[DC_REPORT]");
        char pidstr[16] = {0};
        if (pos != NULL)
        {
            strncpy(pidstr, line, pos - line - 1);
            pid = atoi(pidstr);
        }
    }
    if (line) free(line);

    flock(fileno(file), LOCK_UN);
    fclose(file);

    // 3. 确认pid是否存在
    char exec_path[512] = {0};
    snprintf(buff, sizeof(buff), "/proc/%d/exe", pid); 
    if (readlink(buff, exec_path, sizeof(exec_path)) < 0)
    {
        NEST_LOG(LOG_DEBUG, "exec file[%s] read failed, not exist", buff);
        return -2;
    }

    // 4. 确认是否为DC-TOOL
    if (strcmp(basename(exec_path), NEST_DC_TOOL) != 0)
    {
        NEST_LOG(LOG_DEBUG, "exec file[%s] not dc, not exist", buff);
        return -3;
    }

    // 5. 确认存在, 返回PID
    return pid;
}

// 确保日志文件存在, 防止dc启动失败
void CPkgProcMap::touch_log_file()
{
    char log_file[512];
    snprintf(log_file, sizeof(log_file), NEST_BIN_PATH"%s"NEST_CTRL_lOG, _pkg_name.c_str()); 

	FILE* file_ptr = fopen(log_file, "a+");
	if (NULL == file_ptr)
	{
        NEST_LOG(LOG_ERROR, "Open file(%s) failed", log_file);
		return;
	}
	
	fclose(file_ptr);
	return;
}


// 启动report工具运行
bool CPkgProcMap::start_report_tool()
{
    // 1. 组合pid文件的路径名
    char bin_file[512], cmd[512];
    snprintf(bin_file, sizeof(bin_file), NEST_BIN_PATH"%s/bin/spp_%s_dc_tool", 
             _pkg_name.c_str(), _service_name.c_str());
    snprintf(cmd, sizeof(cmd), "cd "NEST_BIN_PATH"%s/bin/;./spp_%s_dc_tool -d > /dev/null 2>&1",
             _pkg_name.c_str(), _service_name.c_str());

    // 2. 检查文件是否存在
    if (access(bin_file, X_OK) < 0)
    {
        NEST_LOG(LOG_ERROR, "dc file %s, not exist or can't exec", bin_file);
        return false;
    }
    
    // 3. 执行启动
    NEST_LOG(LOG_DEBUG, "start dc tool, cmd: %s", cmd);

    this->touch_log_file(); // 依赖文件检查
    
    system(cmd);

    return true;
}

// 停止report工具运行
void  CPkgProcMap::stop_report_tool()
{
    // 1. 检查工具是否存在
    int32_t pid = check_report_tool();
    if (pid <= 0) {
        return;
    }
    NEST_LOG(LOG_ERROR, "stop dc tool, pid: %d", pid);

    // 2. 杀死工具进程
    kill(pid, 10);      // USR1 可被感知
    usleep(1);
    kill(pid, 9);
}


// 更新进程重启的异常记录
void CPkgProcMap::write_proc_dead_log(uint32_t group, uint32_t pid)
{
    // 1. 组合死机文件的路径名与死机信息字符串
    char log_file[256], log_info[256];
    snprintf(log_file, sizeof(log_file), NEST_BIN_PATH"%s"NEST_CTRL_lOG, _pkg_name.c_str()); 
    snprintf(log_info, sizeof(log_info), "group[%u] pid[%u] is dead, no heartbeat.\n", group, pid);

    // 2. 打开日志文件
    int32_t fd = open(log_file, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
    {
        NEST_LOG(LOG_ERROR, "open log file %s failed, log(%m)", log_file);
        return;
    }
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    
    struct stat st = {0};
    if (fstat(fd, &st) < 0)
    {
        NEST_LOG(LOG_ERROR, "fstat log file %s failed, log(%m)", log_file);
        close(fd);
        return;
    }

    // 3. 锁定并写入日志
    flock(fd, LOCK_EX);
    if (st.st_size > 100*1024*1024) {
        ftruncate(fd, 0);
    }
    lseek(fd, 0, SEEK_END);
    write(fd, log_info, strlen(log_info));
    flock(fd, LOCK_UN);
    close(fd);

    return;
}


// 析构函数, 清理资源
CPkgMng::~CPkgMng()
{
    for (TPkgMap::iterator it = _pkg_map.begin(); it != _pkg_map.end(); ++it)
    {
        delete it->second;
    }
    _pkg_map.clear();
}

// 包管理添加进程
void CPkgMng::add_proc_pkg(TNestProc* proc)
{
    CPkgProcMap* pkg_procs = NULL;
    string& pkg_name = proc->GetPackageName();
    string& service_name = proc->GetServiceName();
    TPkgMap::iterator it = _pkg_map.find(pkg_name);
    if (it != _pkg_map.end())
    {
        pkg_procs = it->second;    
    }
    else
    {
        pkg_procs = new CPkgProcMap(pkg_name, service_name);
        if (!pkg_procs) {
            NEST_LOG(LOG_ERROR, "alloc memory failed, log");
            return;
        }
        _pkg_map[pkg_name] = pkg_procs;
    }

    pkg_procs->add_pid_proc(proc);

    // 按包管理, 实时CGROUP隔离控制 20140625
    CAgentCGroups& cgroups = CAgentProxy::Instance()->GetCGroupHandle();
    cgroups.service_group_add_task(service_name, proc->GetProcPid());
    
}

// 包管理删除进程
void CPkgMng::del_proc_pkg(TNestProc* proc)
{
    CPkgProcMap* pkg_procs = NULL;
    string& pkg_name = proc->GetPackageName();
    TPkgMap::iterator it = _pkg_map.find(pkg_name);
    if (it != _pkg_map.end())
    {
        pkg_procs = it->second; 
    }

    if (pkg_procs) {
        pkg_procs->del_pid_proc(proc);
    }
}

// 进程死机日志
void CPkgMng::write_dead_log(TNestProc* proc)
{
    CPkgProcMap* pkg_procs = NULL;
    string& pkg_name = proc->GetPackageName();
    TPkgMap::iterator it = _pkg_map.find(pkg_name);
    if (it != _pkg_map.end())
    {
        pkg_procs = it->second; 
    }

    if (pkg_procs) {
        pkg_procs->write_proc_dead_log(proc->GetProcConf().proc_type(), proc->GetProcPid());
    }
}


// 检查pid更新与上报工具的运行
void CPkgMng::check_run()
{
    // 1. 遍历安装包对象
    CPkgProcMap* pkg_procs = NULL;
    for (TPkgMap::iterator it = _pkg_map.begin(); it != _pkg_map.end(); ++it)
    {
        pkg_procs = it->second;
        if (!pkg_procs) {
            NEST_LOG(LOG_ERROR, "pkg maps null, log");
            continue;
        }

        // 2. 检查并dump pid文件
        if (pkg_procs->check_pids_change())
        {
            pkg_procs->write_pids_file();
        }

        // 3. 如果无进程则杀死dc, 否则检查拉起DC
        uint32_t pid_count = pkg_procs->get_pid_count();
        if (!pid_count) 
        {
            pkg_procs->stop_report_tool();    
        } 
        else 
        {
            if (pkg_procs->check_report_tool() < 0) 
            {
                pkg_procs->start_report_tool();
            }
        }
    }
}


/** 
 * @brief 本地进程的全局管理
 */
TNestProcMng* TNestProcMng::_instance = NULL;
TNestProcMng* TNestProcMng::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new TNestProcMng;
    }

    return _instance;
}

/**
 * @brief 进程管理全局的销毁接口
 */
void TNestProcMng::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}


/** 
 * @brief 析构函数
 */
TNestProcMng::~TNestProcMng()
{
    TProcMap::iterator it;
    for (it = _pno_map.begin(); it != _pno_map.end(); ++it)
    {
        delete it->second;
    }
    _pno_map.clear();
    _pid_map.clear();
}

/** 
 * @brief 添加进程接口
 */
int32_t TNestProcMng::AddProc(TNestProc* proc)
{
    TProcKey key;
    key._service_id = proc->GetProcConf().service_id();
    key._proc_type  = proc->GetProcConf().proc_type();
    key._proc_no    = proc->GetProcConf().proc_no();

    TNestProc* proc_find = this->FindProc(key);
    if (proc_find != NULL)
    {
        this->DelProc(proc_find);
    }

    uint32_t pid = proc->GetProcConf().proc_pid();
    _pno_map.insert(TProcMap::value_type(key, proc));   // 保证成功
    _pid_map[pid] = proc;

    _pkg_mng.add_proc_pkg(proc);    // 包管理增加进程

    return 0;
}

/** 
 * @brief 删除进程接口
 */
void TNestProcMng::DelProc(TNestProc* proc)
{
    TNestProc* proc_del = NULL;
    
    TProcKey key;
    key._service_id = proc->GetProcConf().service_id();
    key._proc_type  = proc->GetProcConf().proc_type();
    key._proc_no    = proc->GetProcConf().proc_no();

    TProcMap::iterator it = _pno_map.find(key);
    if (it != _pno_map.end())
    {
        proc_del = it->second;
        _pno_map.erase(it);

        if (proc_del) {
            uint32_t pid = proc_del->GetProcPid();
            _pid_map.erase(pid);
            _pkg_mng.del_proc_pkg(proc_del); // 包管理删除进程
        }
        
        delete proc_del;
    }
}

/** 
 * @brief 查询进程接口
 */
TNestProc* TNestProcMng::FindProc(TProcKey& key)
{
    TProcMap::iterator it = _pno_map.find(key);
    if (it != _pno_map.end())
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
}


/** 
 * @brief 查询进程接口
 */
TNestProc* TNestProcMng::FindProc(uint32_t pid)
{
    TPidMap::iterator it = _pid_map.find(pid);
    if (it != _pid_map.end())
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
}


/**
 * @brief 重启进程, 更新进程pid
 */
void TNestProcMng::UpdateProcPid(TNestProc* proc, uint32_t pid)
{
    TProcKey key;
    key._service_id = proc->GetProcConf().service_id();
    key._proc_type  = proc->GetProcConf().proc_type();
    key._proc_no    = proc->GetProcConf().proc_no();
    TNestProc* proc_find = this->FindProc(key);
    if (!proc_find)
    {
        return;    
    }

    // 1. 删除旧的映射关系
    _pid_map.erase(proc_find->GetProcPid());
    _pkg_mng.del_proc_pkg(proc_find); // 包管理删除进程

    // 2. 更新pid信息
    proc_find->SetProcPid(pid);
    
    // 3. 建立新的映射
    _pid_map[pid] = proc_find;
    _pkg_mng.add_proc_pkg(proc_find); // 包管理重添加进程
    
}

/** 
 * @brief 获取有效的pid信息列表
 */
void TNestProcMng::GetPidList(std::vector<uint32_t>& pid_list)
{
    TPidMap::iterator it;
    for (it = _pid_map.begin(); it != _pid_map.end(); ++it)
    {
        pid_list.push_back(it->first);
    }
}

/** 
 * @brief 获取指定service name的业务进程列表
 */
void TNestProcMng::GetServiceProc(const string& service, TProcList& proc_list)
{
    TProcMap::iterator it;
    for (it = _pno_map.begin(); it != _pno_map.end(); ++it)
    {
        TNestProc* proc = it->second;
        if (!proc) {
            continue;
        }

        if (proc->GetServiceName() == service)
        {
            proc_list.push_back(proc);
        }
    }
}

/**
 * @brief 检查进程是否心跳超时
 */
void TNestProcMng::CheckHeartbeat(uint64_t now)
{
    TProcMap::iterator it;
    for (it = _pno_map.begin(); it != _pno_map.end(); ++it)
    {
        TNestProc* proc = it->second;
        if (!proc) {
            continue;
        }

        if (proc->GetHeartbeat() + 60 * 1000 < now)
        {
            NEST_LOG(LOG_ERROR, "PROC[%u][%u][%u] pid[%u] down",
                    proc->GetProcConf().service_id(), proc->GetProcConf().proc_type(),
                    proc->GetProcConf().proc_no(), proc->GetProcPid());
            _pkg_mng.write_dead_log(proc);  // 适配死机日志文件
                    
            _pid_map.erase(proc->GetProcPid()); // 冗余逻辑
            this->RestartProc(proc);            
            proc->UpdateHeartbeat(now);
            continue;
        }
    }
}

/**
 * @brief 重启业务进程
 */
void TNestProcMng::RestartProc(TNestProc* proc)
{
    CThreadAddProc* thread = new CThreadAddProc;
    if (!thread)
    {
        return;
    }

    nest_sched_add_proc_req req;
    req.set_service_name(proc->GetServiceName());
    req.set_package_name(proc->GetPackageName());
    req.set_service_id(proc->GetProcConf().service_id());
    req.set_proc_type(proc->GetProcConf().proc_type());
    nest_proc_base* conf = req.add_proc_conf();
    *conf = proc->GetProcConf();
    
    thread->SetRestartCtx(&req);
    thread->Start();
}

/**
 * @brief 获取可用的port信息
 */
uint32_t TNestProcMng::GetUnusedPort(TProcKey& proc)
{
    // 1. 尝试按sid-pno来获取 port
    uint64_t seed = ((uint64_t)proc._service_id) << 32;
    uint16_t port = ((seed + proc._proc_no) % NEST_PROXY_PRIMER) + NEST_PROXY_BASE_PORT;
    TPortMap::iterator it = _port_map.find(port);
    if (it == _port_map.end())
    {
        _port_map[port] = proc;
        return port;
    }

    // 2. 重复申请, 直接使用
    TProcKey& item = it->second;
    if (item == proc)
    {
        return port;
    }

    // 3. 遍历查询重复的节点, 重用port, 防止重复分配
    for (it = _port_map.begin(); it != _port_map.end(); ++it)
    {
        if (it->second == proc)
        {
            NEST_LOG(LOG_DEBUG, "port[%u] map find reuse ok, proc[%u-%u]",
                it->first, proc._service_id, proc._proc_no);
            return it->first;
        }
    }
    
    // 4. 遍历并占用port, 避开首尾段, 分配占用
    for (port = NEST_PROXY_BASE_PORT + 1000; port < NEST_PROXY_BASE_PORT + 3000; port++)
    {
        if (_port_map.find(port) == _port_map.end())
        {
            _port_map[port] = proc;
            return port;
        }
    }

    // 5. 返回失败 0
    return 0;
}


/**
 * @brief 释放分配的port信息, 容错处理
 */
void TNestProcMng::FreeUsedPort(TProcKey& proc, uint32_t port)
{
    // 1. 已知PORT的时候, 校验并删除
    TPortMap::iterator it = _port_map.find(port);
    if (it != _port_map.end())
    {
        TProcKey& item = it->second;
        if (item == proc)
        {
            _port_map.erase(it);
            return;
        }
        NEST_LOG(LOG_ERROR, "port[%u] map error, proc[%u-%u], no match item [%u-%u]",
            port, proc._service_id, proc._proc_no, item._service_id, item._proc_no);
    }

    // 2. 没有port, 或者port与proc不匹配, 遍历删除
    for (it = _port_map.begin(); it != _port_map.end(); ++it)
    {
        if (it->second == proc)
        {
            NEST_LOG(LOG_DEBUG, "port[%u] map find del, proc[%u-%u]",
                it->first, proc._service_id, proc._proc_no);
            _port_map.erase(it);
            return;
        }
    }

    // 3. 无数据返回
    return;
}


/**
 * @brief 更新占用port信息
 */
void TNestProcMng::UpdateUsedPort(TProcKey& proc, uint32_t port)
{
    // 1. 根据PORT的信息, 校验并更新log
    TPortMap::iterator it = _port_map.find(port);
    if (it != _port_map.end())
    {
        TProcKey& item = it->second;
        if (!(item == proc)) // TODO 临时用等于判定
        {
            NEST_LOG(LOG_ERROR, "port[%u] map error, proc[%u-%u], no match item [%u-%u]",
                port, proc._service_id, proc._proc_no, item._service_id, item._proc_no);
            _port_map.erase(it);
        }
    }

    // 2. 覆盖更新, 心跳信息, 标志proxy已经占用该port, 可以强制更新
    _port_map[port] = proc;
    
}

/**
 * @brief 尝试注册port, 冲突直接返回失败
 */
bool TNestProcMng::RegistProxyPort(TProcKey& proc, uint32_t port)
{
    // 1. 根据PORT的信息, 检测是否有注册
    TPortMap::iterator it = _port_map.find(port);
    if (it != _port_map.end())
    {
        TProcKey& item = it->second;
        NEST_LOG(LOG_ERROR, "port[%u] used already, proc[%u-%u]",
            port, item._service_id, item._proc_no);
        return false;
    }

    // 2. 设置新的端口与进程映射, 返回成功
    _port_map[port] = proc;

    return true;
}


CServiceMng* CServiceMng::_instance = NULL;  // 单例指针

/// 构造函数
CServiceMng::CServiceMng()
{
    pthread_mutex_init(&_lock,NULL);
}

/// 获取单例
CServiceMng* CServiceMng::Instance()
{
    if (!_instance)
    {
        _instance = new CServiceMng;
    }

    return _instance;
}

/// 更新业务类型字段
void CServiceMng::UpdateType(std::string& name, uint32_t type)
{
    SrvMap::iterator it;

    pthread_mutex_lock(&_lock);
    it = _srv_map.find(name);
    if (it != _srv_map.end())
    {
        ServiceInfo &info = it->second;
        if (info.type != type)
        {
            info.type         = type;
            info.report_times = 0;
        }

        pthread_mutex_unlock(&_lock);
        return;
    }

    ServiceInfo info;
    info.name = name;
    info.type = type;
    info.report_times = 0;

    _srv_map.insert(SrvMap::value_type(name, info));

    pthread_mutex_unlock(&_lock);
    return;
}

/// 生成pb协议，用于上报调度中心
void CServiceMng::GenPbReport(nest_sched_load_report_req &proto)
{
    SrvMap::iterator it;

    pthread_mutex_lock(&_lock);
    for (it = _srv_map.begin(); it != _srv_map.end(); it++)
    {
        const string&  name = it->first;
        const uint32_t type = it->second.type;

        if (it->second.report_times >= 10)
        {
            continue;
        }

        it->second.report_times++;

        nest_service_type *srv_type = proto.add_srv_type();
        srv_type->set_name(name);
        srv_type->set_type(type);
    }

    pthread_mutex_unlock(&_lock);
}



