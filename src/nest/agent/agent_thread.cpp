/**
 * @brief Nest agent thread process
 */
#include <stdio.h>
#include <unistd.h> 
#include <stdint.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sched.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include "nest_log.h"
#include "agent_thread.h"
#include "agent_net_mng.h"
#include "nest_agent.h"

using namespace nest;

// 进程clone时的参数传递结构体定义
struct TCloneArgs
{
    nest_proc_base  base;
    string          path;
    string          bin;
    string          etc;
    string          package;
    string          service;
};


/**
 * @brief 负载采集线程资源清理接口
 */
CThreadSysload::~CThreadSysload()
{
    this->CleanProcMap(_proc_map);
}

/**
 * @brief 负载采集线程入口函数
 */
void CThreadSysload::Run()
{
    // 1. 获取系统信息
    NEST_LOG(LOG_DEBUG, "Node cpu num[%u] mhz[%u] idle[%u]%, mem total[%u], free[%u]",
            _sys_info.GetCpuNum(), _sys_info.GetCpuMhz(), _sys_info.GetCpuIdleRatio(),
            _sys_info.GetMemTotal(), _sys_info.GetMemFree());
            
    usleep(1000*1000);

    // 2. 开始主循环
    while (true)
    {
        // 1. 更新pid对应的负载信息
        this->UpdateLoad(10*1000);
        
        // 2. 上报系统负载 获取最新的pid列表
        this->ReportSendRecv();

        // 3. 适当sleep让出cpu
        //usleep(10*1000*1000);
    }
}


/**
 * @brief 负载采集更新进程控制块
 */
void CThreadSysload::ExtractProc(vector<uint32_t>& pids)
{
    proc_map_t  procs;
    _proc_map.swap(procs);

    for (uint32_t i = 0; i < pids.size(); i++)
    {
        uint32_t pid = pids[i];
        proc_map_t::iterator it = procs.find(pid);
        if (it != procs.end())
        {
            _proc_map.insert(proc_map_t::value_type(pid, it->second));
            procs.erase(it);
        }
        else
        {
            CProcInfo* proc = new CProcInfo(pid);
            if (!proc)
            {
                NEST_LOG(LOG_ERROR, "alloc proc failed, no more memory, error!!");
                continue;
            }
            _proc_map.insert(proc_map_t::value_type(pid, proc));
        }
    }

    this->CleanProcMap(procs);
}

/**
 * @brief 负载采集清理进程控制块
 */
void CThreadSysload::CleanProcMap(proc_map_t& proc_map)
{
    proc_map_t::iterator it;
    for (it = proc_map.begin(); it != proc_map.end(); ++it)
    {
        delete it->second;
    }
    proc_map.clear();
}

/**
 * @brief 负载采集获取负载信息
 */
void CThreadSysload::UpdateLoad(uint32_t time_interval)
{
    uint32_t time_delay = 0;

    // 1. 计算每次负载数据更新的时间间隔
    if (_proc_map.size() != 0)
    {
        time_delay = time_interval / _proc_map.size();
    }
    else
    {
        time_delay = time_interval;
    }

    // 2. 更新全局的负载信息
    _sys_info.Update();

    // 3. 更新进程的负载信息
    proc_map_t::iterator it;
    for (it = _proc_map.begin(); it != _proc_map.end(); ++it)
    {
        CProcInfo* proc = it->second;
        if (proc) {
            proc->Update();
        }

        usleep(time_delay * 1000);
    }

    usleep(time_delay * 1000);
}

/**
 * @brief 负载采集上报负载组包
 */
int32_t CThreadSysload::PackReportPkg(void* buff, uint32_t buff_len)
{
    nest_msg_head  head;        
    nest_agent_sysload_req req_body;
    nest_agent_sysload_rsp rsp_body;

    head.set_msg_type(NEST_PROTO_TYPE_AGENT);
    head.set_sub_cmd(NEST_SCHED_CMD_LOAD_REPORT);
    
    req_body.set_cpu_num(_sys_info.GetCpuNum());
    req_body.set_cpu_total(_sys_info.GetCpuTotal());
    req_body.set_cpu_load(_sys_info.GetCpuUsed());
    req_body.set_mem_total(_sys_info.GetMemTotal());
    req_body.set_mem_used(_sys_info.GetMemTotal() - _sys_info.GetMemFree());

    for (proc_map_t::iterator it = _proc_map.begin(); it != _proc_map.end(); ++it)
    {
        nest_proc_stat* stat = req_body.add_stats();
        CProcInfo* proc = it->second;
        if (!proc) {
            NEST_LOG(LOG_ERROR, "proc null, pid[%u], error", it->first);
            continue;
        }

        // 进程cpu按标准MHZ折算处理
        stat->set_proc_pid(it->first);
        stat->set_cpu_load(proc->GetCpuUsed() * _sys_info.GetCpuNum() * _sys_info.GetCpuMhz() / STD_CPU_MHZ);
        stat->set_mem_used(proc->GetMemUsed());
    }

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed, error!");
        return -1;
    }

    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = buff;
    rsp_pkg.buff_len    = buff_len;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return -2;
    }

    return ret_len;        
}


/**
 * @brief 负载采集上报负载解包
 */
