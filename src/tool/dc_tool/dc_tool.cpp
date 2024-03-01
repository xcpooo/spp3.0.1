#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>

#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <dirent.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>


#include <errno.h>
#include "StatMoniMgr.h"
#include "report/report_handle.h"
#include "report/report_base.h"


#include "dcapi_cpp.h"
using namespace DataCollector;


USING_SPP_STAT_NS;

using std::vector;
using std::string;
using std::map;


#define DC_TOOL_VERSION "v0.2.0"
#define DC_TOOL_LOCK "./.dc_tool.lock"
#define DC_PID_FILE  "./dc.pid"


// DC上相关参数
#define DC_APP_NAME_1000016 "log1000016"
#define DC_TYPE_ID          (1000055)
#define DC_ERROR_TYPE_ID    (1000066)
#define DC_APP_NAME_1000113 "cdp1000113"      
#define DC_COST_TYPE_ID     (1000113)
#define DC_APP_NAME_1000270 "cdp1000270"

// 上报间隔
#define SLEEP_TIME (60/*second*/)

//每6小时上报一次版本信息
#define VERSION_REPORT_INTERVAL (6*60*60)


#define MAX_CHECK_SIZE (1024*1024)

#define SPP_WORKER      "../bin/spp_worker"
#define SPP_PID_FILE    "../bin/spp.pid"
#define SPP_STAT_TOOL   "../bin/stat_tool"
#define SPP_PROXY_STAT_FILE     "../stat/stat_spp_proxy.dat"
#define SPP_WORKER_STAT_FILE    "../stat/stat_spp_worker%d.dat"
#define CTRL_LOG_FILE   "../log/spp_frame_ctrl.log"

// 调试标志，上线可以删除
// #define I_AM_DEBUG

#define PRINTF(FMT, ARGS...) do {\
    printf("[%s][%5d] ", now_timestr(), getpid());\
    printf(FMT, ##ARGS); } while(0)


#ifdef I_AM_DEBUG
#   define DEBUG_P PRINTF
#else
#   define DEBUG_P(FMT, ARGS...) do { } while(0)
#endif

string g_iname;
int send_data_to_dc(CLogger &logger, char* buf);


/*返回码上报线程*/  
void *report_thread(void * arg)  
{
	CReport rpt;
	report_handle_t hdl;

	struct in_addr addr;
	string remote_ip;
	time_t t;
	struct timeval tv;

	int iRet = 0;
	CLogger rpt_log;

	rpt_log.init("dc00473");

	while(1)
	{
		if ((iRet = rpt.proc_report(&hdl, &tv)) < 0)
		{
			usleep(5000);
			continue;
		}

		//对比时间
		t = time(NULL);
		if (tv.tv_sec - t > 60)
		{
			cout << "ret report timeout" << endl;
			continue;
		}

		addr.s_addr = hdl.remote_ipv4;
		remote_ip = inet_ntoa(addr);

		char buf[2048] = { 0 }; //足够了
		int len = sizeof(buf);

		snprintf(buf, sizeof(buf) - 1,	
				"sip=%s&spp=%s&cmd=%s&result=%d&cost=%d",
				remote_ip.c_str(), g_iname.c_str(), hdl.cmd, hdl.result, hdl.msg_cost); 

		send_data_to_dc(rpt_log, buf);
	}
}

//是否使用微线程标记
bool g_bMT = false;

void trim(std::string &str)
{
	char buff[10*1024] = {0};
	int len = 0;

	if (str.size() > sizeof(buff))
		return;

	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n')
			continue;

		buff[len++] = str[i];
	}

	if (len)
	{
		str.assign(buff, len);
	}
}

//摘自CMisc 
unsigned getip(const char* ifstr)
{
    char ifname[16] = {0};
    if (!ifstr)
    {
        strncpy(ifname, "eth1", sizeof(ifname) - 1);
    }
    else if(0 == strncmp(ifname, "all", sizeof(ifname)))
    {
        return 0U; // all interface = IP 0.0.0.0
    }
    else
    {
        strncpy(ifname, ifstr, sizeof(ifname) -1);
    }

    register int fd, intrface;
    struct ifreq buf[10];
    struct ifconf ifc;
    unsigned ip = 0;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;

        if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
        {
            intrface = ifc.ifc_len / sizeof(struct ifreq);

            while (intrface-- > 0)
            {
                if (strcmp(buf[intrface].ifr_name, ifname) == 0)
                {
                    if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface])))
                        ip = (unsigned)((struct sockaddr_in *)(&buf[intrface].ifr_addr))->sin_addr.s_addr;

                    break;
                }
            }
        }

        close(fd);
    }

    return ip;
}

