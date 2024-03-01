/**
 * @filename nest_timer.cpp
 * @info ��װ��loop��ʱ������
 */

#include "nest_log.h"       // for log
#include "nest_timer.h"

using namespace nest;

/**
 * @brief ��������������
 */
TLoopTimer::TLoopTimer()
{
    _timer_list = NULL;
    _func       = NULL;
    _args       = NULL;
    _time_wait  = 0;
    _run_flag   = false;
}
TLoopTimer::~TLoopTimer()
{
    if (_run_flag) {
        DisableTimer();
        _run_flag = false;
    }   
}

/**
 * @brief ����������ʱ��, Ĭ��ֱ������
 * @param func      -�ص�����
 * @param time_wait -��ʱʱ��, Ĭ��ms
 * @param args      -ҵ��ص��Զ������
 * @return <0 ʧ�� 0 �ɹ�
 */
int32_t TLoopTimer::SetTimer(TNestTimerFunc func, uint32_t time_wait, void* args)
{
    _func       = func;
    _time_wait  = time_wait;
    _args       = args;

    CTimerUnit* timer_unit = TNestTimerMng::Instance()->GetTimerUnit();
    if (timer_unit) {
        _timer_list = timer_unit->GetTimerList((int)time_wait);
    }
    
    if (!timer_unit || !_timer_list)
    {
        NEST_LOG(LOG_ERROR, "create timer failed, unit: %p, list: %p", timer_unit, _timer_list);
        return -1;
    }
    
    AttachTimer(_timer_list);
    _run_flag = true;
    
    return 0;
}


/**
 * @brief ����������ʱ��
 * @return <0 ʧ�� 0 �ɹ�
 */
int32_t TLoopTimer::StartTimer()
{
    if (_run_flag) {
        return 0;
    }

    if (!_timer_list)
    {
        return -1;
    }
    
    AttachTimer(_timer_list);
    _run_flag = true;
    
    return 0;
}

/**
 * @brief ����ֹͣ��ʱ��
 */
void TLoopTimer::StopTimer()
{
    if (!_run_flag) {
        return;
    }
    
    DisableTimer();
    _run_flag = false;
    return;
}

/**
 * @brief ��ʱ֪ͨ�ص�����, �ص�����ɾ������
 */
void TLoopTimer::TimerNotify()
{
    if (_run_flag) {
        AttachTimer(_timer_list);
    }
    
    if (_func) {
        _func(_args);
    }
}


/**
 * @brief ��ʱ��ȫ�ֹ�����
 * @return ȫ�־��ָ��
 */
TNestTimerMng* TNestTimerMng::_instance = NULL;
TNestTimerMng* TNestTimerMng::Instance ()
{
    if (NULL == _instance)
    {
        _instance = new TNestTimerMng();
    }

    return _instance;
}

/**
 * @brief ��ʱ������ȫ�ֵ����ٽӿ�
 */
void TNestTimerMng::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}

/**
 * @brief ��ʱ�������
 */
TNestTimerMng::TNestTimerMng()
{
    _timer_unit  = new CTimerUnit;
    _curr_tid    = 0;
}

/**
 * @brief ��������, �����йܵĶ�ʱ���ڵ�
 */
TNestTimerMng::~TNestTimerMng()
{
    for (CNestTimerMap::iterator it = _timer_map.begin(); it != _timer_map.end(); ++it)
    {
        delete it->second;
    }

    if (_timer_unit) 
    {
        delete _timer_unit;
        _timer_unit = NULL;
    }
}

/**
 * @brief ��ѯ��ʱ������
 */
TLoopTimer* TNestTimerMng::FindTimer(TNestTimerID id)
{
    CNestTimerMap::iterator it = _timer_map.find(id);
    if (it != _timer_map.end())
    {
        return it->second;
    } 
    else 
    {
        return NULL;
    }
}

/**
 * @brief ����������ʱ��, Ĭ��ֱ������
 * @param func      -�ص�����
 * @param time_wait -��ʱʱ��, Ĭ��ms
 * @param args      -ҵ��ص��Զ������
 * @return <0 ʧ�� >0 �ɹ�����timerid
 */
TNestTimerID TNestTimerMng::CreateTimer(TNestTimerFunc func, uint32_t time_wait, void* args)
{
    TLoopTimer* timer = new TLoopTimer;
    if (!timer) {
        NEST_LOG(LOG_ERROR, "create timer failed, no memory");
        return -1;
    }

    int32_t ret = timer->SetTimer(func, time_wait, args);
    if (ret < 0)
    {
        NEST_LOG(LOG_ERROR, "set timer failed, ret %d", ret);
        delete timer;
        return -2;
    }

    while (++_curr_tid > 0)
    {
        if (this->FindTimer(_curr_tid) == NULL)
        {
            _timer_map[_curr_tid] = timer;
            return 0;
        }
    } 

    NEST_LOG(LOG_ERROR, "set timer no more id, impossible!!");
    _curr_tid = 0;    
    delete timer;
    return -3;
}

/**
 * @brief ֹͣ��ʱ��, ɾ���ڲ��ڵ�
 */
void TNestTimerMng::DeleteTimer(TNestTimerID tid)
{
    TLoopTimer* timer = this->FindTimer(tid);
    if (timer)
    {
        _timer_map.erase(tid);
        delete timer;
    }
}

/**
 * @brief ������ʱ��
 */
int32_t TNestTimerMng::StartTimer(TNestTimerID tid)
{
    TLoopTimer* timer = this->FindTimer(tid);
    if (!timer)
    {
        NEST_LOG(LOG_ERROR, "start timer failed, id %d invalid!!", tid);
        return -100;    
    }

    return timer->StartTimer();
}

/**
 * @brief ��ͣ��ʱ��
 */
void TNestTimerMng::StopTimer(TNestTimerID tid)
{
    TLoopTimer* timer = this->FindTimer(tid);
    if (timer)
    {
        timer->StopTimer();
    }
}

/**
 * @brief ��ʱ������ʱ����ִ��
 */
void TNestTimerMng::RunTimer(uint64_t now)
{
    _timer_unit->CheckExpired(now);
}