int32_t CThreadSysload::UnPackReportRsp(void* buff, uint32_t buff_len)
{
    nest_msg_head  head;        
    nest_agent_sysload_rsp rsp_body;

    string err_msg;
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = buff;
    pkg_info.buff_len  = buff_len;
    
    int32_t ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        NEST_LOG(LOG_ERROR, "rsp check failed, ret %d", ret);
        return -1;
    }

    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        NEST_LOG(LOG_ERROR, "rsp parse failed, error");
        return -2;
    } 

    //  上下文确认
    if ((head.msg_type() != NEST_PROTO_TYPE_AGENT)
        || (head.sub_cmd() != NEST_SCHED_CMD_LOAD_REPORT))
    {
        NEST_LOG(LOG_ERROR, "rsp cmd not match, error");
        return -3;
    }

    //  返回确认
    if (head.result() != 0)
    {
        NEST_LOG(LOG_ERROR, "rsp failed, result %u - %s", head.result(), head.err_msg().c_str());
        return -4;
    }

    //  更新进程信息
    vector<uint32_t> pid_list;
    for (int32_t i = 0; i < rsp_body.pids_size(); i++)
    {
        const uint32_t& pid = rsp_body.pids(i);
        pid_list.push_back(pid);
    }
    this->ExtractProc(pid_list);

    return 0;        
}


/**
 * @brief 负载上报执行函数
 */
int32_t CThreadSysload::ReportSendRecv()
{
    static char req_buff[2*1024*1024];  // only this thread used
    static char rsp_buff[1024*1024];
    uint32_t req_len = sizeof(req_buff);
    uint32_t rsp_len = sizeof(rsp_buff);

    int32_t pkg_len = this->PackReportPkg(req_buff, req_len);
    if (pkg_len < 0)
    {
        NEST_LOG(LOG_ERROR, "pack req msg failed, ret %d", pkg_len);
        return -1;
    }

    struct sockaddr_un uaddr = {0};
    uaddr.sun_family   = AF_UNIX;
    snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), NEST_AGENT_UNIX_PATH);
    TNestAddress address;
    address.SetAddress(&uaddr, true);

    int32_t ret = TAgentNetMng::Instance()->SendRecvLocal(&address, req_buff, 
                                pkg_len, rsp_buff, rsp_len, 200); // 200ms
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "send recv req msg failed, ret %d", ret);
        return -2;
    }

    ret = this->UnPackReportRsp(rsp_buff, rsp_len);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "unpack rsp msg failed, ret %d", ret);
        return -3;
    }
    
    return 0;
}


/**
 * @brief 启动进程运行的执行函数
 */
int32_t CThreadProcBase::PackInitReq(void* buff, uint32_t buff_len, nest_proc_base& proc)
{
    // 1. 组装INIT请求消息
    nest_msg_head  head;        
    nest_proc_init_req req_body;

    head.set_msg_type(NEST_PROTO_TYPE_PROC);
    head.set_sub_cmd(NEST_PROC_CMD_INIT_INFO);
    
    req_body.set_service_name(_service_name);
    req_body.set_package_name(_package_name);    
    nest_proc_base* proc_conf = req_body.mutable_proc();
    proc_conf->set_service_id(proc.service_id());
    proc_conf->set_proc_type(proc.proc_type());
    proc_conf->set_proc_no(proc.proc_no());
    proc_conf->set_proxy_proc_no(proc.proxy_proc_no());
    proc_conf->set_proc_pid(proc.proc_pid());
    proc_conf->set_proxy_ip(proc.proxy_ip());
    proc_conf->set_proxy_port(proc.proxy_port());

    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed, error!");
        return -1;
    }

    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = buff;
    rsp_pkg.buff_len    = buff_len;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return -2;
    }

    NEST_LOG(LOG_DEBUG, "ProcInit Msg detail: [%s][%s]", head.ShortDebugString().c_str(),
            req_body.ShortDebugString().c_str());

    return ret_len;
    
}

/**
 * @brief 进程启动应答解包
 */
int32_t CThreadProcBase::UnPackInitRsp(void* buff, uint32_t buff_len, nest_proc_base& proc, uint32_t& worker_type)
{
    nest_msg_head  head;        
    nest_proc_init_rsp rsp_body;

    string err_msg;
    NestPkgInfo pkg_info = {0};
    pkg_info.msg_buff  = buff;
    pkg_info.buff_len  = buff_len;
    
    int32_t ret = NestProtoCheck(&pkg_info, err_msg);
    if (ret <= 0) 
    {
        NEST_LOG(LOG_ERROR, "rsp check failed, ret %d", ret);
        return -1;
    }

    if (!head.ParseFromArray((char*)pkg_info.head_buff, pkg_info.head_len)
        || !rsp_body.ParseFromArray((char*)pkg_info.body_buff, pkg_info.body_len))
    {
        NEST_LOG(LOG_ERROR, "rsp parse failed, error");
        return -2;
    } 

    //  上下文确认
    if ((head.msg_type() != NEST_PROTO_TYPE_PROC)
        || (head.sub_cmd() != NEST_PROC_CMD_INIT_INFO))
    {
        NEST_LOG(LOG_ERROR, "rsp cmd not match, error");
        return -3;
    }

    //  返回确认
    if (head.result() != 0)
    {
        NEST_LOG(LOG_ERROR, "rsp failed, result %u - %s", head.result(), head.err_msg().c_str());
        return -4;
    }
    
    NEST_LOG(LOG_DEBUG, "Proc init rsp detail: [%s][%s]", head.ShortDebugString().c_str(),
            rsp_body.ShortDebugString().c_str());

    //  更新进程信息
    proc = rsp_body.proc();

    if (rsp_body.has_type())
    {
        worker_type = rsp_body.type();
    }

    return 0;        
}


