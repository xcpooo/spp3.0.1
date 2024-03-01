/**
 * @brief  通道统计信息接口
 */


#ifndef _NEST_STAT_H__
#define _NEST_STAT_H__

#include "stdint.h"
#include <time.h>


namespace nest
{
    /**
     * @brief 消息时延统计的简单接口类
     */
    class CMsgStat
    {
    public:

        ///< 构造与析构函数
        CMsgStat(){
            this->Reset(time(NULL));
        };    
        ~CMsgStat(){};

        ///< 获取开始统计的时间 秒
        uint64_t GetStatTime() {
            return _lst_time;
        };

        ///< 获取累计的时延 毫秒
        uint64_t GetCostSum() {
            return _total_cost;
        };

        ///< 获取累计的请求数
        uint64_t GetTotalReq() {
            return _total_req;
        };
        
        ///< 获取累计的应答数
        uint64_t GetTotalRsp() {
            return _total_rsp;
        };

        ///< 获取累计期间的最大时延
        uint32_t GetMaxCost() {
            return _max_cost;
        };
        
        ///< 获取累计期间的最小时延
        uint32_t GetMinCost() {
            return _min_cost;
        };

        ///< 更新统计的周期, 重置统计值
        void Reset(uint64_t time) {
            _lst_time   = time;
            _total_req  = 0;
            _total_rsp  = 0;
            _total_cost = 0;
            _max_cost   = 0;
            _min_cost   = 0;
        };

        ///< 更新统计的请求数
        void ReportReq() {
            _total_req++;
        };

        ///< 更新统计的应答数, 与处理时延
        void ReportRsp(uint32_t cost) {
            _total_rsp++;
            _total_cost += cost;
            
            if (cost > _max_cost) {
                _max_cost = cost;
            }
            
            if (!_min_cost || cost < _min_cost) {
                _min_cost = cost;
            }
        };

    protected:

        uint64_t        _lst_time;              // 最近一次采集时间 秒单位
        uint64_t        _total_req;             // 总的请求量
        uint64_t        _total_rsp;             // 总的应答量
        uint64_t        _total_cost;            // 总的时延量
        uint32_t        _max_cost;              // 最大的时延量
        uint32_t        _min_cost;              // 最小的时延量

    };

}

#endif

