#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdlib.h>
#include <alloca.h>
#include <pwd.h>

#define MAX_PATH_LEN 256
#define MAX_IP_LEN   32
#define MAX_PORT_LEN 16
#define MAX_TYPE_LEN 16
#define DEFAULT_PROXY_PORT "19872"

struct CONF
{
    char bin[MAX_PATH_LEN];
    char etc[MAX_PATH_LEN];
    int  type;
    int  stop_flag;
    char proxy_ip[MAX_IP_LEN];
    char proxy_port[MAX_PORT_LEN];
};

CONF g_conf;

bool check_file_exist(const char* path);

// 获取业务包名:上一级目录的目录名为业务包名
int get_pkg_name(char *name, int len)
{
    char  path[MAX_PATH_LEN];
    char* pos = NULL;
    if (NULL == getcwd(path, MAX_PATH_LEN))
    {
        perror("getcwd failed!\n");
        return -1;
    }

    pos = rindex(path, '/');
    if (NULL == pos)
    {
        fprintf(stderr, "get path error! %s\n", path);
        return -2;
    }

    *pos = '\0';
    pos = rindex(path, '/');
    if (NULL == pos)
    {
        fprintf(stderr, "get path error! %s\n", path);
        return -3;
    }

    pos++;
    strncpy(name, pos, len);

    return 0;
}

// 获取业务名:bin参数去掉spp_ _proxy _worker部分便是业务名
int get_service_name(char *name, int len)
{
    char  bin[MAX_PATH_LEN];
    char* head_pos;
    char* end_pos;

    strncpy(bin, g_conf.bin, MAX_PATH_LEN);

    head_pos = index(bin, '_');
    if (NULL == head_pos)
    {
        fprintf(stderr, "service name is wrong! %s\n", g_conf.bin);
        return -1;
    }

    *head_pos++ = '\0';
    if (strcmp(bin, "spp") && strcmp(bin, "./spp"))
    {
        fprintf(stderr, "service name is wrong! %s\n", g_conf.bin);
        return -2;
    }

    end_pos = rindex(head_pos, '_');
    if (NULL == end_pos)
    {
        fprintf(stderr, "service name is wrong! %s\n", g_conf.bin);
        return -3;
    }
    
    *end_pos++ = '\0';
    if (strcmp(end_pos, "proxy") && strcmp(end_pos, "worker"))
    {
        fprintf(stderr, "service name is wrong! %s\n", g_conf.bin);
        return -4;
    }

    if (end_pos - head_pos < 2)
    {
        fprintf(stderr, "service name is wrong! %s\n", g_conf.bin);
        return -5;
    }

    strncpy(name, head_pos, len);

    return 0;
}

// 获取worker组号
int get_group_id(void)
{
    char  wk_magic[] = "spp_worker";
    char  conf[128];
    char* pos;
    int   id;
    

    strncpy(conf, g_conf.etc, sizeof(conf));

    if (strstr(conf, "spp_proxy.xml"))
    {
        return 0;
    }

    if (NULL == (pos = strstr(conf, wk_magic)))
    {
        fprintf(stderr, "config file is error, %s\n", conf);
        return -1;
    }

    pos += (sizeof(wk_magic) - 1);

    id = (int)strtol(pos, NULL, 10);

    if (id <= 0)
    {
        return -1;
    }

    return id;
}

// 获取set id
int get_set_id(void)
{
    char file[] = "/tmp/.nest_set_file";
    char magic[] = "[SET_ID]";
    char buff[1024];
    int  fd;
    int  len;
    char* pos;

    if (!check_file_exist(file))
    {
        fprintf(stderr, "%s is not exist!\n", file);
        return -1;
    }

    fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "open file failed, [%s] [%m]\n", file);
        return -2;
    }

    if ((len = read(fd, buff, sizeof(buff)) < 0))
    {
        fprintf(stderr, "read file failed, [%s] [%m]\n", file);
        return -3;
    }

    if (NULL == (pos = strstr(buff, magic)))
    {
        fprintf(stderr, "maybe file read is not complete, [%s] [%m]\n", file);
        return -4;
    }

    return (int)strtol(pos + sizeof(magic), NULL, 10);
}

// 检查指定目录是否存在
bool check_dir_exist(const char* path)
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

// 检查文件是否存在
bool check_file_exist(const char* path)
{
    if (!path) {
        return false;
    }
    
    int result = access (path, F_OK);
    if (result < 0) {
        return false;
    } else {
        return true;
    } 
}