/**
 * @brief 进程初始化执行函数
 */
int32_t CThreadProcBase::InitSendRecv(nest_proc_base& proc, uint32_t& worker_type)
{
    char req_buff[65535];       // all thread use it
    char rsp_buff[65535];
    uint32_t req_len = sizeof(req_buff);
    uint32_t rsp_len = sizeof(rsp_buff);

    int32_t pkg_len = this->PackInitReq(req_buff, req_len, proc);
    if (pkg_len < 0)
    {
        NEST_LOG(LOG_ERROR, "pack req msg failed, ret %d", pkg_len);
        return -1;
    }

    struct sockaddr_un uaddr = {0};
    uaddr.sun_family   = AF_UNIX;
    snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), NEST_PROC_UNIX_PATH"_%u", proc.proc_pid());
    TNestAddress address;
    address.SetAddress(&uaddr, true);

    int32_t ret = TAgentNetMng::Instance()->SendRecvLocal(&address, req_buff, 
                                pkg_len, rsp_buff, rsp_len, 30*1000); // 30秒启动时间
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "send recv req msg failed, ret %d", ret);
        return -2;
    }

    ret = this->UnPackInitRsp(rsp_buff, rsp_len, proc, worker_type);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "unpack rsp msg failed, ret %d", ret);
        return -3;
    }
    
    return 0;
}

/**
 * @brief 等待进程执行启动的时间, 等待到unix path ok
 */
void CThreadProcBase::CheckWaitStart(uint32_t pid, uint32_t wait_ms)
{
    bool exist = false;
    while (wait_ms > 0)
    {
        exist = TAgentNetMng::Instance()->CheckUnixSockPath(pid, true);
        if (exist)
        {
            break;
        }

        usleep(20*1000);
        wait_ms--;
    }
    
    NEST_LOG(LOG_DEBUG, "start wait time left %u, pid %u", wait_ms, pid);
}


/**
 * @brief 等待进程执行停止的时间, 等待到no pid
 */
void CThreadProcBase::CheckWaitStop(uint32_t pid, uint32_t wait_ms)
{
    while (wait_ms > 0)
    {
        int32_t ret = kill(pid, 0);
        if ((ret == -1) && (errno == ESRCH))
        {
            break;
        }

        usleep(1*1000);
        wait_ms--;
    }
    
    NEST_LOG(LOG_DEBUG, "stop wait time left %u, pid %u", wait_ms, pid);
}

/**
 * @brief 修改子进程的用户名
 * @return =0 成功 <0 失败
 */
int32_t CThreadProcBase::ChgUser(const char *user)
{
    struct passwd *pwd;

    pwd = getpwnam(user);
    if (!pwd)
    {
        return -1;
    }

    setgid(pwd->pw_gid);
    setuid(pwd->pw_uid);
    prctl(PR_SET_DUMPABLE, 1);

    return 0;
}

/*
 * @brief 获取spp包基准目录
 */
char* CThreadProcBase::GetBinBasePath(void)
{
    char *path = NEST_BIN_PATH;

    if (CAgentProxy::_cluster_type == "sf2")
    {
        path = "/home/oicq/";
    }

    return path;
}

/**
 * @brief 启动进程运行的执行函数
 */
int32_t CThreadProcBase::StartProc(const nest_proc_base& conf, nest_proc_base& proc)
{
    int32_t ret = 0;

    // 1. 组合路径信息
    char path[256];
    snprintf(path, sizeof(path), "%s%s/bin", GetBinBasePath(), _package_name.c_str());
    if (!TAgentNetMng::IsDirExist(path))
    {
        NEST_LOG(LOG_ERROR, "path[%s] not exist, failed", path);
        return -1;
    }
    
    // 2. 组合bin与etc信息
    char bin[256], etc[256];
    if (conf.proc_type() == 0)
    {
        snprintf(bin, sizeof(bin), "./spp_%s_proxy", _service_name.c_str());
        snprintf(etc, sizeof(etc), "../etc/spp_proxy.xml");
    }
    else
    {
        snprintf(bin, sizeof(bin), "./spp_%s_worker", _service_name.c_str());
        snprintf(etc, sizeof(etc), "../etc/spp_worker%u.xml", conf.proc_type());
    }
    NEST_LOG(LOG_NORMAL, "start exec bin[%s], etc[%s]", bin, etc);

    // 检查当前内存量
    // 内存空闲量大于CGroupMng::cgroup_conf.mem_sys_reserved_mb
    CSysInfo sys;
    if(!(sys.GetMemFree() > (CGroupMng::cgroup_conf.mem_sys_reserved_mb <<10)))
    {
        NEST_LOG(LOG_ERROR, "fork child rejected, sys overload[ left_mem=%u, reserved_mem=%u]",
            sys.GetMemFree(),(CGroupMng::cgroup_conf.mem_sys_reserved_mb <<10));
        return -2;        
    }

    // 3. fork-exec 启动进程
    TCloneArgs args;
    args.base       = conf;
    args.bin        = bin;
    args.etc        = etc;
    args.path       = path;
    args.service    = _service_name;
    args.package    = _package_name;

    pid_t pid = DoFork(&args);
    if (pid < 0)
    {
        NEST_LOG(LOG_ERROR, "fork child failed, ret[%d][%m]", pid);
        return -3;
    }

    // 4. 父进程主动探测设置子进程
    CheckWaitStart((uint32_t)pid, 10);   // 最大等待200ms

    if (kill(pid, 0) == -1 && errno == ESRCH)
    {
        NEST_LOG(LOG_ERROR, "proc[%d] start failed", pid);
        return -4;
    }
    proc = conf;
    proc.set_proc_pid((uint32_t)pid);

    // 5. 设置成功, 启动OK
    uint32_t worker_type = 0xffff;
    ret = InitSendRecv(proc, worker_type);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "proc[%d] remote init failed", pid);
        DelProc(proc);  // 异常处理
        return -5;
    }

    if (worker_type != 0xffff)
    {
        CServiceMng::Instance()->UpdateType(_service_name, worker_type);
    }

    return 0;
    
}