int pipe_fd[2];
FILE* rstream;

int init_pipe(void);
int do_cmd(char* const argv[]);

typedef struct {
    unsigned count;
    unsigned value;
} stat_value_t;
typedef map<string, stat_value_t> map_info_t;
/*
   取得statfile文件的key/value对
*/
int get_info(const char* statfile, map_info_t &result);


/*
   根据pid取得fd数
*/
unsigned get_fd_num(int pid);


/*
   根据pid取得内存使用 (kbytes)
*/
unsigned get_memory_usage(int pid);


/*
   检查ctrl.log文件有没dead信息
*/
int do_check_ctrl_log(void);



typedef map<int, vector<int> > group_pid_t;
/*
   取得bin目录下spp.pid的分组信息和pid
*/
int get_pid_list(group_pid_t &pids);
/*
   处理基于命令字错误码的时延上报回掉函数
 */
int proc_cost_moni_data(void* arg);

int send_data_to_dc(char* buf);

char *now_timestr(void);

CLogger logger_1000016;
CLogger logger_1000113;
CLogger logger_1000270; // spp 版本号上报
typedef struct {
    string statfile;
    unsigned tCount;        // 周期内请求数
    unsigned rspCount;      // 周期内回复数
    unsigned reqlen;        // 平均请求长度
    unsigned rsplen;        // 平均回复长度
    unsigned group;       // 组号
    unsigned spp_type;      // 同步异步类型，1 for worker同步，2 for worker异步, 0 for 未定义
    unsigned process_num;   // 进程数
    unsigned avg_fd_num;    // 平均fd数
    unsigned max_fd_num;    // 最大fd数
    unsigned avg_mem;       // 平均内存占用(Rss)
    unsigned max_mem;       // 最大内存占用(Rss)
    
    unsigned cur_connects;  // 当前连接数, 仅对proxy有效
    unsigned msg_shm_time;  // 消息在共享内存中的时延(ms), 仅对worker有效
    unsigned msg_shm_count; // 消息在共享内存个数, 仅对worker有效
    // 下面字段用于异常上报 
#define PROXY_CONN_OVERLOAD reused_1
#define PROXY_SHM_ERROR     reused_2

#define WORKER_MSG_TIMEOUT  reused_1
    unsigned reused_1;
    unsigned reused_2;

    
} moni_data_t;

typedef struct {
	std::string str_spp_ver;
	std::string str_mt_ver;
	std::string str_l5_ver;
	std::string str_ip;
} version_data_t;

int get_version_data(version_data_t &data);
int do_version_report(version_data_t &data);


/*
   从stat文件取得数据
*/
int get_data_from_stat(moni_data_t &data);

bool g_isRun = true;

void sig_handler_exit(int signo)
{
    g_isRun = false;
    PRINTF("recv exit signal[%d]\n", signo);
}

void sig_handler_comm(int signo)
{
    PRINTF("recv signal[%d]", signo);
}

typedef map <int, int> map_dead_info; 
map_dead_info g_groups_dead_info;
/*
 * 从buf读取dead信息
 */
int get_dead_info(const char* buf, map_dead_info& info);

int send_to_dc(CLogger &logger, moni_data_t &data);

void lock_process(int *fd)
{
    *fd = open(DC_TOOL_LOCK, O_WRONLY | O_CREAT, 0666);
    fcntl(*fd, F_SETFD, FD_CLOEXEC);
    if (flock(*fd, LOCK_EX | LOCK_NB) == -1
            && errno == EWOULDBLOCK)
    {
        close(*fd);
        printf("dc_tool is running, please stop before start.\n");
        exit(-1);
    }
}

// add for nest pid check 2014
void write_self_pid()
{
    int fd = open(DC_PID_FILE, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        return;
    }

    char pid_str[256] = {0};
    snprintf(pid_str, sizeof(pid_str), "%d [DC_REPORT]\n", getpid());
    
    flock(fd, LOCK_EX);
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    write(fd, pid_str, strlen(pid_str));
    flock(fd, LOCK_UN);

    close(fd);
    return;
}


