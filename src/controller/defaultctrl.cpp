#include "defaultctrl.h"
#include "../comm/tconfbase/ctrlconf.h"
#include "../comm/tconfbase/tinyxmlcomm.h"
#include "../comm/tbase/misc.h"
#include "../comm/benchadapter.h"
#include "../comm/comm_def.h"
#include "../comm/global.h"
#include "../comm/singleton.h"
#include "../comm/keygen.h"
#include "../comm/exception.h"
#include "../comm/monitor.h"
#include "../comm/timestamp.h"
#include <libgen.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <list>
#include <map>



#define NOTIFY_NUM   "notify_num"

#define PROXY_PID   "proxy_pid"
#define PROXY_FD    "proxy_fd"
#define PROXY_MEM   "proxy_mem"
#define PROXY_LOAD  "proxy_load"
#define PROXY_SRATE "proxy_srate"
#define PROXY_BRATE "proxy_brate"

using namespace spp::comm;
using namespace spp::ctrl;
using namespace spp::global;
using namespace spp::singleton;
using namespace spp::statdef;
using namespace spp::exception;
using namespace spp::tconfbase;
using namespace std;

#define SPP_PID_FILE "./spp.pid"

CDefaultCtrl::CDefaultCtrl()
: pidfile_fd_(0) 
{
	__spp_get_now_ms();
}

CDefaultCtrl::~CDefaultCtrl()
{
    if (pidfile_fd_ > 0)
    {
        close(pidfile_fd_);
        unlink(SPP_PID_FILE);
    }


    for (unsigned i = 0; i < cmd_on_exit_.size(); ++i)
        system(cmd_on_exit_[i].c_str());
}

void CDefaultCtrl::dump_pid()
{
    static char buf[2][8192]; // 保存新旧pid list
    static unsigned idx = 0;
    int len = monsrv_.dump_pid_list(buf[idx], 8192);
    if (strncmp(buf[0], buf[1], len) != 0)
    {// 发生变化时写入文件
        flock(pidfile_fd_, LOCK_EX);
        ftruncate(pidfile_fd_, 0);
        lseek(pidfile_fd_, 0, SEEK_SET);
        write(pidfile_fd_, buf[idx], len);
        flock(pidfile_fd_, LOCK_UN);

        idx = (idx + 1) % 2; // swap
    }
}

void CDefaultCtrl::check_report_tool()
{
    static int pid = -2;
    static int pipe_fd[2];
    static FILE* rstream;

    if (report_tool.empty()) return;
    if (CServerBase::quit() && pid > 0)
    {
        kill(pid, 10);
        usleep(200000);
        kill(pid, 9);
    }

    if (pid == -2)
    {// 第一次调用
        if ( 0 != pipe(pipe_fd) )
        {
            flog_.LOG_P_PID(LOG_ERROR, "pipe error:%m\n");
            report_tool.clear();
            return;
        }
        int fl = fcntl(pipe_fd[0], F_GETFL);
        fl = fl | O_NONBLOCK;
        if ( 0 > fcntl(pipe_fd[0], F_SETFL, fl) )
        {
            flog_.LOG_P_PID(LOG_ERROR, "fcntl error:%m\n");
            report_tool.clear();
            return;
        }
        rstream = fdopen(pipe_fd[0], "r");
        if (rstream == NULL)
        {
            flog_.LOG_P_PID(LOG_ERROR, "fdopen error:%m\n");
            report_tool.clear();
            return;
        }

    }


    if (pid < 0)
    {// fork and exec
        pid = fork();
        if (pid == 0)
        {
            dup2(pipe_fd[1], STDOUT_FILENO);
            char *cmd[] = {(char *)report_tool.c_str(), (char *)0 };
            if (execv (report_tool.c_str(), cmd)) _exit(1);
        }
        return;
    }
    else
    {// check
        char* line = NULL;
        size_t len = 0;
        while (getline(&line, &len, rstream) != -1)
        {
            flog_.LOG_P_PID(LOG_ERROR, "report_tool: %s", line);
        }
        if (line) free(line);
        clearerr(rstream);

        if (kill(pid, 0) == -1 /*&& errno == ESRCH 刚好pid相同? */)
            pid = -1;
    }


}

