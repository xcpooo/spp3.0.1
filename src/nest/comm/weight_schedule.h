/**
 * @brief 动态权重调度算法实现
 */

#ifndef _WEIGHT_SCHEDULE__
#define _WEIGHT_SCHEDULE__


#include  "stdint.h"
#include  <algorithm>
#include  <vector>

using std::vector;

namespace nest
{

    // 动态权重可调度对象接口定义
    class IDispatchAble
    {
    public:

        // 构造与析构函数
        IDispatchAble() {
            _weight     = 0;
            _max_weight = 0;
        };
        virtual ~IDispatchAble() {};

        // 更新权重, 初始化为max权重值
        void set_weight(uint32_t weight) {
            _weight     = weight;
        };

        // 获取当前的权重值
        uint32_t get_weight() {
            return _weight;
        };

        // 设置校正过的权重, 以标准权重为基准, 按校正策略, 累计变更, 有默认边界限制
        void set_max_weight(uint32_t weight) {
            _max_weight = weight;
        };
        
        // 获取目前的校正权重值
        uint32_t get_max_weight() {
            return _max_weight;
        };

    protected:

        uint32_t    _weight;               // 当前权重值, 根据应答情况, 动态调整
        uint32_t    _max_weight;           // 校正权重, 实际权重不得超过该值
    };
    typedef vector<IDispatchAble*>   CSchedList;
    

    // 调度算法对象定义
    class CWeightSchedule
    {
    public:
        
        // 构造与析构函数
        CWeightSchedule();
        virtual ~CWeightSchedule();

        // 增加调度节点
        void add_item(IDispatchAble* item);

        // 删除调度节点
        void del_item(IDispatchAble* item);

        // 调度节点
        IDispatchAble* get_next_item();

        // 节点动态权重恢复
        void inc_weight(IDispatchAble* item);

        // 节点动态权重调整
        void dec_weight(IDispatchAble* item);

        // 节点默认权重修正
        void fix_max_weight(IDispatchAble* item){};

        // 重设置调节的参数信息
        void set_step_params(uint32_t dec);

        // 获取待激活的节点
        IDispatchAble* find_detect_item();

        // 独立随机函数, 免冲突
        void srand(uint32_t seed);
        uint32_t random();

    protected:
    
        CSchedList  _item_list;            // 调度对象列表
        
        uint32_t    _max_quota;            // 最大权值
        uint32_t    _min_quota;            // 最小权值
        uint32_t    _inc_step;             // 增加时的幅度
        uint32_t    _dec_step;             // 权值减少时的幅度
        uint64_t    _total_times;          // 分配的次数，每隔一定时间使用一次权值最小的节点
        uint32_t    _wakeup_frq;           // 激活频率 每次经过一定次数启用权值最小节点
        uint32_t    _rand_seed;            // 私有随机实现的种子
    };

    /**
     * @brief 动态控制权重参数
     */
    class CDynamicCtrl
    {
    public:

        // 窗口常量定义
        enum 
        {
            MAX_QUOTA       = 10000,
            MAX_WINDOWS     = 20,
            MIN_WINDOWS     = 1,
            DFLT_WINDOWS    = 10,
        };

        // 构造与析构函数
        CDynamicCtrl();
        virtual ~CDynamicCtrl(){};

        // 请求上报记录
        void request_report();

        // 记录时延信息
        void response_report(uint32_t delay);

        // 周期性的重计算调整
        void reset_report(uint64_t time_ms);

        // 获取统计的开始时间
        uint64_t report_start_time() {
            return _report_time;
        };
        
        // 计算调节参数
        uint32_t calc_step(uint64_t time_now, uint32_t count);
        
    protected:

        uint32_t        _max_quota;     // 默认的最大变更权重
        uint64_t        _report_time;   // 本周期开始的毫秒
        uint32_t        _req_count;     // 本周期的请求量
        uint32_t        _rsp_count;     // 本周起时延统计次数
        uint64_t        _cost_total;    // 本周起汇总时延
    };


}

#endif

