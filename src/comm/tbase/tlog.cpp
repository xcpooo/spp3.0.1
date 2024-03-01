#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <linux/unistd.h>
#include <errno.h>
#include "tlog.h"
#include "time_convert.h"
#include "logsys_client.h"
#include "singleton.h"

using namespace spp::singleton;

#if !__GLIBC_PREREQ(2, 3)
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

using namespace tbase::tlog;

#ifdef _THREAD_SAFE
pthread_mutex_t CTLog::CLogLock::mutex_ = PTHREAD_MUTEX_INITIALIZER;
#endif

#define SPRINT(buff, len, fmt, args...)   do{len += snprintf(buff + len, MAX_LOG_LEN - len - 1, fmt, ##args);\
					     	len=(len<MAX_LOG_LEN-1?len:MAX_LOG_LEN-1);} while(0);

class ErrnoGuard
{
public:
	ErrnoGuard():err(errno)
	{}
	~ErrnoGuard()
	{
		errno = err;
	}
private:
	int err;
};

CTLog::CTLog(): log_level_(0), log_type_(0), max_file_size_(DEFAULT_MAX_FILE_SIZE), max_file_no_(DEFAULT_MAX_FILE_NO), log_fd_(ERR_FD_NO), cur_file_size_(0), cur_file_no_(0), pre_time_(0), hook_(NULL)
{
    memset(log_path_, 0x0, MAX_PATH_LEN);
    memset(name_prefix_, 0x0, MAX_PATH_LEN);
    memset(buffer_, 0x0, MAX_LOG_LEN);

}
CTLog::~CTLog()
{
    close_file();
}
inline void CTLog::get_time(int& buff_len)
{
    time_t now = time(NULL);
    struct tm tmm;
    localtime_r_nl(&now, &tmm);

    SPRINT(buffer_, buff_len, "[%04d-%02d-%02d %02d:%02d:%02d]", tmm.tm_year + 1900, tmm.tm_mon + 1, tmm.tm_mday, tmm.tm_hour, tmm.tm_min, tmm.tm_sec);

}
inline void CTLog::get_tid(int& buff_len)
{
    pid_t tid = syscall(__NR_gettid);
    SPRINT(buffer_, buff_len, "[%-5d]", tid);
}
inline void CTLog::get_level(int& buff_len, int log_level)
{
    static char* level_str[] = {"TRACE", "DEBUG", "NORMAL", "ERROR", "FATAL"};
    SPRINT(buffer_, buff_len, "[%-6s]", level_str[log_level]);
}
void CTLog::log_file_name(char* filepath, char* filename)
{
    char timestr[64] = {0};
    struct tm tmm;
    localtime_r_nl((time_t*) & (pre_time_), &tmm);
    snprintf(timestr, sizeof(timestr) - 1, "%04d%02d%02d%02d%02d%02d", tmm.tm_year + 1900, tmm.tm_mon + 1, tmm.tm_mday, tmm.tm_hour, tmm.tm_min, tmm.tm_sec);

    switch (log_type_)
    {
    case LOG_TYPE_DAILY:
    case LOG_TYPE_CYCLE_DAILY:
    {
        timestr[8] = '.';
        timestr[9] = 0;
        break;
    }
    case LOG_TYPE_HOURLY:
    case LOG_TYPE_CYCLE_HOURLY:
    {
        timestr[10] = '.';
        timestr[11] = 0;
        break;
    }
#ifdef TEST_LOG
    case LOG_TYPE_CYCLE_MIN:
    case LOG_TYPE_MIN:
    {
        timestr[12] = '.';
        timestr[13] = 0;
        break;
    }
#endif
    default:
    {
        timestr[0] = 0;
        break;
    }
    }

    if (likely(filepath != NULL))
    {
        snprintf(filepath, MAX_PATH_LEN - 1, "%s/%s.%slog", log_path_, name_prefix_, timestr);

        if (likely(filename != NULL))
        {
            strncpy(filename, filepath + strlen(log_path_) + 1, MAX_PATH_LEN - 1);
        }
    }
}
int CTLog::open_file()
{
    char filepath[MAX_PATH_LEN] = {0};
    log_file_name(filepath, filename_);

    if (likely(((log_fd_ = open(filepath, O_CREAT | O_RDWR | O_APPEND, 0666)) > 0)
               && ((cur_file_size_ = lseek(log_fd_, 0, SEEK_END)) >= 0)))
    {
        int flag = fcntl(log_fd_, F_GETFD);
        if ( flag >= 0 )
        {
            fcntl( log_fd_, F_SETFD, flag | FD_CLOEXEC );
        } 
        return 0;
    }
    else
    {
        cur_file_size_ = 0;
        log_fd_ = ERR_FD_NO;
        return -1;
    }
}
void CTLog::init_cur_file_no() //初始化日志文件编号
{
    char base_file[MAX_PATH_LEN] = {0};
    char file_name[MAX_PATH_LEN] = {0};
    log_file_name(base_file, file_name);
    cur_file_no_ = find_max_file_no(file_name);
    return;
}
void CTLog::update_cur_file_no()
{
    cur_file_no_ = find_max_file_no(filename_);
    return;
}
unsigned int CTLog::find_max_file_no(const char* filename)
{
    unsigned int filename_len = strlen(filename);
    struct dirent* ptr = NULL;
    DIR* dir = opendir(log_path_);
    unsigned int file_no = 0;

    if (NULL == dir)
    {
        return file_no;
    }

    while ((ptr = readdir(dir)) != NULL)
    {
        if (strncmp(filename, ptr->d_name, filename_len) == 0)
        {
            if (strlen(ptr->d_name) != filename_len)
            {
                unsigned int tmp_no = strtoul(ptr->d_name + filename_len + 1, NULL, 10);
                if (tmp_no > file_no)
                    file_no = tmp_no;
            }
        }
    }
    closedir(dir);
    return file_no + 1;

}
void CTLog::force_rename(const char* src_pathname, const char* dst_pathname)
{
    unlink(dst_pathname);
    rename(src_pathname, dst_pathname);
}
void CTLog::remove_file(const char *filename)
{
    unlink(filename);
    return;
}
void CTLog::close_file()
{
    if (likely(log_fd_ > 0))
    {
        if (LOG_TYPE_NET_LOG_ULS == log_type_)
        {
            LsLogFsClose(log_fd_);
        }
        else
        {
            close(log_fd_);
        }

        log_fd_ = ERR_FD_NO;
    }
}
const char* CTLog::to_hex(const char* bin, int len, char* buf, int size)
{
    for (int i = 0; i < len; ++i)
    {
        size -= i * 3;
        snprintf(buf + i * 3, size - 1, "%02X ", bin[i]);
    }

    return buf;
}

