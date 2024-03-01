/**
 * @brief WORKER\PROXY 公共管理对象接口部分
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
     * @brief 公共进程对象定义
     */
    class CProcCtrl : public TNestDispatch
    {
    public:

        /**
         * @brief 进程对象析构函数
         */
        CProcCtrl() {};

        /**
         * @brief 进程对象析构函数
         */
        virtual ~CProcCtrl() {};

        /**
         * @brief 本地报文处理接口
         */
        virtual int32_t DoRecv(TNestBlob* blob)   ;

        /**
         * @brief 本地报文处理接口
         */
        virtual int32_t DispatchCtrl(TNestBlob* blob, NestPkgInfo& pkg_info, nest_msg_head& head);

        /**
         * @brief 发送定时心跳消息
         */
        virtual void SendHeartbeat();

        /**
         * @brief 发送定时心跳消息
         */
        virtual void SendLoadReport();

        /**
         * @brief 初始化监听对象
         */
        int32_t InitListen();

        /**
         * @brief 本地报文发送给agent
         */
        int32_t SendToAgent(void* data, uint32_t len);
        
        /**
         * @brief 本地报文发送给agent
         */
        int32_t SendRspMsg(TNestBlob* blob, void* data, uint32_t len);
        int32_t SendRspPbMsg(TNestBlob* blob, string& head, string& body); 

        /**
         * @brief 设置获取包名称
         */
        void SetPackageName(const string& pkg_name) {
            _package_name = pkg_name;
        };

        string& GetPackageName() {
            return _package_name;
        };

        /**
         * @brief 设置获取业务名称
         */
        void SetServiceName(const string& service_name) {
            _service_name = service_name;
        };
        string& GetServiceName() {
            return _service_name;
        };

        /**
         * @brief 设置获取进程基本信息
         */
        void SetProcConf(nest_proc_base& conf) {
            _base_conf = conf;
        };
        nest_proc_base& GetProcConf() {
            return _base_conf;
        };

    protected:

        TNestUdpListen      _listen;            // 本地的监听对象
        
        string              _service_name;      // 服务名
        string              _package_name;      // 包名, 路径名
        nest_proc_base      _base_conf;         // 基本的配置与运行信息
    };



}

#endif


