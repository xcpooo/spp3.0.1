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
     * @brief Cgroup��, Ĭ������һ�� Hierarchy ֮��
     */
    class CGroup
    {
    public:

        // ��������������
        CGroup(const string& root, const string& group_name);

		// ��cgroup
		CGroup(const string& root);
		
        ~CGroup(){};

        // �Ƿ���ִ�г�ʼ��
        bool enabled();

        // ������group��Ϣ
        bool init();

        // �����group��Ϣ
        bool term();

        // ����task��Ϣ
        bool add_task(uint32_t pid);

        // ����cpu share����
        bool set_cpu_shares(uint32_t share);

        // ����memory limit
        bool set_memory_limit(uint64_t limit);

        // ����memory limit
        bool set_memory_swap_limit(uint64_t limit);

        // ����cpu quota
        bool set_cpu_quota(uint32_t percent);

		// ����memory limit��ͬʱ����memory.limit_in_bytes �� memory.memsw.limit_in_bytes
		bool set_memory_quota(uint64_t limit);

		// ����memory limit��ʹ�ðٷֱ�
		bool set_memory_percent(uint32_t percent);

		bool set_memory_reserved(uint64_t reserved);

		// ����memory.use_hierarchy Ϊ 1
		bool enable_memory_hierarchy();

		// ����memory.use_hierarchy Ϊ 0
		bool disable_memory_hierarchy();
    private:
		bool init_inner(const string& parent, const string& group_name);
		// ����memory.use_hierarchy 
		bool set_memory_hierarchy(bool flag);

        string      _parent;            // �����group��·��,����·��
        string      _name;              // ��group������

		string      _cpu_path;          // cpu cgroup����·��
		string 		_mem_path;          // mem cgroup ����·��
    };

    /**
     * @brief hierarchy������
     */
    class Hierarchy
    {
    public:
    
        // ��������������
        Hierarchy(const string& name);
        ~Hierarchy(){};
    
        // �Ƿ���ִ�г�ʼ��
        bool enabled();
    
        // ������Hierarchy��Ϣ
        bool init(const string& sub_systems);
	private:
		bool reset();
		bool resetRoot();
		bool resetTools();
		bool resetService(const string& name);
    private:

        string          _name;      // ����
        string          _sub_sys;   // �Ѿ�bind����ϵͳ
    
    };


    /**
     * @brief Cgroups������
     */
    class CGroupMng
    {
    public:

        // Ĭ�ϵ�ȫ�ֶ��岿��
        static string ctrl_file;          // �����ļ� /proc/cgroups
        static string cgroup_root;        // ���� /cgroups
        static string hierarchy_root;     // ������豸�� nest_cg
        static string subsystem;          // Ŀǰֻ֧��cpu,memory
        static int    cgroup_version;     // δ��ʼ��ʱ��Ϊ-1��ϵͳδmount /sys/fs/cgroup����Ϊ1������Ϊ0������ʾ�ɰ汾tlinux�����վɹ���mount��/cgroups
		
		static CGroupConf cgroup_conf;

        // ϵͳ�Ƿ�֧��cgoups
        static bool enabled();
        
        // ������ϵͳ
        static bool mount(const string& hierarchy, const string& subsystems);
        
        // ȡ������
        static void unmount(const string& hierarchy);

        // �Ƿ��Ѿ�mount
        static bool mounted(const string& hierarchy, string& opts, const string& req_mnt_type);
        
        // ��ȡ������Ϣ
        static bool read(const string& ctrl_file, string& value);
        
        // д��������Ϣ
        static bool write(const string& ctrl_file, const string& value);
        
        // �ļ���Ŀ¼�Ƿ����
        static bool exists(const string& path);

		// ���cgroup version
		static int check_version();
    };



    /**
     * @brief CGroupȫ�ֹ�����Ϣ
     */
    class CAgentCGroups
    {
    public:

        // ��������������
        CAgentCGroups();
        virtual ~CAgentCGroups(){};

        // �����Ƿ�����OK
        bool cgroup_inited() {
            return _cgroup_inited;
        };

        // ҵ��group����
        bool service_group_create(const string& service);

        // ҵ��group���������Ϣ
        bool service_group_add_task(const string& service, uint32_t pid);

    private:

        bool    _cgroup_inited;         // �����Ƿ��Ѿ�����
    };

}


#endif