class CFlock
{// CTLog::shift_file() 使用此类
public:
    CFlock( int fd ): _fd(fd)
    { 
        ::flock(fd, LOCK_EX);
    }
    ~CFlock()
    {
        ::flock(_fd, LOCK_UN);
    }
    int _fd;
};

int CTLog::shift_file()
{
    bool need_shift = false;
    int new_file_no = 0;
    struct tm cur_tm;
    struct tm pre_tm;

    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        return 0;
    }

	ErrnoGuard err_guard;

    time_t cur_time = time(NULL);
    localtime_r_nl(&cur_time, &cur_tm);
#ifndef _MP_MODE
    localtime_r_nl(&pre_time_, &pre_tm);
#endif

#ifdef _MP_MODE
    CFlock lock_fd_in_this_block( log_fd_ ); // 文件锁

    char cur_file[MAX_PATH_LEN] = {0};
    snprintf(cur_file, sizeof(cur_file) - 1, "%s/%s", log_path_, filename_);
    struct stat st;
    struct stat st2;
    if ( stat(cur_file, &st) != 0 )
    {
        // 已经有其他进程rename，但文件还没创建
        pre_time_ = cur_time;
        close_file();
        return open_file();
    }
    else if ( fstat(log_fd_, &st2) == 0 )
    {
        if ( st.st_ino == st2.st_ino )
        {
            // 相同文件 
            cur_file_size_ = st.st_size;
            pre_time_ = st.st_mtime;
            localtime_r_nl(&pre_time_, &pre_tm); 

        }
        else
        {
            // 不同文件
            pre_time_ = cur_time;
            close_file();
            return open_file();
        }
    }
    else
    {
        pre_time_ = cur_time;
        close_file();
        return open_file();
    }
