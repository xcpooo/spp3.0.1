/**
 * @file nest_context.h
 * @info 上下文映射管理, 为保证二进制so兼容, 需要额外维护上下文
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

    class CMsgCtx;                                  // 消息上下文    
    typedef CListObject<CMsgCtx>    CListCtx;       // 消息双链表
    typedef CAllocatePool<CMsgCtx>  CMsgCtxPool;    // 消息内存池
    typedef std::map<uint32_t, CMsgCtx*> CMsgCtxMap; // 消息映射map

    /**
     * @brief Tcp内部交互的协议头定义 8字节对齐, 超过固定长度部分, 用TLV扩展解析与封装
     * @info  新增可扩展TLV的话，需要严格要求是int32类型，保证无gap
     */
    struct TNetCommuHead
    {
        uint32_t    magic_num;          // 消息标记 NEST
        uint32_t    msg_len;            // 分包长度标记
        uint32_t    head_len;           // 扩展头长度, 不同版本，追加字段兼容
        uint32_t    msg_type;           // 消息类型, 0 数据消息, 其它为控制消息
        uint32_t    route_id;           // 路由分发信息
        uint32_t    flow;               // 4字节的流id
        uint32_t    crc;                // crc值
        uint32_t    from_ip;            // ipv4 地址, 防上下文错误, 暂未使用
        uint32_t    from_port;          // tcp端口, 防上下文错误, 暂未使用
        uint32_t    rcv_sec;            // 时间戳, 秒
        uint32_t    rcv_usec;           // 时间戳, 微秒
        uint32_t    time_cost;          // 后端处理时间, 用于计算网络交互时延
    };

    /**
     * @brief 消息上下文对象定义
     */
    class CMsgCtx : public CListObject<CMsgCtx>
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        CMsgCtx() {
            _time_start = 0;
        };

        virtual ~CMsgCtx() {};

        /**
         * @brief 获取与设置起始时间ms
         */
        uint64_t get_start_time() {
            return _time_start;
        };

        void set_start_time(uint64_t start) {
            _time_start = start;    
        };

        /**
         * @brief 记录与获取上下文信息
         */
        TNetCommuHead& get_msg_head() {
            return _msg_head;
        };
        void set_msg_head(TNetCommuHead& head) {
            _msg_head = head;
        };

        /**
         * @brief 设置与获取流id信息
         */
        uint32_t get_flow_id() {
            return _msg_head.flow;
        };

        void set_flow_id(uint32_t flow) {
            _msg_head.flow = flow;
        };

    protected:

        uint64_t        _time_start;        //  起始的时间点 ms
        TNetCommuHead   _msg_head;          //  具体上下文数据
    };


    /**
     * @brief 消息上下文管理对象
     */
    class CMsgCtxMng
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        CMsgCtxMng();        
        ~CMsgCtxMng();

        /**
         * @brief 分派一个上下文对象
         */
        CMsgCtx* alloc_ctx();

        /**
         * @brief 释放一个上下文对象
         */
        void free_ctx(CMsgCtx* ctx);

        /**
         * @brief 插入上下文对象
         */
        bool insert_ctx(CMsgCtx* ctx);

        /**
         * @brief 删除上下文对象
         */
        void remove_ctx(CMsgCtx* ctx);

        /**
         * @brief 查询上下文对象
         */
        CMsgCtx* find_ctx(uint32_t flow);

        /**
         * @brief 检查上下文对象超时
         */
        void check_expired(uint64_t now, uint32_t wait);
        
    protected:

        CMsgCtxMap      _ctx_map;           // 上下文map
        CListCtx        _link_list;         // 超时双链表
        CMsgCtxPool     _mem_pool;          // 内存池
    };


}


#endif

