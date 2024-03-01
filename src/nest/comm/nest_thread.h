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
     * @brief �߳�״̬ö������
     */
    enum NestThreadState
    {
        NEST_THREAD_INIT    = 0,
        NEST_THREAD_RUNNING = 1,
        NEST_THREAD_RUNOVER = 2,
    };

    /**
     * @breif �߳��ඨ��
     */
    class CNestThread
    {
    public:

        /**
         * @brief ��������������
         */
        CNestThread() {
            _tid    = 0;
            _state  = NEST_THREAD_INIT;
        };
        virtual ~CNestThread(){};
        
        /**
         * @brief ִ�к�����
         */
        virtual void Run() {};

        /**
         * @brief Ĭ�������������̶߳���, ע�ⲻҪ�ظ��ͷ�
         */
        virtual void Clean() {
            delete this;
        };
        

        /**
         * @brief ��������
         */
        int32_t Start() {
            return pthread_create(&_tid, NULL, RegistRun, this);
        };

        /**
         * @brief �Ƿ�ִ�����
         */
        bool RunFinish() {
            return (_state == NEST_THREAD_RUNOVER);
        }; 

        /**
         * @brief ��ȡtid
         */
        pthread_t GetTid() {
            return _tid;
        };

    private:

        /**
         * @brief ��̬ע�᷽��
         */
        static void* RegistRun(void* args)  
        {
            CNestThread* thread = (CNestThread*)args;
            thread->RealRun();
            return thread;
        };

        /**
         * @brief �ڲ�ִ�з���
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

        pthread_t           _tid;               // �߳�ID
        NestThreadState     _state;             // �˳�״̬, ��Դ�����
    };


    

    
};



#endif

