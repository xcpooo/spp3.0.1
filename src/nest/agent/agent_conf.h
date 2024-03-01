/**
 * @brief 鸟巢代理配置管理
 */

#ifndef _NEST_AGENT_CONF_H__
#define _NEST_AGENT_CONF_H__

#include "stdint.h"
#include "moduleconfbase.h"

using namespace spp::tconfbase;

namespace nest
{
	
    /**
     * @brief Agent的配置信息管理
     */
    class CAgentConf : public CModuleConfBase
    {
    public:

        // 构造与析构函数
        CAgentConf(CLoadConfBase* pConf);
        virtual ~CAgentConf();

        // 初始化读取接口        
        int32_t init();

        // 读取LOG的配置, 级别, 日志名等
        int32_t getAndCheckFlog(Log& flog);

        // agent集群相关配置
        int32_t getAndCheckCluster(ClusterConf& cluster);

		int32_t getAndCheckCGroupConf(CGroupConf& cgroup_conf);
    protected:

        // 继承接口, 默认框架类实现
        int32_t loadFlog();
        int32_t loadFStat() { return 0;};
        int32_t loadMoni() { return 0;};

        // 读取全局配置, 待扩展
        int32_t loadCtrl();

        // 日志配置接口校验
        int32_t checkFlog();

        // 全局配置校验
        int32_t checkCtrl();

        // agent集群相关配置
        int32_t checkCluster();

    public:
        ClusterConf _cluster;  // agent集群相关配置
        CGroupConf  _cgroup_conf;
    };

}

#endif