#endif

    switch (log_type_)
    {
    case LOG_TYPE_DAILY:
    {
        if (unlikely(cur_tm.tm_yday != pre_tm.tm_yday))
        {
            need_shift = true;
            new_file_no = 1;
        }

        break;
    }
    case LOG_TYPE_HOURLY:
    {
        if (unlikely((cur_tm.tm_yday != pre_tm.tm_yday) || (cur_tm.tm_hour != pre_tm.tm_hour)))
        {
            need_shift = true;
            new_file_no = 1;
        }

        break;
    }
    case LOG_TYPE_CYCLE_DAILY:
    {
        if (unlikely(cur_tm.tm_yday != pre_tm.tm_yday))
        {
            need_shift = true;
            new_file_no = 1;
        }

        break;
    }
    case LOG_TYPE_CYCLE_HOURLY:
    {
        if (unlikely((cur_tm.tm_yday != pre_tm.tm_yday) || (cur_tm.tm_hour != pre_tm.tm_hour)))
        {
            need_shift = true;
            new_file_no = 1;
        }

        break;
    }
#ifdef TEST_LOG
    case LOG_TYPE_MIN:
    case LOG_TYPE_CYCLE_MIN:
    {
        if (unlikely((cur_tm.tm_yday != pre_tm.tm_yday) || (cur_tm.tm_hour != pre_tm.tm_hour) || (cur_tm.tm_min != pre_tm.tm_min)))
        {
            need_shift = true;
            new_file_no = 1;
        }
        break;
    }
#endif

    default:
        break;
    }

    if (likely(!need_shift && cur_file_size_ < max_file_size_))
    {
        return 0;
    }

#ifdef _THREAD_SAFE
    CLogLock guard;
#endif

#ifndef _MP_MODE
    if (likely(cur_file_size_ >= max_file_size_))
    {
        if (need_shift == false)
        {
            new_file_no = cur_file_no_ + 1;
        }
    }
#endif

#ifdef _MP_MODE
    update_cur_file_no();

#endif

    char src_file[MAX_PATH_LEN] = {0};
    char dst_file[MAX_PATH_LEN] = {0};
    char remv_file[MAX_PATH_LEN] = {0};
    snprintf(src_file, sizeof(src_file) - 1, "%s/%s", log_path_, filename_);
    snprintf(dst_file, sizeof(dst_file) - 1, "%s.%u", src_file, cur_file_no_);
    force_rename(src_file, dst_file);
    if (((LOG_TYPE_CYCLE == log_type_) || (LOG_TYPE_CYCLE_DAILY == log_type_) || (LOG_TYPE_CYCLE_HOURLY == log_type_) ))
    {
        snprintf(remv_file, sizeof(remv_file) - 1, "%s.%u", src_file, cur_file_no_ - max_file_no_);
        remove_file(remv_file);
    }
#ifdef TEST_LOG
    if (log_type_ == LOG_TYPE_CYCLE_MIN)
    {
        snprintf(remv_file, sizeof(remv_file) - 1, "%s.%u", src_file, cur_file_no_ - max_file_no_);
        remove_file(remv_file);

    }
#endif

    close_file(); // 先移动文件，再close，防止MP解锁
    pre_time_ = cur_time;
#ifndef _MP_MODE
    cur_file_no_ = new_file_no;
#else
    new_file_no = new_file_no; // for warring
#endif
    return open_file();
}

int CTLog::get_log_ap_path_prefix(char *path, int buf_len)
{
    int pos = 0;

    if (NULL == getcwd(path, buf_len))
    {
        LOG_P_ALL(LOG_ERROR, "get pwd failed, %m\n");
        return -1;
    }

    pos = strlen(path);
    pos += snprintf(path + pos, buf_len - pos, "/%s/%s", log_path_, name_prefix_);

    if (pos > buf_len)
    {
        LOG_P_ALL(LOG_ERROR, "buff is small\n");
        return -2;
    }

    return 0;
}

