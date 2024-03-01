/**
 *  @file mt_mbuf_pool.cpp
 *  @info ΢�߳���Ϣbuf�ع���ʵ��
 *  @time 20130924
 **/

#include "micro_thread.h"
#include "mt_mbuf_pool.h"

using namespace std;
using namespace NS_MICRO_THREAD;


/**
 * @brief ���ӹ���ȫ�ַ��ʽӿ�
 * @return ȫ�־��ָ��
 */
MsgBuffPool* MsgBuffPool::_instance = NULL;
MsgBuffPool* MsgBuffPool::Instance (void)
{
    if (NULL == _instance)
    {
        _instance = new MsgBuffPool;
    }

    return _instance;
}

/**
 * @brief ���ӹ���ȫ�ֵ����ٽӿ�
 */
void MsgBuffPool::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}


/**
 * @brief ��Ϣbuff�Ĺ��캯��
 */
MsgBuffPool::MsgBuffPool(int max_free)
{
    _max_free = max_free;
    _hash_map = new HashList(10000);
}

/**
 * @brief ��Ϣbuff����������
 */
MsgBuffPool::~MsgBuffPool()
{
    if (!_hash_map) {
        return;
    }

    MsgBufMap* msg_map = NULL;
    HashKey* hash_item = _hash_map->HashGetFirst();
    while (hash_item)
    {
        _hash_map->HashRemove(hash_item);
        msg_map = dynamic_cast<MsgBufMap*>(hash_item);
        delete msg_map;
        
        hash_item = _hash_map->HashGetFirst();
    }

    delete _hash_map;
    _hash_map = NULL;
}

/**
 *  @brief ��ȡ��ϢbuffԪ��
 *  @return msgbufָ��, ʧ��ΪNULL
 */
MtMsgBuf* MsgBuffPool::GetMsgBuf(int max_size)
{
    if (!_hash_map) {
        MTLOG_ERROR("MsgBuffPoll not init! hash %p,", _hash_map);
        return NULL;
    }

    MsgBufMap* msg_map = NULL;
    MsgBufMap msg_key(max_size);
    HashKey* hash_item = _hash_map->HashFind(&msg_key);
    if (hash_item) {
        msg_map = (MsgBufMap*)hash_item->GetDataPtr();
        if (msg_map) {
            return msg_map->GetMsgBuf();
        } else {
            MTLOG_ERROR("Hash item: %p, msg_map: %p impossible, clean it", hash_item, msg_map);
            _hash_map->HashRemove(hash_item);
            delete hash_item;
            return NULL;
        }
    } else {
        msg_map = new MsgBufMap(max_size, _max_free);
        if (!msg_map) {
            MTLOG_ERROR("maybe no more memory, failed. size: %d", max_size);
            return NULL;
        }
        _hash_map->HashInsert(msg_map);
        
        return msg_map->GetMsgBuf();
    }
}

/**
 *  @brief ��ȡ��ϢbuffԪ��
 *  @return msgbufָ��, ʧ��ΪNULL
 */
void MsgBuffPool::FreeMsgBuf(MtMsgBuf* msg_buf)
{
    if (!_hash_map || !msg_buf) {
        MTLOG_ERROR("MsgBuffPoll not init or input error! hash %p, msg_buf %p", _hash_map, msg_buf);
        delete msg_buf;
        return;
    }
    msg_buf->Reset();

    MsgBufMap* msg_map = NULL;
    MsgBufMap msg_key(msg_buf->GetMaxLen());
    HashKey* hash_item = _hash_map->HashFind(&msg_key);
    if (hash_item) {
        msg_map = (MsgBufMap*)hash_item->GetDataPtr();
    }
    if (!hash_item || !msg_map) {
        MTLOG_ERROR("MsgBuffPoll find no queue, maybe error: %d", msg_buf->GetMaxLen());
        delete msg_buf;
        return;
    }
    msg_map->FreeMsgBuf(msg_buf);
    
    return;
}


