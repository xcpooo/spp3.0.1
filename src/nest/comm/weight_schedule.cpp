/**
 * @brief ��̬Ȩ�ص����㷨
 */

#include "weight_schedule.h"

using namespace nest;

/**
 * @brief Ԥ��10������Ӧ��, Ȩ�ؽ��͵��ȵ�
 */
CWeightSchedule::CWeightSchedule()
{
    _max_quota      = 10000;        // Ĭ��Ȩ�� 10000, ��������
    _min_quota      = 1;            // ��СȨ�� 1, ��֤���нڵ㲻����ʱ, �ɵ���
    _inc_step       = 1000;         // ����ֵ ��֤10����Ч��Ӧ��ɻָ��ڵ�
    _dec_step       = 960;          // ����ֵ ��֤����10�β��ɴ�, �ڵ㲻����
    _total_times    = 0;            // �ۼ�������, ����̽����
    _wakeup_frq     = 10000;        // ���Ի��ѵ�Ƶ����, ��΢��
    _rand_seed      = 1;            // Ĭ�ϵ��������
}

/**
 * @brief ������������
 */
CWeightSchedule::~CWeightSchedule()
{
    _item_list.clear();
}

/**
 * @brief �ڵ�������
 */
void CWeightSchedule::add_item(IDispatchAble* item)
{
    // ��ʼ��Ĭ�ϵ�Ȩ��ֵ
    item->set_max_weight(_max_quota);
    item->set_weight(_max_quota);

    // ���ظ�����, ���ڵ��Ƿ����
    CSchedList::iterator it = std::find(_item_list.begin(), _item_list.end(), item);
    if (it != _item_list.end())  {
        return;
    }
    
    _item_list.push_back(item);
}

/**
 * @brief �ڵ����ɾ��
 */
void CWeightSchedule::del_item(IDispatchAble* item)
{
    CSchedList::iterator it = std::find(_item_list.begin(), _item_list.end(), item);
    if (it != _item_list.end())  {
        _item_list.erase(it);
    }
}

/**
 * @brief �ڵ����, ��ȡ�����Ľڵ�
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

        // Ȩ����ͬ�Ķ���ڵ�, �������, ��֤���������ļ���, ���л���
        if ((item->get_weight() == min->get_weight()) && (random() % 2))
        {
            min = item;
            continue;
        }
    }

    return min;
}


/**
 * @brief �����㷨, ��ȡ��ǰ�ĵ��Ƚڵ�
 */
IDispatchAble* CWeightSchedule::get_next_item()
{
    IDispatchAble* item = NULL;
    _total_times++;
    
    // 1. ���ڸ������߼�
    if ((_total_times % _wakeup_frq) == (_wakeup_frq - 1))
    {
        return find_detect_item();
    }

    // 2. Ȩ�ػ�������, ��̬Ȩ��, ÿ��Ҫ�ؼ���
    uint32_t total_quota = 0;
    CSchedList::iterator it;
    for (it = _item_list.begin(); it != _item_list.end(); ++it)
    {   
        item = *it;
        total_quota += item->get_weight();
    }    

    // 3. ��Ȩ�������ȡ�ڵ�
    uint32_t weight = 0;
    uint32_t rand = random() % ((total_quota > 0) ? total_quota : 1);
    for (it = _item_list.begin(); it != _item_list.end(); ++it)
    {
        item = *it;
        weight = item->get_weight();
        if (rand < weight) // ����������� �ᵼ�¼������, ���ڵ㲻��ƽ
        {
            break;
        }
        rand -= weight; 
    } 

    return item;
}

/**
 * @brief �����㷨, �ָ���̬Ȩ��
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
 * @brief �����㷨, ������̬Ȩ��
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
 * @brief �������õ��ڲ���, ע����Ķ�, �ӵĿ�
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
 * @brief �����������
 */
void CWeightSchedule::srand(uint32_t seed)
{
    _rand_seed = seed;    
}

/**
 * @brief �������
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
 * @brief ��̬����Ȩ�ز�������
 */
CDynamicCtrl::CDynamicCtrl()
{
    _report_time    = 0;
    _req_count      = 0;
    _rsp_count      = 0; 
    _cost_total     = 0;
    _max_quota      = MAX_QUOTA;    // Ԥ��ֵ, ��CWeightScheduleһ��
}

/**
 * @brief ������Ϣͳ��
 */
void CDynamicCtrl::request_report()
{
    _req_count++;
}


/**
 * @brief ��¼ʱ����Ϣ
 */
void CDynamicCtrl::response_report(uint32_t delay)
{
    _rsp_count++;
    _cost_total += delay;
}

/**
 * @brief ������ڲ���, Ĭ�ϰ�10�μ���, ��� 3-15��
 */
uint32_t CDynamicCtrl::calc_step(uint64_t time_now, uint32_t count)
{
    uint32_t dflt_dec  = MAX_QUOTA / MIN_WINDOWS;
    uint32_t real_wins = DFLT_WINDOWS;

    // 1. ��һ�����ڻ��������һ����, ֱ��ʹ��Ĭ��ֵ
    if ((_report_time == 0) || (_rsp_count == 0))
    {
        return dflt_dec;
    }

    // 2. ͳ�������ڵĴ�����������, ��Чʱ����, ����Ĭ��ֵ
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

    // 3. ���ڼ����߼�, ������ԽС, ����ԽС
    real_wins = MIN_WINDOWS * _req_count / max_capacity;
    if (real_wins < MIN_WINDOWS)
    {
        real_wins = MIN_WINDOWS;
    }
    else if (real_wins > MAX_WINDOWS)
    {
        real_wins = MAX_WINDOWS;
    }

    // 4. ���ض�̬�����Ĳ���
    return (MAX_QUOTA / real_wins);

}

/**
 * @brief �����Ե��ؼ������
 */
void CDynamicCtrl::reset_report(uint64_t time_ms)
{
    _report_time    = time_ms;
    _req_count      = 0;
    _rsp_count      = 0; 
    _cost_total     = 0;
}