int main(int argc, char *argv[])
{
    int ret;
	time_t last_version_report_time = 0;

    if (argc == 2 && strncmp(argv[1], "-v", 2) == 0)
    {
        printf("\nSPP_DC_TOOL_"DC_TOOL_VERSION" Make date: %s\n\n", __DATE__);
        _exit(0);
    }
    
    int fd = -1;
    lock_process(&fd);

    char fullpath[512] = "NULL";
    // 取得父目录
    if (NULL == getcwd(fullpath, sizeof(fullpath)) 
            || NULL == dirname(fullpath))
    {
        PRINTF("getcwd/dirname error path[%s]\n", fullpath);
        return -10;
    }
    // 提取父目录
    char* bname = basename(fullpath);
    if (bname == NULL)
        PRINTF("get basename error. path[%s]\n", fullpath);	
    g_iname = bname; // SPP实例名（用bin的父目录作为实例名）

    string appname = DC_APP_NAME_1000016;
    logger_1000016.init(appname);
    appname = DC_APP_NAME_1000113;
    logger_1000113.init(appname);
	appname = DC_APP_NAME_1000270;
	logger_1000270.init(appname);

    signal(SIGUSR1, sig_handler_exit);
    signal(SIGUSR2, sig_handler_exit);
    signal(SIGPIPE, sig_handler_comm);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_DFL);

    ret = init_pipe();
    if (ret != 0) {
        PRINTF("init_pipe error [%m]\n");
        return -8;
    }


	//创建返回码上报线程
	pthread_t tid;
	if (pthread_create(&tid, NULL, report_thread, NULL) != 0)
	{
		printf("create report pthread error!\n");  
		return -1;  
	}

	
    PRINTF("SPP DC tool started...\n");

    // 先执行一下do_check_ctrl_log
    ret = do_check_ctrl_log();
    if (ret != 0) {
        PRINTF("do_check_ctrl_log return[%d] %m\n", ret);
        return -1;
    }
    g_groups_dead_info.clear();

    map<int, moni_data_t> groups;
    map<int, moni_data_t> groups_old;

    // BEGIN，确定有哪些组
    if (access(SPP_PROXY_STAT_FILE, R_OK) != 0) {
        PRINTF("access "SPP_PROXY_STAT_FILE" error [%m]\n");
        return -2;
    }

    groups[0].statfile = SPP_PROXY_STAT_FILE;
    groups[0].group = 0; 

    char buf[512];
    for (int i = 1; i <= 256; ++i) {
        snprintf(buf, sizeof(buf), SPP_WORKER_STAT_FILE, i);
        if (access(buf, R_OK) == 0) {
            groups[i].statfile = buf;
            groups[i].group = i;
            DEBUG_P("FOUND Worker stat [%s]\n", buf);
        }
    }

    groups_old = groups;

    // 测试读取
    map<int, moni_data_t>::iterator group_it;
    for (group_it = groups_old.begin(); group_it != groups_old.end(); ++group_it)
    {
        ret = get_data_from_stat(group_it->second);
        if (ret) return -3;
    }
    // END
    CStatMoniMgr monimgr;
    ret = monimgr.init();
    if (ret != 0)
    {
        return -3; 
    }
    monimgr.regCallBack(proc_cost_moni_data);

    DEBUG_P("daemon !\n");

    if (argc == 2 && strncmp(argv[1], "-d", 2) == 0)
    {
        daemon(1, 1);
        write_self_pid();   // nest need pid file 20140506
    }
    else
    {
        signal(SIGHUP, sig_handler_exit);
    }


    while (g_isRun && !groups.empty()) {
        // 收集一周期proxy/worker的入出包、入出Bytes、Conn Error、Shm Error

        DEBUG_P("BEGIN Sleep(%d)\n", SLEEP_TIME);
        int sleep_time = SLEEP_TIME;
        while (sleep_time > 0) {
            fflush(stdout);
            //fsync(STDOUT_FILENO);

            sleep(2);
            sleep_time -= 2;
            if (!g_isRun) goto SPP_DC_TOOL_END;

            ret = do_check_ctrl_log();
            if (ret == 444) {
                PRINTF("do_check_ctrl_log[%d]!\n", ret); // 注，如果这里输出dead会跟形成逻辑死循环
            }
            else if (ret != 0) {
                PRINTF("do_check_ctrl_log[%d] error [%m]\n", ret);
                goto SPP_DC_TOOL_END;
            }
        }

        // 收集下一周期数据
        for (group_it = groups.begin(); group_it != groups.end(); ++group_it)
        {

            ret = get_data_from_stat(group_it->second);
            if (ret) goto SPP_DC_TOOL_END;

            moni_data_t bak_data = group_it->second; // 保存当前group的值
            moni_data_t &new_data = group_it->second;
            moni_data_t &old_data = groups_old[group_it->first];
#define UPDATE_NEW_DATA(VAR) new_data.VAR -= old_data.VAR
#define CACULATE_AVERAGE(AVG,SUM,DIV) do {\
            if(new_data.DIV > 0) \
            { \
                new_data.AVG = new_data.SUM / new_data.DIV; \
            } \
            else \
            { \
                new_data.AVG = 0; \
            } \
            }while(0)
#ifdef I_AM_DEBUG
            printf("get tCount: last[%u] cur[%u]\n", old_data.tCount, new_data.tCount);
            printf("get rspCount: last[%u] cur[%u]\n", old_data.rspCount, new_data.rspCount);
            printf("get reqlen: last[%u] cur[%u]\n", old_data.reqlen, new_data.reqlen);
            printf("get rsplen: last[%u] cur[%u]\n", old_data.rsplen, new_data.rsplen);
#endif
            if (new_data.group == 0)
            {
#ifdef I_AM_DEBUG
                printf("get cur_connects:[%u]\n", new_data.cur_connects);
#endif
            }
            else
            {
#ifdef I_AM_DEBUG
                printf("get msg_shm_count: last[%u] cur[%u]\n", old_data.msg_shm_count, new_data.msg_shm_count);
                printf("get msg_shm_time: last[%u] cur[%u]\n", old_data.msg_shm_time, new_data.msg_shm_time);
#endif
                UPDATE_NEW_DATA(msg_shm_count);
                UPDATE_NEW_DATA(msg_shm_time);
#ifdef I_AM_DEBUG
                printf("caculate avg: msg_shm_time[%u] msg_shm_count[%u]\n", new_data.msg_shm_time, new_data.msg_shm_count);
#endif
                CACULATE_AVERAGE(msg_shm_time, msg_shm_time, msg_shm_count);
            }
            UPDATE_NEW_DATA(tCount);    // 开始diff
            UPDATE_NEW_DATA(rspCount);
            UPDATE_NEW_DATA(reqlen);

            
#ifdef I_AM_DEBUG
            printf("caculate avg: reqlen[%u] tCount[%u]\n", new_data.reqlen, new_data.tCount);
#endif
            CACULATE_AVERAGE(reqlen, reqlen, tCount);
            UPDATE_NEW_DATA(rsplen);
#ifdef I_AM_DEBUG
            printf("caculate avg: rsplen[%u] rspCount[%u]\n", new_data.rsplen, new_data.rspCount);
#endif
            CACULATE_AVERAGE(rsplen, rsplen, rspCount);
            UPDATE_NEW_DATA(reused_1);
            UPDATE_NEW_DATA(reused_2);
            old_data = bak_data; // 把当前group值覆盖到old_data (for下周期)
        }

        // 上报异常数据（如有）

        // 读取pid，读取每个分组的fd、内存量
        group_pid_t pids;
        ret = get_pid_list(pids);
        for (group_it = groups.begin(); group_it != groups.end(); ++group_it) {
            vector<int> &group_pid = pids[group_it->first];
            moni_data_t &data = group_it->second;
            data.process_num = group_pid.size();
            data.avg_fd_num = 0;
            data.max_fd_num = 0;
            data.avg_mem = 0;
            data.max_mem = 0;
            
            for (int i = 0; i < group_pid.size(); ++i) {
                unsigned fd_num = get_fd_num(group_pid[i]);
                unsigned mem = get_memory_usage(group_pid[i]);

                data.avg_fd_num += fd_num;
                data.avg_mem += mem;

                if (fd_num > data.max_fd_num)
                    data.max_fd_num = fd_num;

                if (mem > data.max_mem)
                    data.max_mem = mem;
            }

            if (group_pid.size() > 0) {
                data.avg_fd_num /= group_pid.size();
                data.avg_mem /= group_pid.size(); 
            }

            send_to_dc(logger_1000016, data);


#ifdef I_AM_DEBUG
            printf("\n");
            PRINTF("GROUPID     [%u]\n", data.group);
            PRINTF("tCount      [%u]\n", data.tCount);
            PRINTF("rspCount    [%u]\n", data.rspCount);
            PRINTF("reqlen      [%u]\n", data.reqlen);
            PRINTF("rsplen      [%u]\n", data.rsplen);
            PRINTF("spp_type    [%u]\n", data.spp_type);
            PRINTF("process_num [%u]\n", data.process_num);
            PRINTF("avg_fd_num  [%u]\n", data.avg_fd_num);
            PRINTF("max_fd_num  [%u]\n", data.max_fd_num);
            PRINTF("avg_mem     [%u]\n", data.avg_mem);
            PRINTF("max_mem     [%u]\n", data.max_mem);
            PRINTF("reused_1    [%u]\n", data.reused_1);
            PRINTF("reused_2    [%u]\n", data.reused_2);
            PRINTF("cur_connects[%u]\n", data.cur_connects);
            PRINTF("msg_shm_time[%u]\n", data.msg_shm_time);

            printf("\n");
#endif

			if (data.spp_type == 3)
				g_bMT = true;

        }

		time_t now = time(NULL);
		if ((now - last_version_report_time) >= VERSION_REPORT_INTERVAL)
		{
			last_version_report_time = now;
		
			version_data_t data;
				
			get_version_data(data);
		
			do_version_report(data);
		}

        monimgr.run();
    }