/**
 * @brief 终止进程运行的执行函数, 用于异常回滚
 */
void CThreadProcBase::DelProc(const nest_proc_base& proc)
{
    uint32_t pid = proc.proc_pid();

    if (pid > 0) {
        kill(pid, 10);
        CheckWaitStop(pid, 200);
        kill(pid, 9);
    }
    
    return;
}


/**
 * @brief clone执行进程初始化的入口函数
 */
int32_t CThreadProcBase::CloneEntry(void* args)
{
    TCloneArgs* conf = static_cast<TCloneArgs*>(args);
    char output[256];

    #if 0
    // 1. 路径检查, 私有目录创建
    if (!CThreadProcBase::MountPriPath(conf->package))
    {
        NEST_LOG(LOG_ERROR, "make service[%s] private path failed", conf->service.c_str());
        return -1;
    }
    
    // 2. 切换用户名
    if (CThreadProcBase::ChgUser("user_00") < 0)
    {
        snprintf(output, sizeof(output), "change user failed");
        NEST_LOG(LOG_ERROR, "%s", output);
        perror(output);
        return -2;
    }

    // 3. 切换工作目录
    if (::chdir(conf->path.c_str()) < 0)
    {
        snprintf(output, sizeof(output), "change work path[%s] failed", conf->path.c_str());
        NEST_LOG(LOG_ERROR, "%s", output);
        perror(output);
        exit(1);
        return -3;
    }
    NEST_LOG(LOG_NORMAL, "start exec bin[%s], etc[%s], pid[%u]", conf->bin.c_str(), 
            conf->etc.c_str(), getpid());
            
    // 4. 私有脚本执行
    if (!TAgentNetMng::IsFileExist(conf->etc.c_str()))
    {
        system("./yaml_tool x");
        NEST_LOG(LOG_NORMAL, "[%s] etc [%s] not exist, yaml_tool run", 
                conf->path.c_str(), conf->etc.c_str());
    }
    
    // 5. LD_LIBRARY_PATH环境变量设置值
    char *envname = "LD_LIBRARY_PATH";
    char  env[2048] = {0};
    char *env_tmp = getenv(envname);
    if (env_tmp)
    {
        snprintf(env, sizeof(env), "%s=/usr/local/services/%s/client/%s/lib:%s",
                 envname, conf->package.c_str(), conf->service.c_str(), env_tmp);
    }
    else
    {
        snprintf(env, sizeof(env), "%s=/usr/local/services/%s/client/%s/lib",
                 envname, conf->package.c_str(), conf->service.c_str());
    }

    // 6. 执行业务进程启动
    char *const envp[] = {env, (char *)0};
    char *const cmd[] = {(char*)conf->bin.c_str(), (char*)conf->etc.c_str(), (char*)0};
    int32_t ret = execve(conf->bin.c_str(), cmd, envp);
    if (ret != 0)
    {
        snprintf(output, sizeof(output), "start proc[%s %s] failed, ret %d", 
                conf->bin.c_str(), conf->etc.c_str(), ret);
        NEST_LOG(LOG_ERROR, "%s", output);
        perror(output);
        exit(1);
        return -4;
    }
    #endif

    // 启动agent exec tool，拉起业务进程
    // ../tool/agent_exec_tool bin etc package service
    char *exec_tool = "../tool/agent_exec_tool";
    char *const cmd[] = {exec_tool, (char*)conf->bin.c_str(), (char*)conf->etc.c_str(), (char*)conf->package.c_str(), (char*)conf->service.c_str(), (char *)CAgentProxy::_cluster_type.c_str(), (char*)0};
    int32_t ret = execv(exec_tool, cmd);
    if (ret != 0)
    {
        snprintf(output, sizeof(output), "start proc[%s %s] failed, ret %d", 
                conf->bin.c_str(), conf->etc.c_str(), ret);
        NEST_LOG(LOG_ERROR, "%s", output);
        perror(output);
        exit(1);
        return -1;
    }

    return 0;    
}

/**
 * @brief 检查path是否存在，如果不存在则创建
 */
bool CThreadProcBase::CheckAndMkdir(char *path)
{
    if (!path)
    {
        NEST_LOG(LOG_ERROR, "make path failed, path is null");
        return false;
    }

    if (!TAgentNetMng::IsDirExist(path))
    {
        if (::mkdir(path, 0666) < 0) 
        {
            NEST_LOG(LOG_ERROR, "make path [%s] failed(%m)", path);
            return false;
        }
    }
    
    if (::chmod(path, 0777) < 0)
    {
        NEST_LOG(LOG_ERROR, "change mod path [%s] failed(%m)", path);
        return false;
    }

    return true;
}