void CDefaultCtrl::check_ctrl_running()
{
    FILE* file = fopen(SPP_PID_FILE, "r");
    if (file == NULL) return;

    flock(fileno(file), LOCK_EX);

    size_t len = 0;
    char* line = NULL;
    int pid = 0;
    if(getline(&line, &len, file) != -1)
    {
        // line = "xxx [SPP_CTRL]"
        if (line)
        {
            char* pos = strstr(line, "[SPP_CTRL]");
            if (pos)
            {
                char pidstr[16] = {0};
                strncpy(pidstr, line, pos - line - 1);
                pid = atoi(pidstr);
            }
        }
    }
    if (line) free(line);

    flock(fileno(file), LOCK_UN);
    fclose(file);
	
	if (pid <= 0)
	{
		unlink(SPP_PID_FILE);
	}

    if(pid > 0 && !CMisc::check_process_exist(pid))
    {
        unlink(SPP_PID_FILE);
    }
}

void CDefaultCtrl::realrun(int argc, char* argv[])
{
    check_ctrl_running();
    // 放这里是因为，ctrl已经启动情况下，仍可以./spp_ctrl -v
    pidfile_fd_ = open(SPP_PID_FILE, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (pidfile_fd_ == -1)
    {
        fprintf(stderr, "spp_ctrl open "SPP_PID_FILE" error:%m\n"
                        "spp_ctrl is running? Check it now~ OR delete "SPP_PID_FILE"\n\n");
        exit(-1);
    }

    fcntl(pidfile_fd_, F_SETFD, FD_CLOEXEC);
	
	//提前一些，尽量早些写入文件，避免空文件无发互斥 2014-08-05 evanqi@tencent.com
	dump_pid();
    clear_stat_file();

    //初始化配置
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    initconf(false);

    flog_.LOG_P_PID(LOG_FATAL, "controller started!\n");

    monsrv_.run(); // 让第一周期就有数据
    sleep(2);

    while (true)
    {
        //监控信息处理
        monsrv_.run();

        //检查reload信号
        //利用reload信号实现worker和proxy的热重载功能
        if (unlikely(CServerBase::reload()))
        {
            flog_.LOG_P_FILE(LOG_FATAL, "recv reload signal\n");

            //被监控的进程根据ctrl指令执行重启
            //ctrl读取热加载配置文件
            reloadconf();
            monsrv_.run();
        }

        //检查quit信号
        if (unlikely(CServerBase::quit()))
        {
            flog_.LOG_P_PID(LOG_FATAL, "recv quit signal\n");

            check_report_tool();

            //停止所有被监控的进程
            monsrv_.killall(SIGUSR1);
            // 包脚本重启会等1s，而worker控制在500ms内安全退出
            usleep(500*1000);
            // 消灭残留进程
            monsrv_.killall(SIGKILL);
            break;
        }

        dump_pid();
        check_report_tool();
        sleep(3);
    }

    flog_.LOG_P_PID(LOG_FATAL, "controller stopped!\n");
}

int CDefaultCtrl::initconf(bool reload)
{
    // 清理锁文件
    system("rm -rf ../bin/.mq_*.lock 2>/dev/null");

    CCtrlConf conf(new CLoadXML(ix_->argv_[1]));
    if (conf.init() != 0)
    {
        exit(-1);
    }

    //初始化日志

    Log flog;
    if (conf.getAndCheckFlog(flog) == ERR_CONF_CHECK_UNPASS)
    {
        exit(-1);
    }

    flog_.LOG_OPEN(flog.level, flog.type, flog.path.c_str(),
                   flog.prefix.c_str(), flog.maxfilesize, flog.maxfilenum);

	Exception ex;
	if (conf.getAndCheckException(ex) == 0)
	{
		_spp_g_exceptioncoredump = ex.coredump;
		_spp_g_exceptionpacketdump = ex.packetdump;
		_spp_g_exceptionrestart = ex.restart;
		_spp_g_exceptionmonitor = ex.monitor;
		_spp_g_exceptionrealtime = ex.realtime;
	}

	if (_spp_g_exceptionmonitor)
	{
		//初始化monitor
		try
		{
			_spp_g_monitor = new CSngMonitor;
		}
		catch(runtime_error &ex)
		{
			_spp_g_monitor = NULL;
		}
	}

    //groupinfo配置

    Procmon procmon ;
    if (conf.getAndCheckProcmon(procmon) != 0)
    {
        exit(-1);
    }
    TGroupInfo groupinfo;
    int groups_num = 0;
    for (size_t i = 0; i < procmon.entry.size(); ++ i)
    {

        memset(&groupinfo, 0x0, sizeof(TGroupInfo));
        groupinfo.adjust_proc_time = 0;
        groupinfo.groupid_ = procmon.entry[i].id;
        strncpy(groupinfo.basepath_, procmon.entry[i].basepath.c_str(), sizeof(groupinfo.basepath_) - 1);
        strncpy(groupinfo.exefile_, procmon.entry[i].exe.c_str(), sizeof(groupinfo.exefile_) - 1);
        strncpy(groupinfo.etcfile_, procmon.entry[i].etc.c_str(), sizeof(groupinfo.etcfile_) - 1);

        groupinfo.exitsignal_ = procmon.entry[i].exitsignal;
        groupinfo.maxprocnum_ = procmon.entry[i].maxprocnum;
        groupinfo.minprocnum_ = procmon.entry[i].minprocnum;
        groupinfo.heartbeat_ = procmon.entry[i].heartbeat;
        groupinfo.affinity_ = procmon.entry[i].affinity;

        // 清理共享内存
        key_t comsumer_key = pwdtok( 2 * groupinfo.groupid_ );
        key_t producer_key = pwdtok( 2 * groupinfo.groupid_ + 1 );

        char rm_cmd[64] = {0};
        snprintf( rm_cmd, sizeof( rm_cmd ), "ipcrm -M 0x%x 2>/dev/null", comsumer_key );
        cmd_on_exit_.push_back(rm_cmd);
        system( rm_cmd );
        snprintf( rm_cmd, sizeof( rm_cmd ), "ipcrm -M 0x%x 2>/dev/null", producer_key);
        cmd_on_exit_.push_back(rm_cmd);
        system( rm_cmd );

        groups_num ++;

        if (!reload)
        {
            int ret = monsrv_.add_group(&groupinfo);
            if(0 != ret)
                LOG_SCREEN(LOG_ERROR, "ctrl add group[%d] error, ret[%d]\n", groupinfo.groupid_, ret);
        }
        else
            monsrv_.mod_group(groupinfo.groupid_, &groupinfo);

    }


    key_t mqkey = pwdtok(255);
    flog_.LOG_P_PID(LOG_NORMAL, "spp ctrl mqkey=%u\n", mqkey);

    if (mqkey == 0)
    {
        mqkey = DEFAULT_MQ_KEY;
    }

    char rm_cmd[BUFSIZ];
    snprintf(rm_cmd, sizeof(rm_cmd), "ipcrm -Q %d 2>/dev/null", mqkey);
    system(rm_cmd);

    CCommu* commu = new CMQCommu(mqkey);
    monsrv_.set_commu(commu);
    monsrv_.set_tlog(&flog_);


    Report report;

    if (conf.getAndCheckReport(report) == 0)
    {
        report_tool = report.report_tool;
        if (report_tool.length() < 3)
            report_tool.clear();
    }

    return 0;
}
void CDefaultCtrl::clear_stat_file()
{
    char rm_cmd[256];
    snprintf(rm_cmd, sizeof(rm_cmd) - 1, "rm -rf ../stat/* 2>/dev/null");
    system(rm_cmd);
    return;
}


int CDefaultCtrl::reloadconf()
{
    CCtrlConf conf(new CLoadXML(ix_->argv_[1]));
    if (conf.init() != 0)
    {
        exit(-1);
    }

    //groupinfo配置

    Procmon procmon ;
    if (conf.getAndCheckProcmon(procmon) != 0)
    {
        exit(-1);
    }
    TGroupInfo groupinfo;
    for (size_t i = 1; i < procmon.entry.size(); ++ i)
    {
        unsigned reload = procmon.entry[i].reload;
        if(reload == 0)
            continue;

        memset(&groupinfo, 0x0, sizeof(TGroupInfo));
        groupinfo.adjust_proc_time = 0;
        groupinfo.groupid_ = procmon.entry[i].id;
        strncpy(groupinfo.basepath_, procmon.entry[i].basepath.c_str(), sizeof(groupinfo.basepath_) - 1);
        strncpy(groupinfo.exefile_, procmon.entry[i].exe.c_str(), sizeof(groupinfo.exefile_) - 1);
        strncpy(groupinfo.etcfile_, procmon.entry[i].etc.c_str(), sizeof(groupinfo.etcfile_) - 1);

        groupinfo.exitsignal_ = procmon.entry[i].exitsignal;
        groupinfo.maxprocnum_ = procmon.entry[i].maxprocnum;
        groupinfo.minprocnum_ = procmon.entry[i].minprocnum;
        groupinfo.heartbeat_ = procmon.entry[i].heartbeat;
        groupinfo.affinity_ = procmon.entry[i].affinity;
        groupinfo.reload_ = procmon.entry[i].reload;
		
		if (groupinfo.reload_ > 0)
		{
			//将1设置为待热启动状态，2设置为fork后状态，0设置为热重启结束
			groupinfo.reload_ = PROC_RELOAD_START;
			groupinfo.reload_time = time(NULL);
		}

        monsrv_.pre_reload_group(groupinfo.groupid_, &groupinfo);
    }

    return 0;
}
