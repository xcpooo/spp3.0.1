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
     * @brief �޸��ӽ��̵��û���
     * @return =0 �ɹ� <0 ʧ��
     */
    int32_t ChgUser(const char *user);

    /**
     * @brief cloneִ�н��̳�ʼ������ں���
     */
    int32_t NestExecv();

    /**
     * @brief �ж��ļ��Ƿ����
     */
    bool IsFileExist(const char* path);

    /**
     * @brief �ж�Ŀ¼�Ƿ����
     */
    bool IsDirExist(const char* path);

    /**
     * @brief ���path�Ƿ���ڣ�����������򴴽�
     */
    bool CheckAndMkdir(char *path);

    /**
     * @brief  �ݹ鴴��Ŀ¼
     * @return =0 �ɹ� <0 ʧ��
     */
    int32_t MkdirRecursive(char *path, size_t len);

    /**
     * @brief ��鲢��ӳ��/dataĿ¼��˽�е�path
     */
    bool MountPriPath();

    /**
     * @brief ���ݳ�ʼ��
     */
    int Init(int argc, char **argv);

    /**
     * @brief ���к���
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
 * @brief �޸��ӽ��̵��û���
 * @return =0 �ɹ� <0 ʧ��
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
 * @brief cloneִ�н��̳�ʼ������ں���
 */
int32_t CAgentExecTool::NestExecv()
{
    // 1. ·�����, ˽��Ŀ¼����
    if (!CAgentExecTool::MountPriPath())
    {
        return -1;
    }
    
    // 2. �л��û���
    if (_type == "spp" && CAgentExecTool::ChgUser("user_00") < 0)
    {
        return -2;
    }

    setsid(); // �����»Ự

    // 3. �л�����Ŀ¼
    char path[256] = {0};
    if (_type == "sf2")
        snprintf(path, sizeof(path), "/home/oicq/%s/bin", _package.c_str());
    else
        snprintf(path, sizeof(path), "/usr/local/services/%s/bin", _package.c_str());

    if (::chdir(path) < 0)
    {
        return -3;
    }
            
    // 4. ˽�нű�ִ��
    if (!IsFileExist(_etc.c_str()))
    {
        system("./yaml_tool x");
    }
    
    // 5. LD_LIBRARY_PATH������������ֵ
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

    // 6. ִ��ҵ���������
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
 * @brief �ж��ļ��Ƿ����
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
 * @brief �ж�Ŀ¼�Ƿ����
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
 * @brief ���path�Ƿ���ڣ�����������򴴽�
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
 * @brief  �ݹ鴴��Ŀ¼
 * @return =0 �ɹ� <0 ʧ��
 */
int32_t CAgentExecTool::MkdirRecursive(char *path, size_t len)
{
    char    cpath[256];
    char*   pos = path + 1;

    if (len > sizeof(cpath))
    {
        return -1;
    }

    // ѭ������Ŀ¼
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

    // ��鲢����Ŀ¼
    if (!CheckAndMkdir(path))
    {
        return -3;
    }

    return 0;
}

/**
 * @brief ��鲢��ӳ��/dataĿ¼��˽�е�path
 */
bool CAgentExecTool::MountPriPath()
{
    // 1. ȷ��PATH·��OK
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

    // 2. ִ��˽��Ŀ¼bind
    if (::mount(src_path, dst_path, NULL, MS_BIND | MS_PRIVATE, NULL) != 0)
    {
        return false;
    }

 
    return true;    
}

/**
 * @brief ��ʼ������
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
 * @brief ���к���
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
