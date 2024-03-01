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
     * @brief ��ʱ���ص����崦��
     */
    typedef int32_t (*TNestTimerFunc)(void* args);
    typedef int32_t  TNestTimerID;

    /**
     * @brief ѭ����ʱ��������
     */
    class TLoopTimer : public CTimerObject
    {
    public:
        
        /**
         * @brief ��������������
         */
        TLoopTimer();
        virtual ~TLoopTimer();

        virtual void TimerNotify(void);

        /**
         * @brief ���ö�ʱ��, Ĭ������״̬
         */
        int32_t SetTimer(TNestTimerFunc func, uint32_t time_wait, void* args);

        /**
         * @brief ��ͣ��ʱ���ӿ�
         */
        int32_t StartTimer();
        void StopTimer();

    private:
        
        CTimerList*       _timer_list;  // �����timerlist 
        TNestTimerFunc    _func;        // �ص�����
        void*             _args;        // ����
        uint32_t          _time_wait;   // ��ʱʱ��, ms
        bool              _run_flag;    // �Ƿ�����
    };
    typedef std::map<TNestTimerID, TLoopTimer*>  CNestTimerMap;     // map����ṹ����

    
    /** 
     * @brief TIMER��ȫ�ֹ���ṹ
     */
    class  TNestTimerMng
    {
    public:

        /** 
         * @brief ��������
         */
        ~TNestTimerMng();

        /** 
         * @brief �����������ʽӿ�
         */
        static TNestTimerMng* Instance();

        /** 
         * @brief ȫ�ֵ����ٽӿ�
         */
        static void Destroy();

        /** 
         * @brief �ڴ����epoll������ʽӿ�
         */
        CTimerUnit* GetTimerUnit() {
            return _timer_unit;
        };

        /**
         * @brief ����ѭ����ʱ��, Ĭ�ϴ���������
         */
        TNestTimerID CreateTimer(TNestTimerFunc func, uint32_t time_wait, void* args);  
        void DeleteTimer(TNestTimerID tid);
        TLoopTimer* FindTimer(TNestTimerID tid);

        /**
         * @brief ����ֹͣ��ʱ��
         */
        int32_t StartTimer(TNestTimerID tid);
        void StopTimer(TNestTimerID tid);

        /**
         * @brief ���ж�ʱ��
         */
        void RunTimer(uint64_t now);        
        
    private:

        /** 
         * @brief ���캯��
         */
        TNestTimerMng();
    
        static TNestTimerMng *_instance;         // �������� 
        
        CTimerUnit*           _timer_unit;       // timer�������
        CNestTimerMap         _timer_map;        // ����
        TNestTimerID          _curr_tid;         // ���Ӽ�¼
    };

}

#endif