SPP_DC_TOOL_END:
    PRINTF("SPP DC tool exited...\n");
    close(fd);
    unlink(DC_TOOL_LOCK);
    return ret;
}


char *now_timestr(void)
{
    static char str_tm[1024];
    struct tm tmm;

    time_t now = time(NULL);
    memset(&tmm, 0, sizeof(tmm) );
    localtime_r((time_t *)&now, &tmm);

    snprintf(str_tm, sizeof(str_tm), "%04d-%02d-%02d %02d:%02d:%02d",
            tmm.tm_year + 1900, tmm.tm_mon + 1, tmm.tm_mday,
            tmm.tm_hour, tmm.tm_min, tmm.tm_sec);

    return str_tm;
}

int init_pipe(void)
{
    if (pipe(pipe_fd) == 0) {
        int opts;
        opts = fcntl(pipe_fd[0], F_GETFL);
        if(opts < 0) {
            return -1;
        }
        opts = opts | O_NONBLOCK;
        if(fcntl(pipe_fd[0], F_SETFL, opts) < 0) {
            return -1;
        }

        rstream = fdopen(pipe_fd[0], "r");
        if (rstream == NULL)
        {
            return -2;

        }

        return 0;
    }
    return -3;
}

int do_cmd(char* const argv[])
{
    int pid = fork();
    if (pid == 0) {
        dup2(pipe_fd[1], STDOUT_FILENO);
        if (execv (argv[0], argv)) {
            _exit(4);
        }
    }
    return pid;
}