/**
 * @brief  递归创建目录
 * @return =0 成功 <0 失败
 */
int32_t CThreadProcBase::MkdirRecursive(char *path, size_t len)
{
    char    cpath[256];
    char*   pos = path + 1;

    if (len > sizeof(cpath))
    {
        NEST_LOG(LOG_ERROR, "make path [%s] failed, path too long.", path);
        return -1;
    }

    // 循环创建目录
    while ((pos = index(pos, '/')))
    {
        int32_t len = pos - path;

        strncpy(cpath, path, len);
        *(cpath + len) = '\0';
        
        if (!CThreadProcBase::CheckAndMkdir(cpath))
        {
            NEST_LOG(LOG_ERROR, "check and mkdir [%s] failed", path);
            return -2;
        }
        
        pos++;
    }

    // 检查并创建目录
    if (!CThreadProcBase::CheckAndMkdir(path))
    {
        NEST_LOG(LOG_ERROR, "check and mkdir [%s] failed", path);
        return -3;
    }

    return 0;
}

/**
 * @brief 检查并重映射/data目录到私有的path
 */
bool CThreadProcBase::MountPriPath(string& package)
{
    // 1. 确认PATH路径OK
    char src_path[256];
    char dst_path[256] = "/data/release";
    snprintf(src_path, sizeof(src_path), "/data/spp_mount_dir/%s/release", package.c_str());

    if (CThreadProcBase::MkdirRecursive(src_path, sizeof(src_path)) < 0)
    {
        NEST_LOG(LOG_ERROR, "make path [%s] failed", src_path);
        return false;
    }

    if (CThreadProcBase::MkdirRecursive(dst_path, sizeof(dst_path)) < 0)
    {
        NEST_LOG(LOG_ERROR, "make path [%s] failed", dst_path);
        return false;
    }

#ifndef MS_PRIVATE
#define MS_PRIVATE (1<<18)
#endif

    // 2. 执行私有目录bind
    if (::mount(src_path, dst_path, NULL, MS_BIND | MS_PRIVATE, NULL) != 0)
    {
        NEST_LOG(LOG_ERROR, "mount path [%s] to /data/release failed(%m)", src_path);
        return false;
    }

 
    return true;    
}

/**
 * @brief 执行隔离目录的进程clone操作
 * @return >0 子进程启动OK, 返回PID, 否则失败
 */
 /*
pid_t CThreadProcBase::DoFork(void* args)
{
    size_t stack_size = 4096;
    void *stack = alloca(stack_size);
    pid_t ret = clone(CloneEntry, (char*)stack + stack_size, CLONE_NEWNS | CLONE_PTRACE | SIGCHLD, args);
    if (ret < 0)    
    {        
        NEST_LOG(LOG_ERROR, "clone failed, ret %d(%m)", ret);
        return -1;
    }

    usleep(1);    
    return ret;
}
*/
pid_t CThreadProcBase::DoFork(void* args)
{
    TCloneArgs* conf = static_cast<TCloneArgs*>(args);
    int32_t ret = -1;
    char req_buff[4*1024]; 
    char rsp_buff[4*1024];

    static atomic_t g_seq = ATOMIC_INIT(0);

    uint32_t curr_seq = (uint32_t)atomic_inc_return(&g_seq);
    if(curr_seq==0)
    {
         curr_seq = (uint32_t)atomic_inc_return(&g_seq);
    }

    nest_msg_head  head;        
    nest_sched_proc_fork_req req_body;
    nest_sched_proc_fork_rsp rsp_body;

    // 设置请求头
    head.set_msg_type(NEST_PROTO_TYPE_AGENT);
    head.set_sub_cmd(NEST_SCHED_CMD_FORK_PROC);

    // 设置请求正文
    req_body.set_task_id(curr_seq);
    req_body.set_service_name(conf->service);
    req_body.set_package_name(conf->package);
    *req_body.mutable_proc_info() = conf->base;
    req_body.set_path(conf->path);
    req_body.set_bin(conf->bin);
    req_body.set_etc(conf->etc);
    
    
    string head_str, body_str;
    if (!head.SerializeToString(&head_str)
        || !req_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed, error!");
        return -1;
    }

    NestPkgInfo req_pkg = {0};
    req_pkg.msg_buff    = req_buff;
    req_pkg.buff_len    = (uint32_t)sizeof(req_buff);
    req_pkg.head_buff   = (void*)head_str.data();
    req_pkg.head_len    = head_str.size();
    req_pkg.body_buff   = (void*)body_str.data();
    req_pkg.body_len    = body_str.size();

    // 打包请求包
    string err_msg;
    int32_t req_len = NestProtoPackPkg(&req_pkg, err_msg);
    if (req_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                req_len, err_msg.c_str());
        return -2;
    }


    struct sockaddr_un uaddr = {0};
    uaddr.sun_family   = AF_UNIX;
    snprintf(uaddr.sun_path,  sizeof(uaddr.sun_path), NEST_CLONE_UNIX_PATH);
    TNestAddress address;
    address.SetAddress(&uaddr, true);

    uint32_t rsp_len = (int32_t)sizeof(rsp_buff);

    // 发送请求，并等待响应
    ret = TAgentNetMng::Instance()->SendRecvLocal(&address, req_buff, 
                                (uint32_t)req_len, rsp_buff, rsp_len, 1000); // 1 second
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "send recv req msg failed, ret %d", ret);
        return -3;
    }

    NEST_LOG(LOG_DEBUG, "agent_clone process send back ok, run end!");

    // 检查响应包
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = rsp_buff;
    rsp_pkg.buff_len    = rsp_len;
        
    ret = NestProtoCheck(&rsp_pkg, err_msg);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "rsp check failed, ret %d, msg: %s", ret, err_msg.c_str());    
        return -4;
    }        

    // 解析报文头信息
    nest_msg_head rsp_head;
    if (!rsp_head.ParseFromArray((char*)rsp_pkg.head_buff, rsp_pkg.head_len))
    {
        NEST_LOG(LOG_ERROR, "rsp head parse failed");    
        return -5;
    }  

    if(rsp_head.result()!=NEST_RESULT_SUCCESS)
    {
        NEST_LOG(LOG_ERROR, "rsp head error=%d, errmsg=%s", rsp_head.result(), rsp_head.err_msg().c_str());
        return -6;
    }

    // 解析报文正文
    if(!rsp_body.ParseFromArray((char*)rsp_pkg.body_buff, rsp_pkg.body_len))
    {
        NEST_LOG(LOG_ERROR, "rsp body parse failed");    
        return -7;        
    }

    // 获取响应的pid
    pid_t ret_pid = (pid_t)rsp_body.proc_pid();

    if(ret_pid<0)
    {
        NEST_LOG(LOG_ERROR, "fork child ret pid=%d",(int32_t)ret_pid);
        return -8;
    }
    
    //检查seq
    if(curr_seq!=rsp_body.task_id())
    {
       NEST_LOG(LOG_ERROR,"clone proc return msg error, seq not match (req=%u vs rsp=%u)",curr_seq,rsp_body.task_id());
       kill(ret_pid,9);
       return -9;
    }

    return ret_pid;
}

