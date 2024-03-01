/**
 * @brief 动态权重调度算法
 */

#include "weight_schedule.h"

using namespace nest;

/**
 * @brief 预计10个包不应答, 权重降低到谷底
 */
CWeightSchedule::CWeightSchedule()
{
    _max_quota      = 10000;        // 默认权重 10000, 经验设置
    _min_quota      = 1;            // 最小权重 1, 保证所有节点不可用时, 可调度
    _inc_step       = 1000;         // 经验值 保证10次有效的应答可恢复节点
    _dec_step       = 960;          // 经验值 保证超过10次不可达, 节点不可用
    _total_times    = 0;            // 累计请求数, 触发探测用
    _wakeup_frq     = 10000;        // 尝试唤醒的频率数, 可微调
    _rand_seed      = 1;            // 默认的随机种子
}

/**
 * @brief 析构函数处理
 */
CWeightSchedule::~CWeightSchedule()
{
    _item_list.clear();
}

/**
 * @brief 节点管理添加
 */
void CWeightSchedule::add_item(IDispatchAble* item)
{
    // 初始化默认的权重值
    item->set_max_weight(_max_quota);
    item->set_weight(_max_quota);

    // 防重复插入, 检查节点是否存在
    CSchedList::iterator it = std::find(_item_list.begin(), _item_list.end(), item);
    if (it != _item_list.end())  {
        return;
    }
    
    _item_list.push_back(item);
}

/**
 * @brief 节点管理删除
 */
void CWeightSchedule::del_item(IDispatchAble* item)
{
    CSchedList::iterator it = std::find(_item_list.begin(), _item_list.end(), item);
    if (it != _item_list.end())  {
        _item_list.erase(it);
    }
}

/**
 * @brief 节点管理, 获取待检测的节点
 */
IDispatchAble* CWeightSchedule::find_detect_item()
{
    IDispatchAble* min = NULL;
    CSchedList::iterator it;
    for (it = _item_list.begin(); it != _item_list.end(); ++it)
    {
        IDispatchAble* item = *it;
        if (!min || item->get_weight() < min->get_weight())
        {
            min = item;
            continue;
        }

        // 权重相同的多个节点, 随机处理, 保证多个待激活的几点, 均有机会
        if ((item->get_weight() == min->get_weight()) && (random() % 2))
        {
            min = item;
            continue;
        }
    }

    return min;
}


/**
 * @brief 调度算法, 获取当前的调度节点
 */
IDispatchAble* CWeightSchedule::get_next_item()
{
    IDispatchAble* item = NULL;
    _total_times++;
    
    // 1. 定期复活检测逻辑
    if ((_total_times % _wakeup_frq) == (_wakeup_frq - 1))
    {
        return find_detect_item();
    }

    // 2. 权重汇总重算, 动态权重, 每次要重计算
    uint32_t total_quota = 0;
    CSchedList::iterator it;
    for (it = _item_list.begin(); it != _item_list.end(); ++it)
    {   
        item = *it;
        total_quota += item->get_weight();
    }    

    // 3. 按权重随机获取节点
    uint32_t weight = 0;
    uint32_t rand = random() % ((total_quota > 0) ? total_quota : 1);
    for (it = _item_list.begin(); it != _item_list.end(); ++it)
    {
        item = *it;
        weight = item->get_weight();
        if (rand < weight) // 如果包含等于 会导致极限情况, 最后节点不公平
        {
            break;
        }
        rand -= weight; 
    } 

    return item;
}

/**
 * @brief 调度算法, 恢复动态权重
 */
void CWeightSchedule::inc_weight(IDispatchAble* item)
{
    uint32_t weight     = item->get_weight();
    uint32_t max_weight = item->get_max_weight();
    if ((weight + _inc_step) > max_weight)
    {
        weight = max_weight;
    }
    else
    {
        weight += _inc_step;
    }
    
    item->set_weight(weight);
}

/**
 * @brief 调度算法, 调整动态权重
 */
void CWeightSchedule::dec_weight(IDispatchAble* item)
{
    uint32_t weight = item->get_weight();
    if (weight < (_dec_step + _min_quota))
    {
        weight = _min_quota;
    }
    else
    {
        weight -= _dec_step;
    }
    
    item->set_weight(weight);
}

/**
 * @brief 重新设置调节参数, 注意减的多, 加的快
 */
void CWeightSchedule::set_step_params(uint32_t dec)
{
    if (dec > 0)
    {
        _dec_step   = dec;
        _inc_step   = dec + dec/20;
    }
}

/**
 * @brief 随机种子设置
 */
void CWeightSchedule::srand(uint32_t seed)
{
    _rand_seed = seed;    
}

/**
 * @brief 随机函数
 */
uint32_t CWeightSchedule::random()
{
    uint32_t next   = _rand_seed;
    uint32_t result = 0; 

    next *= 1103515245;
    next += 12345;
    result = (uint32_t) (next / 65536) % 2048; 

    next *= 1103515245;
    next += 12345;
    result <<= 10; 
    result ^= (uint32_t) (next / 65536) % 1024; 

    next *= 1103515245;
    next += 12345;
    result <<= 10; 
    result ^= (uint32_t) (next / 65536) % 1024; 

    _rand_seed = next; 

    return result; 
}


/**
 * @brief 动态控制权重参数构造
 */
CDynamicCtrl::CDynamicCtrl()
{
    _report_time    = 0;
    _req_count      = 0;
    _rsp_count      = 0; 
    _cost_total     = 0;
    _max_quota      = MAX_QUOTA;    // 预设值, 与CWeightSchedule一致
}

/**
 * @brief 请求信息统计
 */
void CDynamicCtrl::request_report()
{
    _req_count++;
}


/**
 * @brief 记录时延信息
 */
void CDynamicCtrl::response_report(uint32_t delay)
{
    _rsp_count++;
    _cost_total += delay;
}

/**
 * @brief 计算调节参数, 默认按10次计算, 跨度 3-15次
 */
uint32_t CDynamicCtrl::calc_step(uint64_t time_now, uint32_t count)
{
    uint32_t dflt_dec  = MAX_QUOTA / MIN_WINDOWS;
    uint32_t real_wins = DFLT_WINDOWS;

    // 1. 第一个周期或无请求的一周期, 直接使用默认值
    if ((_report_time == 0) || (_rsp_count == 0))
    {
        return dflt_dec;
    }

    // 2. 统计周期内的处理能力计算, 无效时间间隔, 启用默认值
    uint32_t avg_cost = _cost_total / _rsp_count;
    if ((avg_cost == 0) || (time_now < _report_time))
    {
        return dflt_dec;
    }
    
    uint32_t max_capacity = count * (time_now - _report_time) / avg_cost;
    if (max_capacity == 0)
    {
        return dflt_dec;
    }

    // 3. 窗口计算逻辑, 请求数越小, 窗口越小
    real_wins = MIN_WINDOWS * _req_count / max_capacity;
    if (real_wins < MIN_WINDOWS)
    {
        real_wins = MIN_WINDOWS;
    }
    else if (real_wins > MAX_WINDOWS)
    {
        real_wins = MAX_WINDOWS;
    }

    // 4. 返回动态调整的步伐
    return (MAX_QUOTA / real_wins);

}

/**
 * @brief 周期性的重计算调整
 */
void CDynamicCtrl::reset_report(uint64_t time_ms)
{
    _report_time    = time_ms;
    _req_count      = 0;
    _rsp_count      = 0; 
    _cost_total     = 0;
}



