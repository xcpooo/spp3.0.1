/**
 * @brief nest msg context process
 */

#include "nest_context.h"
#include "nest_log.h"

using namespace nest;


/**
 * @brief ���캯��
 */
CMsgCtxMng::CMsgCtxMng()
{
}

/**
 * @brief ��������
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
 * @brief ����һ�������Ķ���
 */
CMsgCtx* CMsgCtxMng::alloc_ctx()
{
    return _mem_pool.alloc_ptr();
}

/**
 * @brief �ͷ�һ�������Ķ���
 */
void CMsgCtxMng::free_ctx(CMsgCtx* ctx)
{
    return _mem_pool.free_ptr(ctx);
}

/**
 * @brief ���������Ķ���
 */
bool CMsgCtxMng::insert_ctx(CMsgCtx* ctx)
{
    uint32_t flow = ctx->get_flow_id();
    _ctx_map[flow] = ctx;
    ctx->ListMoveTail(_link_list);
    return true;
}

/**
 * @brief ɾ�������Ķ���
 */
void CMsgCtxMng::remove_ctx(CMsgCtx* ctx)
{
    uint32_t flow = ctx->get_flow_id();
    _ctx_map.erase(flow);
    ctx->ResetList();
}

/**
 * @brief ��ѯ�����Ķ���
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
 * @brief ��������Ķ���ʱ
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



