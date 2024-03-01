/**
 * @brief nest msg context process
 */

#include "nest_context.h"
#include "nest_log.h"

using namespace nest;


/**
 * @brief 构造函数
 */
CMsgCtxMng::CMsgCtxMng()
{
}

/**
 * @brief 析构函数
 */
CMsgCtxMng::~CMsgCtxMng()
{
    while (!_link_list.ListEmpty()) 
    {
        CMsgCtx* ctx = _link_list.NextOwner();
        this->remove_ctx(ctx);
        this->free_ctx(ctx);
    }
}

/**
 * @brief 分派一个上下文对象
 */
CMsgCtx* CMsgCtxMng::alloc_ctx()
{
    return _mem_pool.alloc_ptr();
}

/**
 * @brief 释放一个上下文对象
 */
void CMsgCtxMng::free_ctx(CMsgCtx* ctx)
{
    return _mem_pool.free_ptr(ctx);
}

/**
 * @brief 插入上下文对象
 */
bool CMsgCtxMng::insert_ctx(CMsgCtx* ctx)
{
    uint32_t flow = ctx->get_flow_id();
    _ctx_map[flow] = ctx;
    ctx->ListMoveTail(_link_list);
    return true;
}

/**
 * @brief 删除上下文对象
 */
void CMsgCtxMng::remove_ctx(CMsgCtx* ctx)
{
    uint32_t flow = ctx->get_flow_id();
    _ctx_map.erase(flow);
    ctx->ResetList();
}

/**
 * @brief 查询上下文对象
 */
CMsgCtx* CMsgCtxMng::find_ctx(uint32_t flow)
{
    CMsgCtxMap::iterator it = _ctx_map.find(flow);
    if (it == _ctx_map.end())
    {
        return NULL;
    }

    return it->second;
}

/**
 * @brief 检查上下文对象超时
 */
void CMsgCtxMng::check_expired(uint64_t now, uint32_t max_wait)
{
    while (!_link_list.ListEmpty()) 
    {
        CMsgCtx* ctx = _link_list.NextOwner();
        if ((ctx->get_start_time() + max_wait) > now) {
            break;
        }
        
        this->remove_ctx(ctx);
        this->free_ctx(ctx);
    }
}