int get_info(const char* statfile, map_info_t &result)
{
    int ret;
    int pid;

    char *cmd[] = {(char *)SPP_STAT_TOOL, (char *)statfile, (char *)0};
    pid = do_cmd(cmd);
    if (pid < 0) return -4;

    int status;
    waitpid(pid, &status, 0);
    uint8_t retcode = WEXITSTATUS(status);
    if (WIFEXITED(status) && retcode == 0) {
        char* line = NULL;
        size_t len = 0;
        int isStart = 0;
        while (getline(&line, &len, rstream) != -1) {
            if (!isStart) {
                if (len > 6 && strncmp(line, "StatID", 6) == 0)
                    isStart = 1;

                continue;
            }

            char statID[256];
            stat_value_t val;

            ret = sscanf(line, "%s%*[^-0-9]%d%*[^-0-9]%d", statID, &val.count, &val.value);
            if (ret != 3)
                continue;

            result[statID] = val;
        }
        if (line) free(line);
        clearerr(rstream);
    } else if (retcode == 4) {
        PRINTF("execv stat_tool error!\n");
    } else {
        PRINTF("stat_tool exit error!\n");
    }

    return retcode;
}
int get_dead_info(const char* buf, map_dead_info& info)
{
    int dead_cnt = 0;
    std::stringstream ss(buf);
    while (ss)
    {
        std::string str_line;
        std::getline(ss, str_line, '\n');
        if (str_line.find("dead") != string::npos)
        {
            size_t group_idx = str_line.find("group[");
            if (group_idx != string::npos)
            {
                int id= 0;
                bool flag = 0;
                for (size_t i = group_idx + 6; i < str_line.length(); ++ i)
                {
                    if (str_line[i] >= '0' && str_line[i] <= '9')
                    {
                        id = id * 10 + str_line[i] - '0';
                    }
                    else
                    {
                        flag = 1;
                        break;
                    }
                }
                if (flag)
                {
                    info[id] ++;
                    dead_cnt ++;
                }
            }
                     
        }
    }
    return dead_cnt;
}
unsigned get_memory_usage(int pid)
{
    unsigned ret = 0;
    size_t len = 0;
    char* line = NULL;
    FILE* file = NULL;
    char path[32];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    file = fopen(path, "r");
    if (file == NULL) return 0;

    while (getline(&line, &len, file) != -1) {
        if (len < 5 || strncmp(line, "VmRSS", 5) != 0)
            continue;

        sscanf(line, "%*[^0-9]%u", &ret);
    }

    if (line) free(line);

    fclose(file);
    return ret;

}