// 检查并创建目录
bool check_and_mkdir(char *path)
{
    if (!path)
    {
        fprintf(stderr, "make path failed, path is null");
        return false;
    }

    if (!check_dir_exist(path))
    {
        if (::mkdir(path, 0666) < 0) 
        {
            fprintf(stderr, "make path [%s] failed(%m)", path);
            return false;
        }
    }
    
    if (::chmod(path, 0777) < 0)
    {
        fprintf(stderr, "change mod path [%s] failed(%m)", path);
        return false;
    }

    return true;
}

// 递归创建目录
int32_t mkdir_recursive(char *path, size_t len)
{
    char    cpath[256];
    char*   pos = path + 1;

    if (len > sizeof(cpath))
    {
        fprintf(stderr, "make path [%s] failed, path too long.", path);
        return -1;
    }

    // 循环创建目录
    while ((pos = index(pos, '/')))
    {
        int32_t len = pos - path;

        strncpy(cpath, path, len);
        *(cpath + len) = '\0';
        
        if (!check_and_mkdir(cpath))
        {
            fprintf(stderr, "check and mkdir [%s] failed", path);
            return -2;
        }
        
        pos++;
    }

    // 检查并创建目录
    if (!check_and_mkdir(path))
    {
        fprintf(stderr, "check and mkdir [%s] failed", path);
        return -3;
    }

    return 0;
}


// mount进程私有目录
bool mount_priv(char *pkg_name)
{
    // 1. 确认PATH路径OK
    char src_path[256];
    char dst_path[256] = "/data/release";
    snprintf(src_path, sizeof(src_path), "/data/spp_mount_dir/%s/release", pkg_name);

    if (mkdir_recursive(src_path, sizeof(src_path)) < 0)
    {
        fprintf(stderr, "mkdir recursive failed, [%s]", src_path);
        return false;
    }

    if (mkdir_recursive(dst_path, sizeof(dst_path)) < 0)
    {
        fprintf(stderr, "mkdir recursive failed, [%s]", dst_path);
        return false;
    }

#ifndef MS_PRIVATE
#define MS_PRIVATE (1<<18)
#endif

    // 2. 执行私有目录bind
    if (mount(src_path, dst_path, NULL, MS_BIND | MS_PRIVATE, NULL) != 0)
    {
        fprintf(stderr, "mount path [%s] to /data/release failed(%m)\n", src_path);
        return false;
    }

    return true;    
}

// 切换进程用户名
int chg_user(const char *user)
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

// 子进程执行函数
int clone_handler(void *args)
{
    int  ret = -1;
    char package[MAX_PATH_LEN];
    char service[MAX_PATH_LEN];

    ret = get_pkg_name(package, MAX_PATH_LEN);
    if (ret < 0)
    {
        fprintf(stderr, "get package name failed.ret:%d\n", ret);
        return -1;
    }

    ret = get_service_name(service, MAX_PATH_LEN);
    if (ret < 0)
    {
        fprintf(stderr, "get service name failed.ret:%d\n", ret);
        return -5;
    }

    ret = mount_priv(package);
    if (ret < 0)
    {
        fprintf(stderr, "mount private directory failed.ret:%d\n", ret);
        return -2;
    }

    ret = chg_user("user_00");
    if (ret < 0)
    {
        fprintf(stderr, "change user to user_00 failed.ret:%d\n", ret);
        return -3;
    }
            
    if (!check_file_exist(g_conf.etc))
    {
        system("./yaml_tool x");
    }

    char *envname = "LD_LIBRARY_PATH";
    char  env[2048] = {0};
    char *env_tmp = getenv(envname);
    if (env_tmp)
    {
        snprintf(env, sizeof(env), "%s=/usr/local/services/%s/client/%s/lib:%s",
                 envname, package, service, env_tmp);
    }
    else
    {
        snprintf(env, sizeof(env), "%s=/usr/local/services/%s/client/%s/lib",
                 envname, package, service);
    }

    char *const envp[] = {env, (char *)0};
    char *const cmd[] = {g_conf.bin, g_conf.etc, (char*)0};
    ret = execve(g_conf.bin, cmd, envp);
    if (ret != 0)
    {
        fprintf(stderr, "start proc failed, (%m)\n");
        return -4;
    }

    return 0;    
}

