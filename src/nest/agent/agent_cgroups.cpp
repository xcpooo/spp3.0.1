/**
 * @brief Nest agent cgroups
 * @info  one hierarchy, cpu\memory subsystem
 */

#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mount.h>
#include <mntent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sstream> 
#include <fstream> 
#include <assert.h>
#include "agent_cgroups.h"
#include "nest_log.h"
#include "sys_info.h"

using namespace nest;
using namespace std;

#define CGROUP_ROOT_V0  "/cgroups"
#define HIERARCHY_ROOT_V0 "nest_cgroup"
#define SUBSYSTEM_V0 "cpu,memory"

#define CGROUP_ROOT_V1  "/sys/fs/cgroup"
#define HIERARCHY_ROOT_V1 "tmpfs"
#define SUBSYSTEM_V1 "cpu,memory"   // �ɺ���

string CGroupMng::ctrl_file        = "/proc/cgroups"; // �����ļ� /proc/cgroups
string CGroupMng::cgroup_root      = CGROUP_ROOT_V0;      // ���� /cgroups
string CGroupMng::hierarchy_root   = HIERARCHY_ROOT_V0;   // ������豸�� nest_cg
string CGroupMng::subsystem        = SUBSYSTEM_V0;    // Ŀǰֻ֧��cpu,memory


static const string CPU_PATH_MIDDLE[2] = {string(""),string("/cpu")};
static const string MEM_PATH_MIDDLE[2] = {string(""),string("/memory")};

CGroupConf CGroupMng::cgroup_conf;


int CGroupMng::cgroup_version = -1; // δ��ʼ��ʱ��Ϊ-1��ϵͳδmount /sys/fs/cgroup����Ϊ1������Ϊ0������ʾ�ɰ汾tlinux�����վɹ���mount��/cgroups


static string get_cgroup_cpu_root(bool bMultiLevel=true)// cpu path
{
	std::ostringstream out;
    out << CGroupMng::cgroup_root << CPU_PATH_MIDDLE[CGroupMng::cgroup_version];
    if(bMultiLevel)
    {
    	out <<"/nest";
    }

    return out.str(); 
}

static string get_cgroup_mem_root(bool bMultiLevel=true) // mem path
{
	std::ostringstream out;
    out << CGroupMng::cgroup_root << MEM_PATH_MIDDLE[CGroupMng::cgroup_version];
    if(bMultiLevel)
    {
    	out <<"/nest";
    }

    return out.str();        
}

static string get_cgroup_cpu_path(const string& name, bool bMultiLevel=true)
{
	std::ostringstream out;

	out<<get_cgroup_cpu_root(bMultiLevel);

	if(!name.empty())
    {
        out<<"/"<< name;
    }
    return out.str();
}

static string get_cgroup_mem_path(const string& name, bool bMultiLevel=true)
{
	std::ostringstream out;

	out<<get_cgroup_mem_root(bMultiLevel);

	if(!name.empty())
    {
        out<<"/"<< name;
    }
    return out.str();
}

// ϵͳ�Ƿ�֧��cgoups
bool CGroupMng::enabled()
{
    return exists(ctrl_file);
}


// ������ϵͳ
bool CGroupMng::mount(const string& hierarchy, const string& subsystems)
{
    int32_t ret = ::mount(CGroupMng::hierarchy_root.c_str(), hierarchy.c_str(),
                    "cgroup", 0, subsystems.c_str());
    if (ret < 0 && errno!=EBUSY)
    {
        NEST_LOG(LOG_ERROR, "mount %s failed(%m)", hierarchy.c_str());
        return false;
    }

    return true;
}


// ȡ������
void CGroupMng::unmount(const string& hierarchy)
{
    if (::umount(hierarchy.c_str()) < 0)
    {
        NEST_LOG(LOG_ERROR, "umount %s failed(%m)", hierarchy.c_str());
    }
}


