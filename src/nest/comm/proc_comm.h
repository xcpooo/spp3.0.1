/**
 * @brief WORKER\PROXY �����������ӿڲ���
 */

#ifndef _NEST_PROC_COMM_H__
#define _NEST_PROC_COMM_H__

#include "stdint.h"
#include <time.h>
#include "nest_proto.h"
#include "nest_channel.h"

namespace nest
{

    /**
     * @brief �������̶�����
     */
    class CProcCtrl : public TNestDispatch
    {
    public:

        /**
         * @brief ���̶�����������
         */
        CProcCtrl() {};

        /**
         * @brief ���̶�����������
         */
        virtual ~CProcCtrl() {};

        /**
         * @brief ���ر��Ĵ���ӿ�
         */
        virtual int32_t DoRecv(TNestBlob* blob)   ;

        /**
         * @brief ���ر��Ĵ���ӿ�
         */
        virtual int32_t DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        /**
         * @brief ���Ͷ�ʱ������Ϣ
         */
        virtual void SendHeartbeat();

        /**
         * @brief ���Ͷ�ʱ������Ϣ
         */
        virtual void SendLoadReport();

        /**
         * @brief ��ʼ����������
         */
        int32_t InitListen();

        /**
         * @brief ���ر��ķ��͸�agent
         */
        int32_t SendToAgent(void* data, uint32_t len);
        
        /**
         * @brief ���ر��ķ��͸�agent
         */
        int32_t SendRspMsg(TNestBlob* blob, void* data, uint32_t len);
        int32_t SendRspPbMsg(TNestBlob* blob, string& head, string& body); 

        /**
         * @brief ���û�ȡ������
         */
        void SetPackageName(const string& pkg_name) {
            _package_name = pkg_name;
        };

        string& GetPackageName() {
            return _package_name;
        };

        /**
         * @brief ���û�ȡҵ������
         */
        void SetServiceName(const string& service_name) {
            _service_name = service_name;
        };
        string& GetServiceName() {
            return _service_name;
        };

        /**
         * @brief ���û�ȡ���̻�����Ϣ
         */
        void SetProcConf(nest_proc_base& conf) {
            _base_conf = conf;
        };
        nest_proc_base& GetProcConf() {
            return _base_conf;
        };

    protected:

        TNestUdpListen      _listen;            // ���صļ�������
        
        string              _service_name;      // ������
        string              _package_name;      // ����, ·����
        nest_proc_base      _base_conf;         // ������������������Ϣ
    };



}

#endif