/*
// fork进程请求
message nest_sched_proc_fork_req
{
    optional uint32 task_id        = 1;         // 任务id	
    optional string service_name   = 2;         // 业务名
    optional string package_name   = 3;         // 插件包
    optional nest_proc_base proc_info = 4;      // 启动进程数

    optional string path = 5;
    optional string bin  = 6;
    optional string etc  = 7;
}

// fork进程应答
message nest_sched_proc_fork_rsp
{
    optional uint32 task_id        = 1;         // 任务id	
    optional string service_name   = 2;         // 业务名
    optional string package_name   = 3;         // 插件包
    optional nest_proc_base proc_info = 4;      // 启动进程数

    optional string path = 5;
    optional string bin  = 6;
    optional string etc  = 7;

    optional uint32 proc_pid       = 8;         // pid -请求无效, 应答有效
}
*/

/**
 * @brief 更新消息处理上下文
 */
void CThreadAddProc::SetCtx(TNestBlob* blob, TMsgAddProcReq* req)
{
    _blob = *blob;
    _msg_head = req->GetMsgHead();
    _req_body = req->GetMsgReqBody();
    this->SetServiceName(_req_body.service_name());
    this->SetPackageName(_req_body.package_name());

    NEST_LOG(LOG_DEBUG, "nest_sched_add_proc_req thread info: head: %s, body: %s",
        _msg_head.ShortDebugString().c_str(),  _req_body.ShortDebugString().c_str());
}

/**
 * @brief 更新重启任务处理上下文
 */
void CThreadAddProc::SetRestartCtx(nest_sched_add_proc_req* req)
{
    _restart_flag = true;
    _req_body = *req;
    this->SetServiceName(_req_body.service_name());
    this->SetPackageName(_req_body.package_name());
}

/**
 * @brief 线程执行入口函数
 */
void CThreadAddProc::Run()
{
    if (!_restart_flag)
    {
        this->RunStart();
    }
    else
    {
        this->RunRestart();
    }
}

/**
 * @brief 线程执行启动入口函数
 */