unsigned get_fd_num(int pid)
{
    int count = 0;
    DIR *dir;
    struct dirent *ent;

    char path[32];
    snprintf(path, sizeof(path), "/proc/%d/fd", pid);

    if (NULL == (dir = opendir(path)))
        return 0;

    while (NULL != (ent = readdir(dir)))
        count++;

    closedir(dir);

    return count - 2;
}

int do_check_ctrl_log()
{
    int ret, fd = 0;
    char* buf = NULL;
    size_t diff;
    static long last_size = -1;

    struct stat st;
    stat(CTRL_LOG_FILE, &st);
    long cur_size = st.st_size;

    DEBUG_P(CTRL_LOG_FILE" last_size[%u] cur_size[%u]\n", last_size, cur_size);

    if (last_size == cur_size) {
        ret = 0;
        goto END;
    }

    if (last_size < 0) { // 第一次，OR 出错
        last_size = cur_size;
        ret = 0;
        goto END;
    }

    if (last_size > cur_size)
        last_size = 0;

    diff = (cur_size - last_size) % MAX_CHECK_SIZE;
    buf = (char*)malloc(diff + 2);
    if (buf == NULL) {
        ret = -2;
        goto END;
    }

    fd = open(CTRL_LOG_FILE, O_RDONLY);
    if (fd < 0) {
        ret = -4;
        goto END;
    }

    lseek(fd, last_size, SEEK_SET);
    ret = read(fd, buf, diff);
    if (ret < 0) {
        ret = -3;
        goto END;
    }
    buf[ret] = 0;

    last_size += ret;

    /*
    if (strstr(buf, "dead") != NULL) {
        // 寻找新增的日志里，是否有dead信息
        ret = 444;
        goto END;
    }
    */
    if (get_dead_info(buf, g_groups_dead_info) != 0) 
    {
        ret = 444;
        goto END;
    }

    ret = 0;
END:
    if (buf != NULL) free(buf);
    if (fd > 0) close(fd);
    return ret;

}

int get_pid_list(group_pid_t &pids)
{
    static group_pid_t cache;
    static time_t cache_time = 0;

    struct stat st;
    if (0 != stat(SPP_PID_FILE, &st)) return -1;

    if (st.st_mtime == cache_time) {
        pids = cache;
        return 0;
    }

    FILE* file = fopen(SPP_PID_FILE, "r");
    if (file == NULL) return -1;

    flock(fileno(file), LOCK_EX);

    int group = -1;
    size_t len = 0;
    char* line = NULL;
    while (getline(&line, &len, file) != -1) {
        if (len < 3) continue;

        if (len > 7 && strncmp(line, "GROUPID", 7) == 0) {
            sscanf(line, "%*[^0-9]%d", &group);
        }

        if (group < 0)
            continue;

        int pid;
        if (1 == sscanf(line, "%d", &pid)) {
            pids[group].push_back(pid);
        }
    }
    if (line) free(line);
    cache_time = st.st_mtime;
    cache = pids;

    fclose(file); // auto unlock
    return 0;
}