// �Ƿ��Ѿ�mount
bool CGroupMng::mounted(const string& hierarchy, string& opts, const string& req_mnt_type)
{
    // 1. Ŀ¼�Ƿ����
    if (!exists(hierarchy))
    {
        return false;
    }

    // 2. ��mount���ļ�
	FILE* proc_mount = fopen("/proc/mounts", "re");
	if (NULL == proc_mount)
	{
        NEST_LOG(LOG_ERROR, "file /proc/mounts open failed(%m)");
        return false;
	}

    // 3. ��ȡĿ¼��mount��Ϣ
    bool found = false;
    struct mntent mnt_buff;
    char str_buff[512];
    struct mntent* mnt_entry = NULL;
    while (true) 
    {
        mnt_entry = ::getmntent_r(proc_mount, &mnt_buff, str_buff, sizeof(str_buff));
        if (mnt_entry == NULL) 
        {
            break;
        }

        string mnt_dir = mnt_entry->mnt_dir;
        string mnt_type = mnt_entry->mnt_type;
        if ((mnt_type == req_mnt_type) && (mnt_dir == hierarchy))
        {
            found = true;
            opts = mnt_entry->mnt_opts;
            break;
        }
    }

    // 4. �����˳�
    fclose(proc_mount);
    return found;
}


// ���cgroup version, ���ϵͳԤmount��/sys/fs/cgroup��version����2�����򷵻�1��
int CGroupMng::check_version()
{
     string opt;
     string mnt_type = HIERARCHY_ROOT_V1;
     if(CGroupMng::mounted(CGROUP_ROOT_V1, opt, mnt_type))
     {
        CGroupMng::cgroup_root = CGROUP_ROOT_V1;
        CGroupMng::hierarchy_root = HIERARCHY_ROOT_V1;
        CGroupMng::cgroup_version = 1;
     }
     else
     {
        CGroupMng::cgroup_root = CGROUP_ROOT_V0;
        CGroupMng::hierarchy_root = HIERARCHY_ROOT_V0;        
        CGroupMng::cgroup_version = 0;
     }

     NEST_LOG(LOG_DEBUG, "cgroup version is %d, cgroup_root is %s", CGroupMng::cgroup_version, CGroupMng::cgroup_root.c_str());

     return 0;
}