int CTLog::switch_net_log(const char *srv_name, const char *usr_name, const uint32_t app_id, const uint64_t uin, const uint32_t route_no)
{
    int   ret = 0;
    int   fd  = 0;

    LOG_P_ALL(LOG_DEBUG, "switch to net log, srv_name:%s, usr_name:%s, appid:%u, uin:%llu, route_no:%u\n", srv_name, usr_name, app_id, uin, route_no);

    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        LOG_P_ALL(LOG_DEBUG, "switch repeated, srv_name:%s\n", srv_name);
        return 0;
    }

    char log_path[MAX_PATH_LEN];
    ret = get_log_ap_path_prefix(log_path, MAX_PATH_LEN);
    if (ret < 0)
    {
        LOG_P_ALL(LOG_DEBUG, "get log ap path failed");
        return -1;
    }

    LsLogFileInfo local_info;
    local_info.szLogFilePath = log_path;
    local_info.iLogSize      = max_file_size_;
    local_info.iLogNum       = max_file_no_;

    // 打开uls日志
    fd = LsLogFsOpenNoConf(srv_name, usr_name, app_id, &local_info);
    if (fd < 0)
    {
        LOG_P_ALL(LOG_ERROR, "switch to net log, fs open failed, ret:%d\n", fd);
        return -2;
    }

    // 设置日志等级
    LsLogLevelConf lvl_conf;
    ret = LsLogGetConf(fd, &lvl_conf);
    if (ret)
    {
        LOG_P_ALL(LOG_ERROR, "get log conf failed, ret:%d\n", ret);
        return -3;
    }

    lvl_conf.cWLogLevel = switch_lvl_to_uls(log_level_);
    ret = LsLogSetConf(fd, &lvl_conf);
    if (ret)
    {
        LOG_P_ALL(LOG_ERROR, "set log conf failed, ret:%d\n", ret);
        return -4;
    }

    // 设置相关参数
    LsPkgParam stLsPkgParam;
    memset(&stLsPkgParam, 0, sizeof(stLsPkgParam));
    stLsPkgParam.ddwUin = uin;
    stLsPkgParam.dwUserDef1 = route_no;
    ret = LsLogFsPreSetPktParam(fd, &stLsPkgParam);
    if (ret)
    {
        LOG_P_ALL(LOG_ERROR, "switch to net log, set pkg param failed, ret:%d\n", ret);
        LsLogFsClose(log_fd_);
        return -5;
    }

    // 关闭spp本地日志文件
    close_file();   // 需要先关闭文件描述符
    log_type_ = LOG_TYPE_NET_LOG_ULS;
    log_fd_ = fd;

    return 0;
}

int CTLog::log_open(int log_level, int log_type, const char* log_path, const char* name_prefix, int max_file_size, int max_file_no)
{
    log_level_ = log_level;
    log_type_ = log_type;
    memset(name_prefix_, 0x0, MAX_PATH_LEN);
    strncpy(name_prefix_, name_prefix, MAX_PATH_LEN - 1);
    memset(log_path_, 0x0, MAX_PATH_LEN);
    strncpy(log_path_, log_path, MAX_PATH_LEN - 1);
    int pathlen = strlen(log_path_);

    if (unlikely(log_path_[pathlen - 1] == '/'))
    {
        log_path_[pathlen - 1] = 0;
    }

    max_file_size_ = max_file_size;

    if (unlikely(max_file_size_ > DEFAULT_MAX_FILE_SIZE))
    {
        max_file_size_ = DEFAULT_MAX_FILE_SIZE;
    }

    max_file_no_ = max_file_no;
    pre_time_ = time(NULL);

    close_file();
    init_cur_file_no();
    return open_file();
}
int CTLog::log_level(int level)
{
    if (level < 0)  		//get
    {
        return log_level_;
    }
    else  				//set&return old
    {
        int tmp = log_level_;
        log_level_ = level;
        return tmp;
    }
}

int CTLog::switch_lvl_to_uls(int level)
{
    int uls_lvl = 0;

    switch(level)
    {
        case LOG_TRACE:
            uls_lvl = _LC_DEBUG_;
            break;

        case LOG_DEBUG:
            uls_lvl = _LC_DEBUG_;
            break;

        case LOG_NORMAL:
            uls_lvl = _LC_NOTICE_;
            break;

        case LOG_ERROR:
            uls_lvl = _LC_ERROR_;
            break;

        case LOG_FATAL:
            uls_lvl = _LC_ERROR_;
            break;

        default:
            uls_lvl = _LC_ERROR_;
            break;
    }

    return uls_lvl;
}

void CTLog::log_i(int flag, int log_level, const char *fmt, ...)
{
    // check uls log level
    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        // if (!LsLogFsCheckLevel(log_fd_, log_level))
        if (likely(log_level < log_level_)) // check spp log level
            return ;
    }
    else
    {
        if (likely(log_level < log_level_)) // check spp log level
            return;
    }

    if (unlikely(shift_file() < 0))
        return;
	ErrnoGuard err_guard;
    va_list ap;
    va_start(ap, fmt);