int get_data_from_stat(moni_data_t &data)
{
    int ret;
    map_info_t info;
    ret = get_info(data.statfile.c_str(), info);
    if (ret != 0) return ret;

    data.reused_1 = 0;
    data.reused_2 = 0;

    map_info_t::iterator it;
    if (data.group == 0) {
        // proxy
        it = info.find("rx_bytes");
        if (it == info.end()) it = info.find("ator_recv_num");
        if (it == info.end())
        {
            PRINTF("Get rx_bytes/ator_recv_num In %s Error\n", data.statfile.c_str());
            return -1;
        }
        data.tCount = (it->second).count;
        data.reqlen = (it->second).value;

        it = info.find("tx_bytes");
        if (it == info.end()) it = info.find("ctor_recv_num");
        if (it == info.end())
        {
            PRINTF("Get tx_bytes/ctor_recv_num In %s Error\n", data.statfile.c_str());
            return -2;
        }   
        data.rspCount = (it->second).count;
        data.rsplen = (it->second).value;

        it = info.find("conn_overload");
        if (it != info.end())
            data.PROXY_CONN_OVERLOAD = (it->second).count;
        it = info.find("shm_error");
        if (it != info.end())
            data.PROXY_SHM_ERROR = (it->second).count;
        it = info.find("cur_connects");
        if (it != info.end())
            data.cur_connects = (it->second).value;

        data.spp_type = 0;
        data.msg_shm_time = 0;
        data.msg_shm_count = 0;
    } else {
        // worker
        it = info.find("shm_rx_bytes");
        if (it == info.end()) it = info.find("ator_recv_num");
        if (it == info.end())
        {
            PRINTF("Get shm_rx_bytes/ator_recv_num In %s Error\n", data.statfile.c_str());
            return -3;
        }   
        data.tCount = (it->second).count;
        data.reqlen = (it->second).value;

        it = info.find("shm_tx_bytes");
        if (it == info.end()) it = info.find("ator_send_num");
        if (it == info.end())
        {
            PRINTF("Get shm_tx_bytes/ator_send_num In %s Error\n", data.statfile.c_str());
            return -4;
        }   
        data.rspCount = (it->second).count;
        data.rsplen = (it->second).value;

        it = info.find("worker_type");
        if (it != info.end()) data.spp_type = (it->second).value;
        it = info.find("msg_timeout");
        if (it != info.end()) data.WORKER_MSG_TIMEOUT = (it->second).value;
        it = info.find("msg_shm_time");
        if (it != info.end()) 
        {
            data.msg_shm_time = (it->second).value;
            data.msg_shm_count = (it->second).count;

        }
        data.cur_connects = 0;
    }

    return 0;

}
int send_data_to_dc(CLogger &logger, char* buf)
{
    DEBUG_P("DC STR : %s\n", buf);
    string up_str = buf; 
    char type = LT_NORMAL;
    int ret = logger.write_baselog(type, up_str, true); 
    if (ret != 0) { 
        PRINTF("[%d] write_baselog error [%d]", __LINE__,ret);
        return ret;
    }
    return 0;
}
int send_to_dc(CLogger &logger, moni_data_t &data)
{
    int ret;
    char buf[2048]; //足够了
    time_t now = time(NULL);
    snprintf(buf, 2047,
            "version=0&typeid=%u&time=%u&iName=%s&tCount=%u&"
            "rspCount=%u&reqlen=%u&rsplen=%u&group=%u&"
            "spp_type=%u&process_num=%u&avg_fd_num=%u&max_fd_num=%u&"
            "avg_mem=%u&max_mem=%u&"
            "cur_connects=%u&"
            "msg_shm_time=%u" ,
            DC_TYPE_ID, now, g_iname.c_str(), data.tCount,
            data.rspCount, data.reqlen, data.rsplen, data.group,
            data.spp_type, data.process_num, data.avg_fd_num, data.max_fd_num,
            data.avg_mem, data.max_mem,
            data.cur_connects,
            data.msg_shm_time);

    send_data_to_dc(logger_1000016, buf);

    // 上报异常日志
    int scnt = snprintf(buf, 2047,
            "version=0&typeid=%u&time=%u&iName=%s&", 
            DC_ERROR_TYPE_ID, now, g_iname.c_str());

    if (data.reused_1 != 0) {
        string error;
        if (data.group == 0) { // proxy
            error = "CONN_FLOW_OVERLOAD";
        } else {
            error = "MSG_TIMEOUT_DROP";
        }
            snprintf(buf + scnt, 2047 - scnt,
                    "fName=%s&group=%u&count=%u", 
                    error.c_str(), data.group, data.reused_1);
        send_data_to_dc(logger_1000016, buf);
    }

    if (data.reused_2 != 0) {
        if (data.group == 0) { // proxy
            snprintf(buf + scnt, 2047 - scnt, "fName=%s&group=%u&count=%u",
                    "SHARE_MEMORY_ERROR", data.group, data.reused_2);
            send_data_to_dc(logger_1000016, buf);
        }
    }

    if (g_groups_dead_info.find(data.group) != g_groups_dead_info.end())
    {
        snprintf(buf + scnt, 2047 - scnt, "fName=%s&group=%u&count=%u",
                        "PROCESS_DEAD", data.group, g_groups_dead_info[data.group]);
        send_data_to_dc(logger_1000016, buf);
        g_groups_dead_info.erase(data.group);
    }
    
}

