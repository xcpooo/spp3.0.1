/**
 * @file SEXInterface.cpp (SEX = SppEXtra)
 * @brief 异步模式下的连接数上限管理
 * @author aoxu, aoxu@tencent.com
 * @version 1.0
 * @date 2011-10-10
 */
#include "NetMgr.h"
#include "RouteMgr.h"

USING_L5MODULE_NS;
using namespace spp::comm;

extern int g_spp_groupid;
extern int g_spp_groups_sum;

BEGIN_ASYNCFRAME_NS

// 该API用到的所有中间变量
static struct SEXVarContainer
{
    SEXVarContainer()
    : pAI(NULL), seq(0)
    {}

    void* pAI; // 保存正在处理的ActionInfo指针，用户可以在Action里调用
    uint64_t seq; // 保存多发多收框架正在处理的包序列号
} SVC;

//管理一组ip/port的异步连接数上限
void InitConnNumLimit(const std::string& ip, const unsigned short port, const unsigned limit)
{
	CNetMgr::Instance()->InitConnNumLimit(ip, port, limit);
}

//管理集成L5的异步连接数上限
//L5根据一组modid/cmdid来管理连接数上限，对应的每组ip/port都是最多limit个连接
void InitConnNumLimit(const int modid, const int cmdid, const unsigned int limit)
{
    CRouteMgr::Instance()->InitConnNumLimit(modid, cmdid, limit);
}

// Seq & ActionInfo Helper ===START====
void SetCurrentActionInfo(void* ptr)
{
    SVC.pAI = ptr;
}

void* GetCurrentActionInfo()
{
    return SVC.pAI;
}

void SetCurrentSeq(uint64_t seq)
{
    SVC.seq = seq;
}

uint64_t GetCurrentSeq()
{
    return SVC.seq;
}
// Seq & ActionInfo Helper ===END====

int GetGroupId()
{
    return g_spp_groupid;
}

int GetGroupsSum() {
    return g_spp_groups_sum;
}

END_ASYNCFRAME_NS
