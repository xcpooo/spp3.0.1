/**
 * @brief �񳲴������ù���
 */

#ifndef _NEST_AGENT_CONF_H__
#define _NEST_AGENT_CONF_H__

#include "stdint.h"
#include "moduleconfbase.h"

using namespace spp::tconfbase;

namespace nest
{
	
    /**
     * @brief Agent��������Ϣ����
     */
    class CAgentConf : public CModuleConfBase
    {
    public:

        // ��������������
        CAgentConf(CLoadConfBase* pConf);
        virtual ~CAgentConf();

        // ��ʼ����ȡ�ӿ�        
        int32_t init();

        // ��ȡLOG������, ����, ��־����
        int32_t getAndCheckFlog(Log& flog);

        // agent��Ⱥ�������
        int32_t getAndCheckCluster(ClusterConf& cluster);

		int32_t getAndCheckCGroupConf(CGroupConf& cgroup_conf);
    protected:

        // �̳нӿ�, Ĭ�Ͽ����ʵ��
        int32_t loadFlog();
        int32_t loadFStat() { return 0;};
        int32_t loadMoni() { return 0;};

        // ��ȡȫ������, ����չ
        int32_t loadCtrl();

        // ��־���ýӿ�У��
        int32_t checkFlog();

        // ȫ������У��
        int32_t checkCtrl();

        // agent��Ⱥ�������
        int32_t checkCluster();

    public:
        ClusterConf _cluster;  // agent��Ⱥ�������
        CGroupConf  _cgroup_conf;
    };

}

#endif