// ��ȡ������Ϣ
bool CGroupMng::read(const string& ctrl_file, string& value)
{
    std::ifstream file(ctrl_file.c_str());
    if (!file.is_open()) 
    {
        NEST_LOG(LOG_ERROR, "file %s open failed(%m)", ctrl_file.c_str());
        return false;
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();

    bool ret = false;
    if (file.fail()) 
    {
        NEST_LOG(LOG_ERROR, "file [%s] write [%s] failed(%m)", ctrl_file.c_str(),
            value.c_str());
        ret = false;
    }
    else
    {
        ret = true;
        value = ss.str();
    }
    
    file.close();
    return ret;
}


// д��������Ϣ
bool CGroupMng::write(const string& ctrl_file, const string& value)
{
    std::ofstream file(ctrl_file.c_str()); // Ĭ�ϸ���д
    if (!file.is_open()) 
    {
        NEST_LOG(LOG_ERROR, "file %s open failed(%m)", ctrl_file.c_str());
        return false;
    }

    file << value << std::endl;

    bool ret = false;
    if (file.fail()) 
    {
        NEST_LOG(LOG_ERROR, "file [%s] write [%s] failed(%m)", ctrl_file.c_str(),
            value.c_str());
        ret = false;
    }
    else
    {
        ret = true;
    }

    file.close();
    return ret;
}


// �ļ���Ŀ¼�Ƿ����
bool CGroupMng::exists(const string& path)
{
    struct stat s;
    if (::lstat(path.c_str(), &s) < 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}


// ��������������
Hierarchy::Hierarchy(const string& name)
{
    _name = name;
}

// �Ƿ���ִ�г�ʼ��
bool Hierarchy::enabled()
{
    // 1. ϵͳ��֧��cgroup
    if (!CGroupMng::enabled())
    {
        NEST_LOG(LOG_DEBUG, "cgroups kenerl unsupported");
        return false;
    }

    // 2. check cgroup version
    if(0!=CGroupMng::check_version())
    {
        NEST_LOG(LOG_DEBUG, "cgroups check version failed");
        return false;        
    }

     _name = CGroupMng::cgroup_root;

    // 3. �����Ƿ񴴽�; versionΪ1ʱ��ϵͳĬ��mount��
    if(CGroupMng::cgroup_version == 0)
    {
        string opts;
        string mnt_type = "cgroup";
        if (!CGroupMng::mounted(CGroupMng::cgroup_root, opts, mnt_type))
        {
            NEST_LOG(LOG_DEBUG, "cgroups root uninited");
            return false;
        }
    }
    else
    {
         return false; // Ϊ�˼�鲢���� tools_cgroups
    }

	if(!reset())
	{
		return false;
	}

    return true;
}

#include <sys/types.h>
#include <dirent.h>

static bool listCGroupDir(const string& src, vector<string>& items)
{
	DIR *dir;
	struct dirent *ptr;
	vector<string> tmpItems;
	dir = opendir(src.c_str());
	if(dir==NULL)
	{
        NEST_LOG(LOG_ERROR, "listCGroupDir (%s) failed, %m", src.c_str());
        return false;  		
	}

	while((ptr=readdir(dir))!=NULL)
	{
		if(ptr->d_type ==DT_DIR && ptr->d_name!=NULL&&ptr->d_name[0]!='.'&&strncmp("nest",ptr->d_name,4)!=0)
		{
			tmpItems.push_back(ptr->d_name);
		}
	}
	items.swap(tmpItems);
	return true;
}

static bool load_cgroup_tasks(const string& path,vector<uint32_t>& pids)
{
	// ֧�ֵ�tasks �ַ����64k����������ȫ����Ҫ��
	static char buf[65536]={'\0'};
	int wr_pos = 0;
	int buf_len = sizeof(buf)-1;
	int rd_len = 0;
	char* pre_ptr = 0;
	char* cf_ptr = 0;
	string tasks_path;
	int fd = -1;
	uint32_t pid = 0;

	stringstream ss;
	ss<<path<<"/tasks";
	tasks_path = ss.str();
	bool ret = true;

	fd = open(tasks_path.c_str(), O_RDONLY);
	if(fd<0)
	{
		NEST_LOG(LOG_ERROR,"open cgroup [%s] tasks failed, %m", tasks_path.c_str());
		return false;
	}

	//read tasks
	while((rd_len=read(fd,buf+wr_pos,buf_len-wr_pos)) > 0)
	{
		wr_pos += rd_len;
	}

	if(rd_len<0)
	{
		NEST_LOG(LOG_ERROR,"read cgroup [%s] tasks failed, %m", tasks_path.c_str());
		ret = false;
		goto EXIT_LOAD;
	}

	// ��tasks
	if(wr_pos==0)
	{
		goto EXIT_LOAD;		
	}

	buf[wr_pos]='\0';

	pids.clear();

	pre_ptr = buf;
	cf_ptr=NULL;

	
	while(true)
	{
		cf_ptr = strchr(pre_ptr,(int)'\n');
		if(cf_ptr==NULL)
		{
			break;
		}

		pid = (uint32_t)atoll(pre_ptr);

		if(pid==0)
		{
			NEST_LOG(LOG_ERROR,"read cgroup [%s] tasks pos[%u] failed, %m", tasks_path.c_str(),(uint32_t)(pre_ptr-buf));
			ret = false;		
			break;
		}

		pids.push_back(pid);

		pre_ptr = cf_ptr+1;
	}
	
EXIT_LOAD:
	if(fd>0)
	{
		close(fd);	
	}

	return ret;
}



static bool move_cgroup_tasks(const string& name)
{
	string cpu_src_path = get_cgroup_cpu_path(name, false);
	string cpu_dst_path = get_cgroup_cpu_path(name,true);

	// ��·��������·�������ڣ�����ҪǨ��tasks��
	if((!CGroupMng::exists(cpu_src_path))||(!CGroupMng::exists(cpu_dst_path)))
	{
		return true;
	}	

	// ����tasks
	// 2. ����task, д��tasks
    CGroup group(CGroupMng::cgroup_root, name);

	vector<uint32_t> pids;

	if(!load_cgroup_tasks(cpu_src_path, pids))
	{
		NEST_LOG(LOG_ERROR, "cgroup %s load old tasks from %s failed", name.c_str(), cpu_src_path.c_str());
		return false;
	}


	if(pids.empty())
	{
		return true;
	}

	vector<uint32_t>::iterator it = pids.begin();
	for(;it!=pids.end();++it)
	{
		if(!group.add_task(*it))
		{
	        NEST_LOG(LOG_ERROR, "cgroup %s add task %u failed", name.c_str(), *it);
	        return false;			
		}
	}
	
	
	return true;
}


bool Hierarchy::reset()
{
	// reset root cpu/mem setting
	if(!resetRoot())
	{
		return false;
	}

	if(!resetTools())
	{
		return false;
	}

	vector<string> cgroup_dirs;
	string cpu_root = get_cgroup_cpu_root(true);
	
	if(!listCGroupDir(cpu_root.c_str(), cgroup_dirs))
	{
		return true;
	}

	// ��ϵͳ��û��tools����service
	if(cgroup_dirs.empty())
	{
		return true;
	}

	vector<string>::iterator it = cgroup_dirs.begin();
	for(;it!=cgroup_dirs.end();++it)
	{
		if(*it == "tools_cgroups")
		{
			continue;
		}
		
		if(!resetService(*it))
		{
			return false;
		}
	}	

	//string old_cpu_root = get_cgroup_cpu_root(false);
	//cgroup_dirs.clear();
	
	//if(!listCGroupDir(old_cpu_root.c_str(), cgroup_dirs))
	//{
	//	return true;
	//}

	// ��ϵͳ��û��tools����service
	//if(cgroup_dirs.empty())
	//{
	//	return true;
	//}
    // ֻ������Ŀ¼�´��ڵ�
	it = cgroup_dirs.begin();
	for(;it!=cgroup_dirs.end();++it)
	{		
		if(!move_cgroup_tasks(*it))
		{
			NEST_LOG(LOG_ERROR, "move cgroup [%s] tasks failed, %m", it->c_str());
			return false;
		}
	}		
	
	return true;
}

bool Hierarchy::resetRoot()
{
    // 1. ����ϵͳ������Դ
    CGroup sys(CGroupMng::cgroup_root);
    if(!sys.init())
    {
        NEST_LOG(LOG_ERROR, "sys CGroup init failed");
        return false;        
    }

    if(!sys.set_cpu_quota(100-CGroupMng::cgroup_conf.cpu_sys_reserved_percent))
    {
        NEST_LOG(LOG_ERROR, "sys CGroup cpu quota failed");
        return false;         
    }


    if(!sys.set_memory_reserved(((uint64_t)CGroupMng::cgroup_conf.mem_sys_reserved_mb)<<20))
    {
        NEST_LOG(LOG_ERROR, "sys CGroup mem reserved failed");
        return false;  
    }

    if(!sys.enable_memory_hierarchy())
    {
        NEST_LOG(LOG_ERROR, "sys CGroup mem use_hierarchy failed");
        return false;         
    }
    
    NEST_LOG(LOG_TRACE, "sys CGroup resert ok");
    return true;
}

bool Hierarchy::resetTools()
{
    // ����Ĭ�ϵ�tools cgroup
    CGroup tools(CGroupMng::cgroup_root, "tools_cgroups");

    if (!tools.init())
    {
        NEST_LOG(LOG_ERROR, "tools CGroup init failed");
        return false;
    }

    if (!tools.set_cpu_quota(CGroupMng::cgroup_conf.cpu_tools_percent)) // tools���ռ��CPU��20%
    {
        NEST_LOG(LOG_ERROR, "set cgroup [tools] cpu quota failed");
        return false;
    }

    if(!tools.set_memory_percent(CGroupMng::cgroup_conf.mem_tools_percent))
    {
        NEST_LOG(LOG_ERROR, "set cgroup [tools] mem percent failed");
        return false;
    }

    if(!tools.enable_memory_hierarchy())
    {
        NEST_LOG(LOG_ERROR, "set cgroup [tools] mem use_hierarchy failed");
        return false;        
    }
    
    return true;
}

bool Hierarchy::resetService(const string& name)
{
    // ����Ĭ�ϵ�tools cgroup
    CGroup service(CGroupMng::cgroup_root, name);

    if (!service.init())
    {
        NEST_LOG(LOG_ERROR, "service CGroup [%s] init failed", name.c_str());
        return false;
    }

    if (!service.set_cpu_quota(CGroupMng::cgroup_conf.cpu_service_percent)) // tools���ռ��CPU��20%
    {
        NEST_LOG(LOG_ERROR, "set cgroup [%s] cpu quota failed", name.c_str());
        return false;
    }

    if(!service.set_memory_percent(CGroupMng::cgroup_conf.mem_service_percent))
    {
        NEST_LOG(LOG_ERROR, "set cgroup [%s] mem percent failed", name.c_str());
        return false;
    }

    if(!service.enable_memory_hierarchy())
    {
        NEST_LOG(LOG_ERROR, "set cgroup [%s] mem use_hierarchy failed", name.c_str());
        return false;        
    }
    
    return true;
}

// ������Hierarchy��Ϣ
bool Hierarchy::init(const string& sub_systems)
{
    if(CGroupMng::cgroup_version == 0)
    {
        // 1. ��鲢���� /cgroups Ŀ¼
        if (!CGroupMng::exists(CGroupMng::cgroup_root))
        {
            if (::mkdir(CGroupMng::cgroup_root.c_str(), 0755) < 0)
            {
                NEST_LOG(LOG_ERROR, "make root path [%s] failed(%m)", CGroupMng::cgroup_root.c_str());
                return false;
            }
        }

        // 2. mount��ϵͳ��Ϣ
        if (!CGroupMng::mount(CGroupMng::cgroup_root, CGroupMng::subsystem))
        {
            NEST_LOG(LOG_ERROR, "mount [%s] system [%s] failed(%m)", CGroupMng::cgroup_root.c_str(),
                    CGroupMng::subsystem.c_str());
            return false;
        }
    }

    if(!reset())
    {
    	return false;
    }

    return true;
}




// ��������������
CGroup::CGroup(const string& root, const string& group_name)
{
    init_inner(root,group_name);
}

CGroup::CGroup(const string& root)
{
    string group_name;
    init_inner(root,group_name);
}

bool CGroup::init_inner(const string & root, const string & group_name)
{
    _parent = root;
    _name   = group_name;

    assert(CGroupMng::cgroup_version==1 || CGroupMng::cgroup_version==0);
    _cpu_path = get_cgroup_cpu_path(_name);
    _mem_path = get_cgroup_mem_path(_name);

    return true;
}

// �Ƿ���ִ�г�ʼ��, ���ж�, Ŀ¼���ڼ���
bool CGroup::enabled()
{
    return CGroupMng::exists(_cpu_path)&&CGroupMng::exists(_mem_path);
}

// ������group��Ϣ, ��ʼ��������Ĭ�ϵ�share��limit
bool CGroup::init()
{
    // 1. ϵͳ��֧��cgroup, ֱ���˳�
    if (!CGroupMng::enabled())
    {
        NEST_LOG(LOG_DEBUG, "cgroups kenerl unsupported");
        return false;
    }

    // 2. ����δ����, ֱ���˳�
    if (!CGroupMng::exists(_parent))
    {
        NEST_LOG(LOG_DEBUG, "parents cgroups [%s] not inited", _parent.c_str());
        return false;
    }

    // 3. ��������Ŀ¼��Ϣ
    // cpu
    if (!CGroupMng::exists(_cpu_path))
    {
        if (::mkdir(_cpu_path.c_str(), 0755) < 0)
        {
            NEST_LOG(LOG_ERROR, "make group path [%s] failed(%m)", _cpu_path.c_str());
            return false;
        }
    }
    // memory
    if (!CGroupMng::exists(_mem_path))
    {
        if (::mkdir(_mem_path.c_str(), 0755) < 0)
        {
            NEST_LOG(LOG_ERROR, "make group path [%s] failed(%m)", _mem_path.c_str());
            return false;
        }
    }    

    // 4. ����Ĭ�ϵ�cpushare limit
    return true;
}



// �����group��Ϣ
bool CGroup::term()
{
    // cpu
    if (CGroupMng::exists(_cpu_path) && ::rmdir(_cpu_path.c_str()) < 0)
    {
        NEST_LOG(LOG_ERROR, "rm group path [%s] failed(%m)", _cpu_path.c_str());
        return false;
    }

    // memory
    if (CGroupMng::exists(_mem_path) && ::rmdir(_mem_path.c_str()) < 0)
    {
        NEST_LOG(LOG_ERROR, "rm group path [%s] failed(%m)", _mem_path.c_str());
        return false;
    }

    return true;
}


// ����task��Ϣ
bool CGroup::add_task(uint32_t pid)
{
    // cpu
    {
        std::ostringstream out;
        out << _cpu_path << "/" << "tasks";
        string path = out.str();

        std::ostringstream val;
        val << pid;
        string value = val.str();
        
        if (!CGroupMng::write(path, value))
        {
            NEST_LOG(LOG_ERROR, "write pid[%u] to path [%s] failed(%m)", pid, path.c_str());
            return false;
        }
    }

    // mem
    {
        std::ostringstream out;
        out << _mem_path << "/" << "tasks";
        string path = out.str();

        std::ostringstream val;
        val << pid;
        string value = val.str();
        
        if (!CGroupMng::write(path, value))
        {
            NEST_LOG(LOG_ERROR, "write pid[%u] to path [%s] failed(%m)", pid, path.c_str());
            return false;
        }
    }

    

    return true;
}

// ����cpu share����
bool CGroup::set_cpu_shares(uint32_t share)
{
    std::ostringstream out;
    out << _cpu_path << "/" << "cpu.shares";
    string path = out.str();

    std::ostringstream val;
    val << share;
    string value = val.str();
    
    if (!CGroupMng::write(path, value))
    {
        NEST_LOG(LOG_ERROR, "write cpu share[%u] to path [%s] failed(%m)", share, path.c_str());
        return false;
    }

    return true;
}

// ����memory limit
bool CGroup::set_memory_limit(uint64_t limit)
{
    std::ostringstream out;
    out << _mem_path << "/" << "memory.limit_in_bytes";
    string path = out.str();

    std::ostringstream val;
    val << limit;
    string value = val.str();
    
    if (!CGroupMng::write(path, value))
    {
        NEST_LOG(LOG_ERROR, "write mem limit[%llu] to path [%s] failed(%m)", limit, path.c_str());
        return false;
    }

    return true;
}

// ����memory limit
bool CGroup::set_memory_swap_limit(uint64_t limit)
{
    std::ostringstream out;
    out << _mem_path << "/" << "memory.memsw.limit_in_bytes";
    string path = out.str();

    std::ostringstream val;
    val << limit;
    string value = val.str();
    
    if (!CGroupMng::write(path, value))
    {
        NEST_LOG(LOG_ERROR, "write memsw limit[%llu] to path [%s] failed(%m)", limit, path.c_str());
        return false;
    }

    return true;
}


// ����memory limit��ͬʱ����memory.limit_in_bytes �� memory.memsw.limit_in_bytes
bool CGroup::set_memory_quota(uint64_t limit)
{
    // memory.memsw.limit_in_bytes >= memory.limit_in_bytes; ������Լ����������޸�ʧ�ܣ�
    // �޸�  memory.limit_in_bytes �ɹ������޸� memory.memsw.limit_in_bytes����
    if(set_memory_limit(limit))
    {
        return set_memory_swap_limit(limit);
    }

    // �޸�  memory.limit_in_bytes ʧ�ܣ�Ӧ����Υ����Լ������Ҫ���޸�memory.memsw.limit_in_bytes
    // ���޸� memory.limit_in_bytes
    return set_memory_swap_limit(limit)&& set_memory_limit(limit);
}

bool CGroup::set_memory_percent(uint32_t percent)
{
    CSysInfo sys;
    uint64_t total_mem = (uint64_t)sys.GetMemTotal()*1024;

    return set_memory_quota(total_mem*percent/100);
}

bool CGroup::set_memory_reserved(uint64_t reserved)
{
    CSysInfo sys;
    uint64_t total_mem = (uint64_t)sys.GetMemTotal()*1024;
    if(total_mem<reserved)
    {
        return -1;
    }

    return set_memory_quota(total_mem-reserved);
}

bool CGroup::enable_memory_hierarchy()
{
    return set_memory_hierarchy(true);
}

bool CGroup::disable_memory_hierarchy()
{
    return set_memory_hierarchy(false);
}

bool CGroup::set_memory_hierarchy(bool flag)
{
    std::ostringstream out;
    out << _mem_path << "/" << "memory.use_hierarchy";
    string path = out.str();

    std::ostringstream val;
    val << (flag?1:0);
    string value = val.str();
    
    if (!CGroupMng::write(path, value))
    {
        NEST_LOG(LOG_ERROR, "write memory.use_hierarchy [%d] to path [%s] failed(%m)", (flag?1:0), path.c_str());
        return false;
    }

    return true;    
}

// ����cpu quota
bool CGroup::set_cpu_quota(uint32_t percent)
{
    if (!percent || percent >= 100)
    {
        NEST_LOG(LOG_ERROR, "invalid cpu quota percent(%u)", percent);
        return false;
    }

    uint32_t cpu_period = 1000000;
    uint32_t cpu_num = CSysInfo::GetCpuNums();
    uint64_t quota = (uint64_t)cpu_num * cpu_period * percent / 100;

    // 1. д��cpu.cfs_quota_us
    std::ostringstream out;
    out << _cpu_path << "/" << "cpu.cfs_quota_us";
    string quota_path = out.str();

    std::ostringstream val;
    val << quota;
    string quota_value = val.str();

    // 2. д��cpu.cfs_period_us
    std::ostringstream out1;
    out1 << _cpu_path << "/" << "cpu.cfs_period_us";
    string period_path = out1.str();

    std::ostringstream val1;
    val1 << cpu_period;
    string period_value = val1.str();
    
    if((CGroupMng::write(period_path, period_value) && CGroupMng::write(quota_path, quota_value))
       ||( CGroupMng::write(quota_path, quota_value) && CGroupMng::write(period_path, period_value)))
    {
        return true;
    }


    NEST_LOG(LOG_ERROR, "write cpu (quota=%llu, period=%u) to path [%s] failed(%m)", quota, cpu_period,_cpu_path.c_str());

    return false;
}



/**
 * @brief CGroupȫ�ֹ�����Ϣ, ���캯��
 */
CAgentCGroups::CAgentCGroups()
{
    // 1. Ĭ�ϲ�֧��cgroups
    _cgroup_inited = false;
    if (!CGroupMng::enabled())
    {
        NEST_LOG(LOG_ERROR, "proc cgroups not exist");
        return;
    }

    // 2. ����root��
    Hierarchy root(CGroupMng::cgroup_root);
    if (root.enabled())
    {
        _cgroup_inited = true; 
        return;
    }
    
    if (!root.init(CGroupMng::subsystem))
    {
        NEST_LOG(LOG_ERROR, "cgroup root init failed");
        return;
    }

    // 3. �ɹ�����
    _cgroup_inited = true; 
    return;
}


// ҵ��group�Ƿ񴴽�
bool CAgentCGroups::service_group_create(const string& service)
{
    // 1. �޷�����cgroups�ж�
    if (!this->cgroup_inited())
    {
        NEST_LOG(LOG_ERROR, "cgroup root uninit failed");
        return false;
    }

    // 2. �Ѿ�����, �ⴴ��
    CGroup group(CGroupMng::cgroup_root, service);
    if (group.enabled())
    {
        NEST_LOG(LOG_DEBUG, "cgroup %s inited already", service.c_str());
        return true;
    }

    // 3. ��ʼ������
    if (!group.init())
    {
        NEST_LOG(LOG_ERROR, "cgroup %s init failed", service.c_str());
        return false;
    }

    // 4. ����Ĭ���ڴ�ģ��
    if (!group.set_memory_percent(CGroupMng::cgroup_conf.mem_service_percent))
    {
        NEST_LOG(LOG_ERROR, "cgroup %s set mem %u % limit failed", service.c_str(), CGroupMng::cgroup_conf.mem_service_percent);
        group.term();
        return false;
    }

    // 5. ����CPU�컨�壬���ռ��40%
    if (!group.set_cpu_quota(CGroupMng::cgroup_conf.cpu_service_percent))
    {
        NEST_LOG(LOG_ERROR, "cgroup %s set cpu limit failed", service.c_str());
        group.term();
        return false;
    }

    if(!group.enable_memory_hierarchy())
    {
        NEST_LOG(LOG_ERROR, "cgroup %s set mem use_hierarchy failed", service.c_str());
        group.term();
        return false;        
    }

    return true;
}

// ҵ��group���������Ϣ
bool CAgentCGroups::service_group_add_task(const string& service, uint32_t pid)
{
    // 1. �޷�����cgroups�ж�
    if (!service_group_create(service))
    {
        NEST_LOG(LOG_ERROR, "cgroup %s init failed", service.c_str());
        return false;
    }

    // 2. ����task, д��tasks
    CGroup group(CGroupMng::cgroup_root, service);
    if (!group.add_task(pid))
    {
        NEST_LOG(LOG_ERROR, "cgroup %s add task %u failed", service.c_str(), pid);
        return false;
    }

    // 3. �ɹ�����
    return true;
}



