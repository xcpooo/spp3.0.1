/**
 * @brief nest thread 
 */

#ifndef _NEST_THREAD_H
#define _NEST_THREAD_H

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

namespace nest
{
    /**
     * @brief 线程状态枚举类型
     */
    enum NestThreadState
    {
        NEST_THREAD_INIT    = 0,
        NEST_THREAD_RUNNING = 1,
        NEST_THREAD_RUNOVER = 2,
    };

    /**
     * @breif 线程类定义
     */
    class CNestThread
    {
    public:

        /**
         * @brief 构造与析构函数
         */
        CNestThread() {
            _tid    = 0;
            _state  = NEST_THREAD_INIT;
        };
        virtual ~CNestThread(){};
        
        /**
         * @brief 执行函数体
         */
        virtual void Run() {};

        /**
         * @brief 默认清理是析构线程对象, 注意不要重复释放
         */
        virtual void Clean() {
            delete this;
        };
        

        /**
         * @brief 启动函数
         */
        int32_t Start() {
            return pthread_create(&_tid, NULL, RegistRun, this);
        };

        /**
         * @brief 是否执行完毕
         */
        bool RunFinish() {
            return (_state == NEST_THREAD_RUNOVER);
        }; 

        /**
         * @brief 获取tid
         */
        pthread_t GetTid() {
            return _tid;
        };

    private:

        /**
         * @brief 静态注册方法
         */
        static void* RegistRun(void* args)  
        {
            CNestThread* thread = (CNestThread*)args;
            thread->RealRun();
            return thread;
        };

        /**
         * @brief 内部执行方法
         */
        void RealRun()
        {
            _tid = pthread_self();
            _state = NEST_THREAD_RUNNING;        
            this->Run();
            _state = NEST_THREAD_RUNOVER;
            pthread_detach(_tid);
            _tid = 0;
            this->Clean();
            pthread_exit(NULL);
        };
        
    protected:

        pthread_t           _tid;               // 线程ID
        NestThreadState     _state;             // 退出状态, 资源清理等
    };


    

    
};



#endif

