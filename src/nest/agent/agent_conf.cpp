/**
 * @brief �񳲴������ù���
 */
#include <string>

#include "agent_conf.h"


using namespace spp::tconfbase;
using namespace nest;



///< ���캯��ʵ��
CAgentConf::CAgentConf(CLoadConfBase* pConf):CModuleConfBase(pConf)
{
    return;
}

///< ��������ʵ��
CAgentConf::~CAgentConf()
{
    return;
}

///< ����XML����, ��ȡȫ��������Ϣ
int32_t CAgentConf::init()
{
    int32_t ret = 0;
    ret = CModuleConfBase::init();
    if (ret != 0) {
        return ret;
    }
    
    ret = loadCtrl();
    if (ret != 0) {
        return ret;
    }
    
    return checkCtrl();
}

///< ��ȡXML����־����, У�鲢�ṩĬ��ֵ
int32_t CAgentConf::getAndCheckFlog(Log& flog)
{
    int32_t ret = loadFlog();
    if (ret != 0)
    {
        flog = _flog;
        return ret;
    }
    
    ret = checkFlog();
    flog = _flog;
    
    return ret;
}

///< ��ȡagent��Ⱥ�������
int32_t CAgentConf::getAndCheckCluster(ClusterConf& cluster)
{
    int32_t ret = _pLoadConf->getClusterValue(_cluster, getArgvs(2, "agent", "cluster"));
    if (ret != 0)
    {
        cluster = _cluster;
        return ret;
    }

    ret = checkCluster();
    cluster = _cluster;

    return ret;
}

///< ���agent��Ⱥ�������
int32_t CAgentConf::checkCluster()
{
    if ((_cluster.type != "sf2") && (_cluster.type != "spp"))
    {
        return ERR_CONF_CHECK_UNPASS;
    }

    return ERR_MODULE_CONF_SUCCESS;
}

///< ��ȡXML����־����
int32_t CAgentConf::loadFlog()
{
    _flog.clear("nest_agent");
    _flog.type = 3;    // default normal 
    
    return _pLoadConf->getLogValue(_flog, getArgvs(2, "agent", "flog"));
}

///< ��ȡȫ�ֵ�����
int32_t CAgentConf::loadCtrl()
{
    return 0;
}

///< �����־������
int32_t CAgentConf::checkFlog()
{
    _flog.type = LOG_TYPE_CYCLE;
    _flog.path = "../log";
    _flog.prefix = "nest_agent";
    
    return checklog("flog", _flog);
}

///< ���ȫ�ֵ�����
int32_t CAgentConf::checkCtrl()
{
    return 0;
}

int32_t CAgentConf::getAndCheckCGroupConf(CGroupConf& cgroup_conf)
{
    int32_t ret = _pLoadConf->getCGroupConfValue(_cgroup_conf, getArgvs(2, "agent", "cgroup"));
    if (ret != 0)
    {
        cgroup_conf = _cgroup_conf;
        return ret;
    }

    ret = _cgroup_conf.check();
    cgroup_conf = _cgroup_conf;

    return ret;    
}