void CThreadAddProc::RunStart()
{
    int32_t  i = 0, ret = 0;
    string head_str, body_str;

    // 1. 确认进程的包文件是否OK
    NEST_LOG(LOG_DEBUG, "CThreadAddProc start run! proc count: %u", _req_body.proc_conf_size());
    
    // 2. 遍历进程数目, 依次执行start进程
    for (i = 0; i < _req_body.proc_conf_size(); i++)
    {
        const nest_proc_base& conf = _req_body.proc_conf(i);
        NEST_LOG(LOG_DEBUG, "CThreadAddProc begin start[%u-%u-%u]", conf.service_id(),
            conf.proc_type(), conf.proc_no());
        
        nest_proc_base proc;
        ret = this->StartProc(conf, proc);
        if (ret < 0)
        {
            NEST_LOG(LOG_ERROR, "start proc failed, ret %d", ret);
            break;
        }
        
        NEST_LOG(LOG_DEBUG, "CThreadAddProc start ok[%u-%u-%u] pid[%u]!", proc.service_id(),
            proc.proc_type(), proc.proc_no(), proc.proc_pid());
        
        nest_proc_base* pdata = _rsp_body.add_proc_info();
        *pdata = proc;
    }

    // 2.1 失败回滚
    if (ret < 0)
    {
        for (i = 0; i < _rsp_body.proc_info_size(); i++)
        {
            const nest_proc_base& proc = _rsp_body.proc_info(i);
            this->DelProc(proc);
        }
        _rsp_body.clear_proc_info();

        // 失败需要带回proc信息, 保证清理端口信息OK
        for (i = 0; i < _req_body.proc_conf_size(); i++)
        {
            const nest_proc_base& conf = _req_body.proc_conf(i);
            nest_proc_base* pdata = _rsp_body.add_proc_info();
            *pdata = conf;
        }

        _msg_head.set_result(NEST_ERROR_START_PROC_ERROR);
        _msg_head.set_err_msg("start proc failed");
        
        NEST_LOG(LOG_ERROR, "CThreadAddProc exec failed");
    }
    else  // 2.2 成功返回
    {
        _msg_head.set_result(NEST_RESULT_SUCCESS);
        
        NEST_LOG(LOG_DEBUG, "CThreadAddProc exec success");
    }
    
    _rsp_body.set_task_id(_req_body.task_id());
    _rsp_body.set_proc_type(_req_body.proc_type());
    _rsp_body.set_service_id(_req_body.service_id());
    _rsp_body.set_service_name(_req_body.service_name());
    _rsp_body.set_package_name(_req_body.package_name());

    // 3. 打包并发送应答
    _msg_head.set_msg_type(NEST_PROTO_TYPE_AGENT);
    if (!_msg_head.SerializeToString(&head_str)
        || !_rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return;
    }

    // 4. pack msg
    static char rsp_buff[65535];
    uint32_t buff_len = sizeof(rsp_buff);

    // 1. 调用打包接口, 封装到临时buff
    NestPkgInfo rsp_pkg = {0};
    rsp_pkg.msg_buff    = rsp_buff;
    rsp_pkg.buff_len    = buff_len;
    rsp_pkg.head_buff   = (void*)head_str.data();
    rsp_pkg.head_len    = head_str.size();
    rsp_pkg.body_buff   = (void*)body_str.data();
    rsp_pkg.body_len    = body_str.size();

    string err_msg;
    int32_t ret_len = NestProtoPackPkg(&rsp_pkg, err_msg);
    if (ret_len < 0)
    {
        NEST_LOG(LOG_ERROR, "resp msg pack failed, ret %d, err: %s", 
                ret_len, err_msg.c_str());
        return ;
    }

    
    ret = CAgentProxy::Instance()->SendToAgent(head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "local msg pack send failed, ret %d", ret);
        return;
    }

    NEST_LOG(LOG_DEBUG, "CThreadAddProc thread send back ok, run end!");

    return;
}

/**
 * @brief 线程执行进程重启入口函数
 */
void CThreadAddProc::RunRestart()
{
    int32_t  ret = 0;
    string head_str, body_str;

    // 1. 确认进程的包文件是否OK
    NEST_LOG(LOG_DEBUG, "CThreadAddProc restart run!");
    
    // 2. 重启单个进程
    if (_req_body.proc_conf_size() <= 0)
    {
        NEST_LOG(LOG_ERROR, "restart proc failed, no proc info");
        return;
    }
    const nest_proc_base& conf = _req_body.proc_conf(0);

    // 3. 删除进程
    this->DelProc(conf);
    
    // 4. 执行启动
    nest_proc_base proc;
    ret = this->StartProc(conf, proc);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "start proc failed, ret %d", ret);
        return;
    }
    
    NEST_LOG(LOG_DEBUG, "CThreadAddProc start ok[%u-%u-%u] pid[%u]!", proc.service_id(),
        proc.proc_type(), proc.proc_no(), proc.proc_pid());

    return;
}


/**
 * @brief 单个进程的删除停止工作
 */
int32_t CThreadDelProc::DeleteProcess(const nest_proc_base& proc)
{
    uint32_t pid = 0;
    
    // 1. 查询本地进程条目信息
    TProcKey key;
    key._service_id     = proc.service_id();
    key._proc_type      = proc.proc_type();
    key._proc_no        = proc.proc_no();    
    TNestProc* proc_item = TNestProcMng::Instance()->FindProc(key);
    if (proc_item != NULL)
    {
        NEST_LOG(LOG_DEBUG, "proc exist, [%u][%u][%u], pid[%u]", proc.service_id(),
                proc.proc_type(), proc.proc_no(), proc_item->GetProcPid());
        pid = proc_item->GetProcPid();
    }
    else
    {
        NEST_LOG(LOG_ERROR, "proc not exist, [%u][%u][%u]", proc.service_id(),
                proc.proc_type(), proc.proc_no());
        pid = proc.proc_pid(); // 如果进程没找到, 可以尝试杀死指定的pid
    }

    // 2. 如果pid还是无效, 则退出处理
    if (pid == 0) 
    {
        NEST_LOG(LOG_ERROR, "Del proc pno invalid: 0");
        return -1;
    }

    nest_proc_base del_proc;
    del_proc = proc;
    del_proc.set_proc_pid(pid);

    // 3. 尝试安全退出, 等待后强制杀死进程
    NEST_LOG(LOG_DEBUG, "Del proc[%u-%u-%u], pid[%u]!", proc.service_id(),
             proc.proc_type(), proc.proc_no(), pid);
             
    this->DelProc(del_proc);
    
    return 0;
}