int proc_cost_moni_data(void* argv)
{
    moni_code_data_t *data = (moni_code_data_t*) argv;

    int ret;
    char buf[2048]; //足够了
    int buf_len = sizeof(buf);
    time_t now = time(NULL);
    int scnt = snprintf(buf, buf_len - 1,                  
            "version=0&typeid=%u&time=%u&iName=%s&group=%u&cmdid=%u&code=%u&count=%u",
            DC_COST_TYPE_ID, now, g_iname.c_str(), 
            data->group,
            data->cmdid,
            data->code,
            data->count); 
    //业务命令字上报, 包括平均时延和时延分布
    if (data->cmdid < MAX_PLUGIN_CMD_NUM 
            || data->cmdid == FRAME_REQ_STAT_CMD_ID)
    {
        scnt += snprintf(buf+scnt, buf_len - 1 - scnt, "&timecost=%u&timemap=",data->timecost);
        for (unsigned  i = 0; i < MAX_TIMEMAP_SIZE; ++ i)
        {
            //最后个不能加分号
            if (i != MAX_TIMEMAP_SIZE - 1)
            {
                scnt += snprintf(buf + scnt, buf_len - 1 - scnt, 
                        "%u:%u;", i, data->timemap[i]);
            }
            else
            {
                scnt += snprintf(buf + scnt, buf_len - 1 - scnt,
                        "%u:%u", i, data->timemap[i]);
            }
        }
        send_data_to_dc(logger_1000113, buf);
    }
    else if (data->cmdid == FRAME_CODE_STAT_CMD_ID)
    {
        send_data_to_dc(logger_1000113, buf);
    }
    return 0;
}

int get_version_data(version_data_t &data)
{
    int ret;
    int pid;

    char *cmd[] = {(char *)SPP_WORKER, (char *)"-v", (char *)0};
    pid = do_cmd(cmd);
    if (pid < 0) return -4;

    int status;
    waitpid(pid, &status, 0);
    uint8_t retcode = WEXITSTATUS(status);
    if (WIFEXITED(status) && retcode == 0) {
        char* line = NULL;
        size_t len = 0;
        int isStart = 0;
        while (getline(&line, &len, rstream) != -1) {
        	if (len > 12 && strncmp(line, "spp version:", 12) == 0)
                data.str_spp_ver.assign(&line[12]);

        	if (len > 11 && strncmp(line, "mt version:", 11) == 0)
                data.str_mt_ver.assign(&line[11]);
			
        	if (len > 11 && strncmp(line, "l5 version:", 11) == 0)
                data.str_l5_ver.assign(&line[11]);
        }
        if (line) free(line);
        clearerr(rstream);
    } else if (retcode == 4) {
        PRINTF("execv stat_tool error!\n");
    } else {
        PRINTF("stat_tool exit error!\n");
    }

	trim(data.str_spp_ver);
	trim(data.str_mt_ver);
	trim(data.str_l5_ver);

	struct in_addr stIP;

	stIP.s_addr = getip("eth1");

	data.str_ip = inet_ntoa(stIP);

    return retcode;
}

int do_version_report(version_data_t &data)
{
    char buf[2048] = { 0 }; //足够了
    
    int buf_len = sizeof(buf);

	if (!g_bMT)
	{
		data.str_mt_ver = "mt_0.0.0";
	}
	
    snprintf(buf, sizeof(buf) - 1,                  
            "typeid=1000270&time=%u&ip=%s&spp=%s&spp_ver=%s&mt_ver=%s&l5_api_ver=%s&count=1",
            time(NULL), data.str_ip.c_str(), g_iname.c_str(), data.str_spp_ver.c_str(),
            data.str_mt_ver.c_str(), data.str_l5_ver.c_str()); 

    send_data_to_dc(logger_1000270, buf);
  
    return 0;
}

