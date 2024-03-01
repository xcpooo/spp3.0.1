/**
 * @file nest_context.h
 * @info ������ӳ�����, Ϊ��֤������so����, ��Ҫ����ά��������
 */

#ifndef _NEST_CONTEXT_H__
#define _NEST_CONTEXT_H__

#include <map>
#include "hash_list.h"
#include "list.h"
#include "allocate_pool.h"

using namespace spp::comm;  // list.h

namespace nest
{

    class CMsgCtx;                                  // ��Ϣ������    
    typedef CListObject<CMsgCtx>    CListCtx;       // ��Ϣ˫����
    typedef CAllocatePool<CMsgCtx>  CMsgCtxPool;    // ��Ϣ�ڴ��
    typedef std::map<uint32_t, CMsgCtx*> CMsgCtxMap; // ��Ϣӳ��map

    /**
     * @brief Tcp�ڲ�������Э��ͷ���� 8�ֽڶ���, �����̶����Ȳ���, ��TLV��չ�������װ
     * @info  ��������չTLV�Ļ�����Ҫ�ϸ�Ҫ����int32���ͣ���֤��gap
     */
    struct TNetCommuHead
    {
        uint32_t    magic_num;          // ��Ϣ��� NEST
        uint32_t    msg_len;            // �ְ����ȱ��
        uint32_t    head_len;           // ��չͷ����, ��ͬ�汾��׷���ֶμ���
        uint32_t    msg_type;           // ��Ϣ����, 0 ������Ϣ, ����Ϊ������Ϣ
        uint32_t    route_id;           // ·�ɷַ���Ϣ
        uint32_t    flow;               // 4�ֽڵ���id
        uint32_t    crc;                // crcֵ
        uint32_t    from_ip;            // ipv4 ��ַ, �������Ĵ���, ��δʹ��
        uint32_t    from_port;          // tcp�˿�, �������Ĵ���, ��δʹ��
        uint32_t    rcv_sec;            // ʱ���, ��
        uint32_t    rcv_usec;           // ʱ���, ΢��
        uint32_t    time_cost;          // ��˴���ʱ��, ���ڼ������罻��ʱ��
    };

    /**
     * @brief ��Ϣ�����Ķ�����
     */
    class CMsgCtx : public CListObject<CMsgCtx>
    {
    public:

        /**
         * @brief ��������������
         */
        CMsgCtx() {
            _time_start = 0;
        };

        virtual ~CMsgCtx() {};

        /**
         * @brief ��ȡ��������ʼʱ��ms
         */
        uint64_t get_start_time() {
            return _time_start;
        };

        void set_start_time(uint64_t start) {
            _time_start = start;    
        };

        /**
         * @brief ��¼���ȡ��������Ϣ
         */
        TNetCommuHead& get_msg_head() {
            return _msg_head;
        };
        void set_msg_head(TNetCommuHead& head) {
            _msg_head = head;
        };

        /**
         * @brief �������ȡ��id��Ϣ
         */
        uint32_t get_flow_id() {
            return _msg_head.flow;
        };

        void set_flow_id(uint32_t flow) {
            _msg_head.flow = flow;
        };

    protected:

        uint64_t        _time_start;        //  ��ʼ��ʱ��� ms
        TNetCommuHead   _msg_head;          //  ��������������
    };


    /**
     * @brief ��Ϣ�����Ĺ������
     */
    class CMsgCtxMng
    {
    public:

        /**
         * @brief ��������������
         */
        CMsgCtxMng();        
        ~CMsgCtxMng();

        /**
         * @brief ����һ�������Ķ���
         */
        CMsgCtx* alloc_ctx();

        /**
         * @brief �ͷ�һ�������Ķ���
         */
        void free_ctx(CMsgCtx* ctx);

        /**
         * @brief ���������Ķ���
         */
        bool insert_ctx(CMsgCtx* ctx);

        /**
         * @brief ɾ�������Ķ���
         */
        void remove_ctx(CMsgCtx* ctx);

        /**
         * @brief ��ѯ�����Ķ���
         */
        CMsgCtx* find_ctx(uint32_t flow);

        /**
         * @brief ��������Ķ���ʱ
         */
        void check_expired(uint64_t now, uint32_t wait);
        
    protected:

        CMsgCtxMap      _ctx_map;           // ������map
        CListCtx        _link_list;         // ��ʱ˫����
        CMsgCtxPool     _mem_pool;          // �ڴ��
    };


}


#endif