/**
 * @brief 删除的上下文设置
 */
void CThreadDelProc::SetCtx(TNestBlob* blob, TMsgDelProcReq* req)
{
    _blob = *blob;
    _msg_head = req->GetMsgHead();
    _req_body = req->GetMsgReqBody();
}


/**
 * @brief 进程删除的入口函数
 */
void CThreadDelProc::Run()
{
    int32_t  i = 0, ret = 0;
    string head_str, body_str;

    // 1. 确认进程的包文件是否OK
    NEST_LOG(LOG_DEBUG, "CThreadDelProc start run!");
    
    // 2. 遍历进程数目, 依次执行start进程
    for (i = 0; i < _req_body.proc_list_size(); i++)
    {
        const nest_proc_base& conf = _req_body.proc_list(i);
        
        NEST_LOG(LOG_DEBUG, "CThreadDelProc del proc[%u-%u-%u] pid[%u]!", conf.service_id(),
            conf.proc_type(), conf.proc_no(), conf.proc_pid());
        
        ret = this->DeleteProcess(conf);
        if (ret < 0)
        {
            NEST_LOG(LOG_ERROR, "delete proc failed, ret %d", ret);
            continue;
        }
        
        nest_proc_base* pdata = _rsp_body.add_proc_list();
        *pdata = conf;
    }

    // 2. 默认会成功
    _msg_head.set_result(NEST_RESULT_SUCCESS);
    NEST_LOG(LOG_DEBUG, "CThreadDelProc exec success");
    
    _rsp_body.set_task_id(_req_body.task_id());

    // 3. 打包并发送应答
    _msg_head.set_msg_type(NEST_PROTO_TYPE_AGENT);
    if (!_msg_head.SerializeToString(&head_str)
        || !_rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return;
    }

    // 4. 发送消息
    ret = CAgentProxy::Instance()->SendToAgent(head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "local msg pack send failed, ret %d", ret);
        return;
    }

    NEST_LOG(LOG_DEBUG, "CThreadDelProc thread send back ok, run end!");

    return;
}



/**
 * @brief 更新消息处理上下文
 */
void CThreadRestartProc::SetCtx(TNestBlob* blob, TMsgRestartProcReq* req)
{
    _blob = *blob;
    _msg_head = req->GetMsgHead();
    _req_body = req->GetMsgReqBody();
    this->SetServiceName(_req_body.service_name());
    this->SetPackageName(_req_body.package_name());
}

/**
 * @brief 线程执行入口函数
 */
void CThreadRestartProc::Run()
{
    int32_t  i = 0, ret = 0;
    string head_str, body_str;
    char err_buf[256];

    // 1. 确认进程的包文件是否OK
    NEST_LOG(LOG_DEBUG, "CThreadRestartProc start run!");
    
    // 2. 遍历进程数目, 依次执行start进程
    for (i = 0; i < _req_body.proc_conf_size(); i++)
    {
        const nest_proc_base& conf = _req_body.proc_conf(i);
        nest_proc_base proc;

        // 3.1 删除进程操作
        this->DelProc(conf);
       
        // 3.2 重启进程操作
        ret = this->StartProc(conf, proc);
        if (ret < 0)
        {
            snprintf(err_buf, sizeof(err_buf), "restart proc[%u-%u-%u] failed!",
                proc.service_id(), proc.proc_type(), proc.proc_no());
            NEST_LOG(LOG_ERROR, "%s, ret %d", err_buf, ret);
            break;
        }
        
        NEST_LOG(LOG_DEBUG, "CThreadRestartProc restart ok[%u-%u-%u] pid[%u]!", proc.service_id(),
            proc.proc_type(), proc.proc_no(), proc.proc_pid());
        
        nest_proc_base* pdata = _rsp_body.add_proc_info();
        *pdata = proc;
    }

    // 4.1 失败回滚
    if (ret < 0)
    {
        for (i = 0; i < _rsp_body.proc_info_size(); i++)
        {
            const nest_proc_base& proc = _rsp_body.proc_info(i);
            this->DelProc(proc);
        }
        _rsp_body.clear_proc_info();

        _msg_head.set_result(NEST_ERROR_START_PROC_ERROR);
        _msg_head.set_err_msg(err_buf);
        
        NEST_LOG(LOG_ERROR, "CThreadRestartProc exec failed");
    }
    else  // 4.2 成功返回
    {
        _msg_head.set_result(NEST_RESULT_SUCCESS);
        
        NEST_LOG(LOG_DEBUG, "CThreadRestartProc exec success");
    }
    
    _rsp_body.set_task_id(_req_body.task_id());
    _rsp_body.set_service_name(_req_body.service_name());
    _rsp_body.set_package_name(_req_body.package_name());

    // 5. 打包并发送应答
    _msg_head.set_msg_type(NEST_PROTO_TYPE_AGENT);
    if (!_msg_head.SerializeToString(&head_str)
        || !_rsp_body.SerializeToString(&body_str))
    {
        NEST_LOG(LOG_ERROR, "pb format string pack failed");
        return;
    }

    // 6. pack msg
    ret = CAgentProxy::Instance()->SendToAgent(head_str, body_str);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "local msg pack send failed, ret %d", ret);
        return;
    }

    NEST_LOG(LOG_DEBUG, "CThreadRestartProc thread send back ok, run end!");

    return;
}


