/**
 * @filename nest_timer.cpp
 * @info 封装的loop定时器功能
 */

#include "nest_log.h"       // for log
#include "nest_timer.h"

using namespace nest;

/**
 * @brief 构造与析构函数
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
 * @brief 设置启动定时器, 默认直接运行
 * @param func      -回调函数
 * @param time_wait -超时时间, 默认ms
 * @param args      -业务回调自定义参数
 * @return <0 失败 0 成功
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
 * @brief 设置启动定时器
 * @return <0 失败 0 成功
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
 * @brief 设置停止定时器
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
 * @brief 超时通知回调函数, 回调可以删除对象
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
 * @brief 定时器全局管理句柄
 * @return 全局句柄指针
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
 * @brief 定时器管理全局的销毁接口
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
 * @brief 定时任务管理
 */
TNestTimerMng::TNestTimerMng()
{
    _timer_unit  = new CTimerUnit;
    _curr_tid    = 0;
}

/**
 * @brief 析构函数, 清理托管的定时器节点
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
 * @brief 查询定时器对象
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
 * @brief 设置启动定时器, 默认直接运行
 * @param func      -回调函数
 * @param time_wait -超时时间, 默认ms
 * @param args      -业务回调自定义参数
 * @return <0 失败 >0 成功返回timerid
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
 * @brief 停止定时器, 删除内部节点
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
 * @brief 启动定时器
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
 * @brief 暂停定时器
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
 * @brief 定时驱动定时任务执行
 */
void TNestTimerMng::RunTimer(uint64_t now)
{
    _timer_unit->CheckExpired(now);
}