// 调用clone生成子进程
pid_t do_fork()
{
    size_t stack_size = 4096;
    void *stack = alloca(stack_size);
    pid_t ret = clone(clone_handler, (char*)stack + stack_size, CLONE_NEWNS | CLONE_PTRACE | SIGCHLD, NULL);
    if (ret < 0)    
    {        
        printf("clone failed, ret %d(%m)", ret);
        return -1;
    }

    usleep(200*1000);
    return ret;
}

// 获取本地IP
unsigned int get_ip(void)
{
    char ifname[16] = "eth1";
    int  fd, intrface;
    struct ifreq buf[10];
    struct ifconf ifc;
    unsigned ip = 0;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;

        if (ioctl(fd, SIOCGIFCONF, (char*)&ifc))
        {
            close(fd);
            return 0;
        }

        intrface = ifc.ifc_len / sizeof(struct ifreq);

        while (intrface-- > 0)
        {
            if (strcmp(buf[intrface].ifr_name, ifname) == 0)
            {
                if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface])))
                    ip = (unsigned int)((struct sockaddr_in *)(&buf[intrface].ifr_addr))->sin_addr.s_addr;

                break;
            }
        }

        close(fd);
    }

    return ip;
}

// 初始化参数
int init_args(int argc, char **argv)
{
    unsigned int ip;

    strcpy(g_conf.bin, argv[1]);
    strcpy(g_conf.etc, argv[2]);
    g_conf.type = get_group_id();

    if (g_conf.type < 0)
    {
        fprintf(stderr, "conf error!\n");
        return -1;
    }

    switch (argc)
    {
        case 3:
        case 5:
        {
            g_conf.stop_flag = 0;

            if (!g_conf.type || (argc == 3))
            {
                ip = get_ip();
                strcpy(g_conf.proxy_ip, inet_ntoa(*(struct in_addr *)&ip));
                strcpy(g_conf.proxy_port, DEFAULT_PROXY_PORT);
            }
            else
            {
                strcpy(g_conf.proxy_ip, argv[3]);
                strcpy(g_conf.proxy_port, argv[4]);
            }

            break;
        }

        case 4:
        {
            if (!strcmp(argv[3], "stop"))
            {
                g_conf.stop_flag = 1;
            }
            else
            {
                fprintf(stderr, "init args failed, argc is 4, but argv[3] is not stop.\n");
                return -2;
            }

            break;
        }

        default:
        {
            fprintf(stderr, "argc err.\n");
            return -3;
        }
    }

    return 0;
}

// 发送初始化命令
void send_init_msg(pid_t pid)
{
    #define MAX_CMD_LEN 128

    int ret = 0;
    char cmd[MAX_CMD_LEN];
    char service[MAX_PATH_LEN];
    char package[MAX_PATH_LEN];

    ret =  get_pkg_name(package, MAX_PATH_LEN);
    ret += get_service_name(service, MAX_PATH_LEN);

    if (ret)
    {
        fprintf(stderr, "send_init_msg get pkg or service name failed.\n");
        return;
    }

    snprintf(cmd, sizeof(cmd), "./nest_tool 6 2244668800 %d 1 %d %s %s %s %s", g_conf.type, pid, g_conf.proxy_ip, g_conf.proxy_port, service, package);

    system(cmd);

    return ;
}

// 发送初始化命令
void send_kill_msg(void)
{
    #define MAX_CMD_LEN 128

    int ip;
    char cmd[MAX_CMD_LEN];

    ip = get_ip();

    snprintf(cmd, sizeof(cmd), "./nest_tool 4 %s %d 2244668800 %d 1 0", inet_ntoa(*(struct in_addr *)&ip), get_set_id(), g_conf.type);

    system(cmd);

    return ;
}

void show_usage(char **argv)
{
    printf("Usage : %s [bin] [etc]                                  : start proxy\n", argv[0]);
    printf("        %s [bin] [etc] [stop]                           : kill process\n", argv[0]);
    printf("        %s [bin] [etc] [proxy_ip] [proxy_port]          : start worker\n", argv[0]);
    return;
}

int main(int argc, char **argv)
{
    int ret;
    pid_t pid;

    if ((argc != 3) && (argc != 5) && (argc != 4))
    {
        show_usage(argv);
        return -1;
    }

    ret = init_args(argc, argv);
    if (ret < 0)
    {
        fprintf(stderr, "init args failed.\n");
        return -2;
    }

    if (g_conf.stop_flag) // 杀死进程
    {
        send_kill_msg();
        return 0;
    }

    // 启动进程
    pid = do_fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork service process failed.\n");
        return -2;
    }

    send_init_msg(pid);

    return 0;
}
