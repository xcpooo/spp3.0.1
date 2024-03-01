/**
 * @brief ��̬Ȩ�ص����㷨ʵ��
 */

#ifndef _WEIGHT_SCHEDULE__
#define _WEIGHT_SCHEDULE__


#include  "stdint.h"
#include  <algorithm>
#include  <vector>

using std::vector;

namespace nest
{

    // ��̬Ȩ�ؿɵ��ȶ���ӿڶ���
    class IDispatchAble
    {
    public:

        // ��������������
        IDispatchAble() {
            _weight     = 0;
            _max_weight = 0;
        };
        virtual ~IDispatchAble() {};

        // ����Ȩ��, ��ʼ��ΪmaxȨ��ֵ
        void set_weight(uint32_t weight) {
            _weight     = weight;
        };

        // ��ȡ��ǰ��Ȩ��ֵ
        uint32_t get_weight() {
            return _weight;
        };

        // ����У������Ȩ��, �Ա�׼Ȩ��Ϊ��׼, ��У������, �ۼƱ��, ��Ĭ�ϱ߽�����
        void set_max_weight(uint32_t weight) {
            _max_weight = weight;
        };
        
        // ��ȡĿǰ��У��Ȩ��ֵ
        uint32_t get_max_weight() {
            return _max_weight;
        };

    protected:

        uint32_t    _weight;               // ��ǰȨ��ֵ, ����Ӧ�����, ��̬����
        uint32_t    _max_weight;           // У��Ȩ��, ʵ��Ȩ�ز��ó�����ֵ
    };
    typedef vector<IDispatchAble*>   CSchedList;
    

    // �����㷨������
    class CWeightSchedule
    {
    public:
        
        // ��������������
        CWeightSchedule();
        virtual ~CWeightSchedule();

        // ���ӵ��Ƚڵ�
        void add_item(IDispatchAble* item);

        // ɾ�����Ƚڵ�
        void del_item(IDispatchAble* item);

        // ���Ƚڵ�
        IDispatchAble* get_next_item();

        // �ڵ㶯̬Ȩ�ػָ�
        void inc_weight(IDispatchAble* item);

        // �ڵ㶯̬Ȩ�ص���
        void dec_weight(IDispatchAble* item);

        // �ڵ�Ĭ��Ȩ������
        void fix_max_weight(IDispatchAble* item){};

        // �����õ��ڵĲ�����Ϣ
        void set_step_params(uint32_t dec);

        // ��ȡ������Ľڵ�
        IDispatchAble* find_detect_item();

        // �����������, ���ͻ
        void srand(uint32_t seed);
        uint32_t random();

    protected:
    
        CSchedList  _item_list;            // ���ȶ����б�
        
        uint32_t    _max_quota;            // ���Ȩֵ
        uint32_t    _min_quota;            // ��СȨֵ
        uint32_t    _inc_step;             // ����ʱ�ķ���
        uint32_t    _dec_step;             // Ȩֵ����ʱ�ķ���
        uint64_t    _total_times;          // ����Ĵ�����ÿ��һ��ʱ��ʹ��һ��Ȩֵ��С�Ľڵ�
        uint32_t    _wakeup_frq;           // ����Ƶ�� ÿ�ξ���һ����������Ȩֵ��С�ڵ�
        uint32_t    _rand_seed;            // ˽�����ʵ�ֵ�����
    };

    /**
     * @brief ��̬����Ȩ�ز���
     */
    class CDynamicCtrl
    {
    public:

        // ���ڳ�������
        enum 
        {
            MAX_QUOTA       = 10000,
            MAX_WINDOWS     = 20,
            MIN_WINDOWS     = 1,
            DFLT_WINDOWS    = 10,
        };

        // ��������������
        CDynamicCtrl();
        virtual ~CDynamicCtrl(){};

        // �����ϱ���¼
        void request_report();

        // ��¼ʱ����Ϣ
        void response_report(uint32_t delay);

        // �����Ե��ؼ������
        void reset_report(uint64_t time_ms);

        // ��ȡͳ�ƵĿ�ʼʱ��
        uint64_t report_start_time() {
            return _report_time;
        };
        
        // ������ڲ���
        uint32_t calc_step(uint64_t time_now, uint32_t count);
        
    protected:

        uint32_t        _max_quota;     // Ĭ�ϵ������Ȩ��
        uint64_t        _report_time;   // �����ڿ�ʼ�ĺ���
        uint32_t        _req_count;     // �����ڵ�������
        uint32_t        _rsp_count;     // ������ʱ��ͳ�ƴ���
        uint64_t        _cost_total;    // ���������ʱ��
    };


}

#endif

