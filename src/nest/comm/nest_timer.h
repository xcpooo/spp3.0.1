/**
 * @brief Nest timer mng 
 */
 
#include <stdint.h>
#include <map>
#include "timerlist.h"


#ifndef __NEST_TIMER_H_
#define __NEST_TIMER_H_

namespace nest
{

    /**
     * @brief 定时器回调定义处理
     */
    typedef int32_t (*TNestTimerFunc)(void* args);
    typedef int32_t  TNestTimerID;

    /**
     * @brief 循环定时器对象定义
     */
    class TLoopTimer : public CTimerObject
    {
    public:
        
        /**
         * @brief 构造与析构函数
         */
        TLoopTimer();
        virtual ~TLoopTimer();

        virtual void TimerNotify(void);

        /**
         * @brief 设置定时器, 默认运行状态
         */
        int32_t SetTimer(TNestTimerFunc func, uint32_t time_wait, void* args);

        /**
         * @brief 启停定时器接口
         */
        int32_t StartTimer();
        void StopTimer();

    private:
        
        CTimerList*       _timer_list;  // 分配的timerlist 
        TNestTimerFunc    _func;        // 回调函数
        void*             _args;        // 参数
        uint32_t          _time_wait;   // 超时时间, ms
        bool              _run_flag;    // 是否运行
    };
    typedef std::map<TNestTimerID, TLoopTimer*>  CNestTimerMap;     // map管理结构定义

    
    /** 
     * @brief TIMER的全局管理结构
     */
    class  TNestTimerMng
    {
    public:

        /** 
         * @brief 析构函数
         */
        ~TNestTimerMng();

        /** 
         * @brief 单例类句柄访问接口
         */
        static TNestTimerMng* Instance();

        /** 
         * @brief 全局的销毁接口
         */
        static void Destroy();

        /** 
         * @brief 内存池与epoll管理访问接口
         */
        CTimerUnit* GetTimerUnit() {
            return _timer_unit;
        };

        /**
         * @brief 创建循环定时器, 默认创建即运行
         */
        TNestTimerID CreateTimer(TNestTimerFunc func, uint32_t time_wait, void* args);  
        void DeleteTimer(TNestTimerID tid);
        TLoopTimer* FindTimer(TNestTimerID tid);

        /**
         * @brief 启动停止定时器
         */
        int32_t StartTimer(TNestTimerID tid);
        void StopTimer(TNestTimerID tid);

        /**
         * @brief 运行定时器
         */
        void RunTimer(uint64_t now);        
        
    private:

        /** 
         * @brief 构造函数
         */
        TNestTimerMng();
    
        static TNestTimerMng *_instance;         // 单例类句柄 
        
        CTimerUnit*           _timer_unit;       // timer管理对象
        CNestTimerMap         _timer_map;        // 管理
        TNestTimerID          _curr_tid;         // 种子记录
    };

}

#endif