#if __GLIBC_PREREQ(2, 3)

    if (unlikely(hook_ != NULL))
    {
        va_list ap2;
        va_copy(ap2, ap);

        if (!hook_(fmt, ap2))
        {
            va_end(ap2);
            va_end(ap);
            return;
        }
    }

#endif

#ifdef _THREAD_SAFE
    CLogLock guard;  //buffer_需要互斥
#endif
    int buff_len = 0;

    if (log_type_ != LOG_TYPE_NET_LOG_ULS)
    {
        if (likely(flag & LOG_FLAG_TIME))
            get_time(buff_len);

        if (unlikely(flag & LOG_FLAG_LEVEL))
            get_level(buff_len, log_level);
    }

    if (unlikely(flag & LOG_FLAG_TID))
        get_tid(buff_len);


    buff_len += vsnprintf(buffer_ + buff_len, MAX_LOG_LEN - buff_len - 1, fmt, ap);
    buff_len = (buff_len < MAX_LOG_LEN - 1 ? buff_len : MAX_LOG_LEN - 1);
    *(buffer_ + buff_len - 1) = '\n';
    *(buffer_ + buff_len)     = '\0';

    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        LsLogFsWrite(log_fd_, "", 0, "", switch_lvl_to_uls(log_level), "%s", buffer_);
    }
    else
    {
        write(log_fd_, buffer_, buff_len);
        cur_file_size_ += buff_len;
    }

    va_end(ap);
}


void CTLog::log_i_va(int flag, int log_level, const char *fmt, va_list ap)
{
    // check uls log level
    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        //if (!LsLogFsCheckLevel(log_fd_, log_level))
        if (likely(log_level < log_level_)) // check spp log level
            return ;
    }
    else
    {
        if (likely(log_level < log_level_)) // check spp log level
            return;
    }

    if (unlikely(shift_file() < 0))
        return;
	ErrnoGuard err_guard;
#if __GLIBC_PREREQ(2, 3)

    if (unlikely(hook_ != NULL))
    {
        va_list ap2;
        va_copy(ap2, ap);

        if (!hook_(fmt, ap2))
        {
            va_end(ap2);
            return;
        }
    }

#endif

#ifdef _THREAD_SAFE
    CLogLock guard;  //buffer_需要互斥
#endif
    int buff_len = 0;

    if (log_type_ != LOG_TYPE_NET_LOG_ULS)
    {
        if (likely(flag & LOG_FLAG_TIME))
            get_time(buff_len);

        if (unlikely(flag & LOG_FLAG_LEVEL))
            get_level(buff_len, log_level);
    }

    if (unlikely(flag & LOG_FLAG_TID))
        get_tid(buff_len);

    buff_len += vsnprintf(buffer_ + buff_len, MAX_LOG_LEN - buff_len - 1, fmt, ap);
    buff_len = (buff_len < MAX_LOG_LEN - 1 ? buff_len : MAX_LOG_LEN - 1);

    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        LsLogFsWrite(log_fd_, "", 0, "", switch_lvl_to_uls(log_level), "%s", buffer_);
    }
    else
    {
        write(log_fd_, buffer_, buff_len);
        cur_file_size_ += buff_len;
    }
}


void CTLog::log_i_bin(int log_level, const char* buf, int len)
{
    // check uls log level
    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        // if (!LsLogFsCheckLevel(log_fd_, log_level))
        if (likely(log_level < log_level_)) // check spp log level
            return ;
    }
    else
    {
        if (likely(log_level < log_level_)) // check spp log level
            return;
    }

    if (unlikely(shift_file() < 0))
        return;
	ErrnoGuard err_guard;
#ifdef _THREAD_SAFE
    CLogLock guard;  //buffer_需要互斥
#endif

    if (LOG_TYPE_NET_LOG_ULS == log_type_)
    {
        LsLogFsWrite(log_fd_, "", 0, "", switch_lvl_to_uls(log_level), "%s", buf);
    }
    else
    {
        write(log_fd_, buf, len);
        cur_file_size_ += len;
    }
}
void CTLog::log_set_hook(log_hook hook)
{
    hook_ = hook;
}

