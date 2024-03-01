/**
 * @brief Nest agent cgroups
 * @info  one hierarchy, cpu\memory subsystem
 */
 
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include "loadconfbase.h"

#ifndef __NEST_AGENT_CGROUPS_H_
#define __NEST_AGENT_CGROUPS_H_

using std::string;
using namespace spp::tconfbase;

namespace nest
{

    #define SERVICE_CPU_QUOTA   40  // 
    #define TOOLS_CPU_QUOTA     20  // 


    /**
     * @brief Cgroup类, 默认是在一个 Hierarchy 之下
     */
    class CGroup
    {
    public:

        // 构造与析构函数
        CGroup(const string& root, const string& group_name);

		// 根cgroup
		CGroup(const string& root);
		
        ~CGroup(){};

        // 是否已执行初始化
        bool enabled();

        // 新增该group信息
        bool init();

        // 清除该group信息
        bool term();

        // 新增task信息
        bool add_task(uint32_t pid);

        // 设置cpu share共享
        bool set_cpu_shares(uint32_t share);

        // 设置memory limit
        bool set_memory_limit(uint64_t limit);

        // 设置memory limit
        bool set_memory_swap_limit(uint64_t limit);

        // 设置cpu quota
        bool set_cpu_quota(uint32_t percent);

		// 设置memory limit，同时设置memory.limit_in_bytes 和 memory.memsw.limit_in_bytes
		bool set_memory_quota(uint64_t limit);

		// 设置memory limit，使用百分比
		bool set_memory_percent(uint32_t percent);

		bool set_memory_reserved(uint64_t reserved);

		// 设置memory.use_hierarchy 为 1
		bool enable_memory_hierarchy();

		// 设置memory.use_hierarchy 为 0
		bool disable_memory_hierarchy();
    private:
		bool init_inner(const string& parent, const string& group_name);
		// 设置memory.use_hierarchy 
		bool set_memory_hierarchy(bool flag);

        string      _parent;            // 父层或group的路径,完整路径
        string      _name;              // 本group的名称

		string      _cpu_path;          // cpu cgroup完整路径
		string 		_mem_path;          // mem cgroup 完整路径
    };

    /**
     * @brief hierarchy管理类
     */
    class Hierarchy
    {
    public:
    
        // 构造与析构函数
        Hierarchy(const string& name);
        ~Hierarchy(){};
    
        // 是否已执行初始化
        bool enabled();
    
        // 新增该Hierarchy信息
        bool init(const string& sub_systems);
	private:
		bool reset();
		bool resetRoot();
		bool resetTools();
		bool resetService(const string& name);
    private:

        string          _name;      // 名字
        string          _sub_sys;   // 已经bind的子系统
    
    };


    /**
     * @brief Cgroups管理类
     */
    class CGroupMng
    {
    public:

        // 默认的全局定义部分
        static string ctrl_file;          // 控制文件 /proc/cgroups
        static string cgroup_root;        // 根层 /cgroups
        static string hierarchy_root;     // 根层的设备名 nest_cg
        static string subsystem;          // 目前只支持cpu,memory
        static int    cgroup_version;     // 未初始化时，为-1；系统未mount /sys/fs/cgroup，则为1；否则为0，即表示旧版本tlinux，按照旧规则，mount在/cgroups
		
		static CGroupConf cgroup_conf;

        // 系统是否支持cgoups
        static bool enabled();
        
        // 加载子系统
        static bool mount(const string& hierarchy, const string& subsystems);
        
        // 取消加载
        static void unmount(const string& hierarchy);

        // 是否已经mount
        static bool mounted(const string& hierarchy, string& opts, const string& req_mnt_type);
        
        // 读取配置信息
        static bool read(const string& ctrl_file, string& value);
        
        // 写入配置信息
        static bool write(const string& ctrl_file, const string& value);
        
        // 文件或目录是否存在
        static bool exists(const string& path);

		// 检查cgroup version
		static int check_version();
    };



    /**
     * @brief CGroup全局管理信息
     */
    class CAgentCGroups
    {
    public:

        // 构造与析构函数
        CAgentCGroups();
        virtual ~CAgentCGroups(){};

        // 根层是否启动OK
        bool cgroup_inited() {
            return _cgroup_inited;
        };

        // 业务group创建
        bool service_group_create(const string& service);

        // 业务group加入进程信息
        bool service_group_add_task(const string& service, uint32_t pid);

    private:

        bool    _cgroup_inited;         // 根层是否已经启用
    };

}


#endif



