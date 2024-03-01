/**
 * @brief  ͨ��ͳ����Ϣ�ӿ�
 */


#ifndef _NEST_STAT_H__
#define _NEST_STAT_H__

#include "stdint.h"
#include <time.h>


namespace nest
{
    /**
     * @brief ��Ϣʱ��ͳ�Ƶļ򵥽ӿ���
     */
    class CMsgStat
    {
    public:

        ///< ��������������
        CMsgStat(){
            this->Reset(time(NULL));
        };    
        ~CMsgStat(){};

        ///< ��ȡ��ʼͳ�Ƶ�ʱ�� ��
        uint64_t GetStatTime() {
            return _lst_time;
        };

        ///< ��ȡ�ۼƵ�ʱ�� ����
        uint64_t GetCostSum() {
            return _total_cost;
        };

        ///< ��ȡ�ۼƵ�������
        uint64_t GetTotalReq() {
            return _total_req;
        };
        
        ///< ��ȡ�ۼƵ�Ӧ����
        uint64_t GetTotalRsp() {
            return _total_rsp;
        };

        ///< ��ȡ�ۼ��ڼ�����ʱ��
        uint32_t GetMaxCost() {
            return _max_cost;
        };
        
        ///< ��ȡ�ۼ��ڼ����Сʱ��
        uint32_t GetMinCost() {
            return _min_cost;
        };

        ///< ����ͳ�Ƶ�����, ����ͳ��ֵ
        void Reset(uint64_t time) {
            _lst_time   = time;
            _total_req  = 0;
            _total_rsp  = 0;
            _total_cost = 0;
            _max_cost   = 0;
            _min_cost   = 0;
        };

        ///< ����ͳ�Ƶ�������
        void ReportReq() {
            _total_req++;
        };

        ///< ����ͳ�Ƶ�Ӧ����, �봦��ʱ��
        void ReportRsp(uint32_t cost) {
            _total_rsp++;
            _total_cost += cost;
            
            if (cost > _max_cost) {
                _max_cost = cost;
            }
            
            if (!_min_cost || cost < _min_cost) {
                _min_cost = cost;
            }
        };

    protected:

        uint64_t        _lst_time;              // ���һ�βɼ�ʱ�� �뵥λ
        uint64_t        _total_req;             // �ܵ�������
        uint64_t        _total_rsp;             // �ܵ�Ӧ����
        uint64_t        _total_cost;            // �ܵ�ʱ����
        uint32_t        _max_cost;              // ����ʱ����
        uint32_t        _min_cost;              // ��С��ʱ����

    };

}

#endif

