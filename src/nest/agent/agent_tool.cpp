/**
 * @brief Nest agent tool
 */
#include <stdio.h>
#include <unistd.h> 
#include <stdint.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sched.h>
#include <alloca.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include <string>

using namespace std;

class CAgentExecTool
{
public:

    /**
     * @brief 修改子进程的用户名
     * @return =0 成功 <0 失败
     */
    int32_t ChgUser(const char *user);

    /**
     * @brief clone执行进程初始化的入口函数
     */
    int32_t NestExecv();

    /**
     * @brief 判定文件是否存在
     */
    bool IsFileExist(const char* path);

    /**
     * @brief 判定目录是否存在
     */
    bool IsDirExist(const char* path);

    /**
     * @brief 检查path是否存在，如果不存在则创建
     */
    bool CheckAndMkdir(char *path);

    /**
     * @brief  递归创建目录
     * @return =0 成功 <0 失败
     */
    int32_t MkdirRecursive(char *path, size_t len);

    /**
     * @brief 检查并重映射/data目录到私有的path
     */
    bool MountPriPath();

    /**
     * @brief 数据初始化
     */
    int Init(int argc, char **argv);

    /**
     * @brief 运行函数
     */
    int Run(int argc, char **argv);

public:

    string  _bin;
    string  _etc;
    string  _package;
    string  _service;
    string  _type;
};


/**
 * @brief 修改子进程的用户名
 * @return =0 成功 <0 失败
 */
int32_t CAgentExecTool::ChgUser(const char *user)
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



/**
 * @brief clone执行进程初始化的入口函数
 */
int32_t CAgentExecTool::NestExecv()
{
    // 1. 路径检查, 私有目录创建
    if (!CAgentExecTool::MountPriPath())
    {
        return -1;
    }
    
    // 2. 切换用户名
    if (_type == "spp" && CAgentExecTool::ChgUser("user_00") < 0)
    {
        return -2;
    }

    setsid(); // 创建新会话

    // 3. 切换工作目录
    char path[256] = {0};
    if (_type == "sf2")
        snprintf(path, sizeof(path), "/home/oicq/%s/bin", _package.c_str());
    else
        snprintf(path, sizeof(path), "/usr/local/services/%s/bin", _package.c_str());

    if (::chdir(path) < 0)
    {
        return -3;
    }
            
    // 4. 私有脚本执行
    if (!IsFileExist(_etc.c_str()))
    {
        system("./yaml_tool x");
    }
    
    // 5. LD_LIBRARY_PATH环境变量设置值
    char *envname = "LD_LIBRARY_PATH";
    char  env[2048] = {0};
    char *env_tmp = getenv(envname);
    if (env_tmp)
    {
        snprintf(env, sizeof(env), "%s=/usr/local/services/%s/client/%s/lib:%s",
                 envname, _package.c_str(), _service.c_str(), env_tmp);
    }
    else
    {
        snprintf(env, sizeof(env), "%s=/usr/local/services/%s/client/%s/lib",
                 envname, _package.c_str(), _service.c_str());
    }

    // 6. 执行业务进程启动
    char *const envp[] = {env, (char *)0};
    char *const cmd[] = {(char*)_bin.c_str(), (char*)_etc.c_str(), (char*)0};
    int32_t ret = execve(_bin.c_str(), cmd, envp);
    if (ret != 0)
    {
        return -4;
    }

    return 0;    
}

/**
 * @brief 判定文件是否存在
 */
bool CAgentExecTool::IsFileExist(const char* path)
{
    if (!path) {
        return false;
    }
    
    int32_t result = access (path, F_OK);
    if (result < 0) {
        return false;
    } else {
        return true;
    } 
}

/**
 * @brief 判定目录是否存在
 */
bool CAgentExecTool::IsDirExist(const char* path)
{
    if (!path) {
        return false;
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return false;
    } else {
        closedir(dir);
        return true;
    } 
}


/**
 * @brief 检查path是否存在，如果不存在则创建
 */
bool CAgentExecTool::CheckAndMkdir(char *path)
{
    if (!path)
    {
        return false;
    }

    if (!IsDirExist(path))
    {
        if (::mkdir(path, 0666) < 0) 
        {
            return false;
        }
    }
    
    if (::chmod(path, 0777) < 0)
    {
        return false;
    }

    return true;
}

/**
 * @brief  递归创建目录
 * @return =0 成功 <0 失败
 */
int32_t CAgentExecTool::MkdirRecursive(char *path, size_t len)
{
    char    cpath[256];
    char*   pos = path + 1;

    if (len > sizeof(cpath))
    {
        return -1;
    }

    // 循环创建目录
    while ((pos = index(pos, '/')))
    {
        int32_t len = pos - path;

        strncpy(cpath, path, len);
        *(cpath + len) = '\0';
        
        if (!CheckAndMkdir(cpath))
        {
            return -2;
        }
        
        pos++;
    }

    // 检查并创建目录
    if (!CheckAndMkdir(path))
    {
        return -3;
    }

    return 0;
}

/**
 * @brief 检查并重映射/data目录到私有的path
 */
bool CAgentExecTool::MountPriPath()
{
    // 1. 确认PATH路径OK
    char src_path[256];
    char dst_path[256] = "/data/release";
    snprintf(src_path, sizeof(src_path), "/data/spp_mount_dir/%s/release", _package.c_str());

    if (CAgentExecTool::MkdirRecursive(src_path, sizeof(src_path)) < 0)
    {
        return false;
    }

    if (CAgentExecTool::MkdirRecursive(dst_path, sizeof(dst_path)) < 0)
    {
        return false;
    }

#ifndef MS_PRIVATE
#define MS_PRIVATE (1<<18)
#endif

    // 2. 执行私有目录bind
    if (::mount(src_path, dst_path, NULL, MS_BIND | MS_PRIVATE, NULL) != 0)
    {
        return false;
    }

 
    return true;    
}

/**
 * @brief 初始化参数
 */
int CAgentExecTool::Init(int argc, char **argv)
{
    _bin = argv[1];
    _etc = argv[2];
    _package = argv[3];
    _service = argv[4];
    _type = argv[5];

    return 0;
}

/**
 * @brief 运行函数
 */
int CAgentExecTool::Run(int argc, char **argv)
{
    int32_t ret = Init(argc, argv);
    if (ret < 0)
    {
        return -1;
    }

    ret = NestExecv();
    if (ret < 0)
    {
        return -2;
    }

    return 0;
}

int main(int argc, char **argv)
{
    CAgentExecTool *tool = new CAgentExecTool;

    tool->Run(argc, argv);
}
