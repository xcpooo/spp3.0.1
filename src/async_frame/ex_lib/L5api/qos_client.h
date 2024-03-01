#ifndef _QOS_AGENT_H_
#define _QOS_AGENT_H_

/**
 * Copyright (c) 1998-2014 Tencent Inc. All Rights Reserved
 * 
 * This software is L5 API for c++,
 * If you need php/python/java version, it can be found here: http://cl5.oa.com
 *
 * @file qos_client.h
 * @brief L5 API interface 
 */

/**
 * If you have L5 api,L5Agent and L5 OSS problems, all docs and details are found here:
 * http://cl5.oa.com
 * and you could contact with @h_l5_helper.
 */

/**
 * @history
 *  2014.04.30 version 5.1.0
 *     1. 增加 distribution consistent hash 
 *
 *  2014.04.02 version 5.0.0
 *     1. 发布异步 L5API
 *
 */
#include <netdb.h> 
#include <sys/socket.h> 
#include <stdio.h> 
#include <errno.h> 
#include <sys/time.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>

//using namespace std; // not use namespace for strick mix program

namespace L5API
{

/**
 * L5API return code
 * @brief return code of l5api function
 * @enum _QOS_RTN  >=0 for success, <0 for errors
 */
enum _QOS_RTN
{
    QOS_RTN_OK	   		  = 0,  // success
    QOS_RTN_ACCEPT        = 1,	// success (forward compatiblility)
    QOS_RTN_LROUTE        = 2,  // success (forward compatiblility)
    QOS_RTN_TROUTE        = 3,  // success (forward compatiblility)
    QOS_RTN_STATIC_ROUTE  = 4,  // success (forward compatiblility)
    QOS_RTN_INITED        = 5,  // success (forward compatiblility)

    QOS_RTN_OVERLOAD	  = -10000,	 // sid overload, all ip:port of sid(modid,cmdid) is not available
    QOS_RTN_TIMEOUT       = -9999,	 // timeout
    QOS_RTN_SYSERR        = -9998,   // error
    QOS_RTN_SENDERR       = -9997,   // send error
    QOS_RTN_RECVERR       = -9996,   // recv error
    QOS_MSG_INCOMPLETE    = -9995,   // msg bad format (forward compatiblility)
    QOS_CMD_ERROR         = -9994,   // cmd invalid (forward compatiblility)
    QOS_MSG_CMD_ERROR     = -9993,   // msg cmd invalid (forward compatiblility)
    QOS_INIT_CALLERID_ERROR = -9992, // init callerid error
    QOS_RTN_PARAM_ERROR   = -9991,   // parameter error
    QOS_RTN_CACHE_EXPIRE  = -9990,   // cache expire
    QOS_RTN_NOEXIST_SID   = -8999    // not exist sid(modid,cmdid)
};

/**
 * @brief hash type for struct QOSREQUEST
 * @note Release Version: 5.1.0 
 */
enum e_hash_type
{
    HASH_T_RANDOM = 0, // random as default
    HASH_T_CONSISTENT_HASH_KETAMA = 1 // consistent hash ketama
};

struct QOSREQUESTTMEXTtag;
//单次访问的基本信息
typedef struct QOSREQUESTtag
{
    int			    _flow;      //flow id, reserved by L5API(do not modify)

    e_hash_type     _hash_type; // HASH_T_RANDOM as default
    std::string     _hash_key;  // input hash key when _hash_type>0, eg. session id or uin 

    int			    _modid;		//被调模块编码
    int			    _cmd;		//被调接口编码
    std::string		_host_ip;	//被调主机IP
    unsigned short  _host_port;	//被调主机PORT
    int             _pre;       //pre value, reserved by L5API(do not modify)

    int             _caller_tid;//caller thread id, reserverd by L5API(do not modify)
    struct timeval  _caller_tm; //caller invoke time, reserved by L5API(do not modify)

    QOSREQUESTtag(const QOSREQUESTTMEXTtag &route);
    QOSREQUESTtag():_flow(0),_modid(0),_cmd(0),_host_port(0),_pre(0),_caller_tid(0),_hash_type(HASH_T_RANDOM){}
}QOSREQUEST;

//新增带时延信息的接口
struct QOSREQUESTTMEXTtag
{
    int			_flow;
    int			_modid;		//被调模块编码
    int			_cmd;		//被调接口编码
    std::string		_host_ip;	//被调主机IP
    unsigned short _host_port; //被调主机PORT
    int _pre;        
    int _delay;        

    QOSREQUESTTMEXTtag( ):_flow(0),_modid(0),_cmd(0),_host_port(0),_pre(0),_delay(0){}
    QOSREQUESTTMEXTtag( const QOSREQUEST & route);
};
typedef struct QOSREQUESTTMEXTtag QOSREQUEST_TMEXT;

typedef struct QOSREQUESTMTTCEXTtag
{
    int32_t			_modid;		//modid
    int32_t			_cmdid;     //cmdid
    int64_t			_key;		//hash key
    int32_t			_funid;     //funcation id
    std::string		_host_ip;	//host ip return by L5API
    unsigned short  _host_port;	//host port return by L5API
}QOSREQUEST_MTTCEXT;

// hash有状态路由
typedef struct QOSREQUEST_MTTCEXT_HASHtag
{
    int			    _flow;      //flow id
    int			    _modid;		//被调模块编码
    int			    _cmd;		//被调接口编码
    int64_t         _key;       //有状态路由 key
    int             _funid;     //有状态路由 funid, 请设置为0
    std::string		_host_ip;	//被调主机IP
    unsigned short  _host_port;	//被调主机PORT
    int             _pre;       //pre value

    int             _caller_tid;//caller thread id
    struct timeval  _caller_tm; //caller time

    QOSREQUEST_MTTCEXT_HASHtag():_flow(0),_modid(0),_cmd(0),_key(0),_funid(0),_host_port(0),_pre(0),_caller_tid(0){}
}QOSREQUEST_MTTCEXT_HASH;

/**
 * @brief Each api caller thread must call this function once to initialize
 *        route table for sid(modid,cmdid) to prepare faster cache.
 * @note Release Version: 5.0.0 
 * Args:
 *   @param modid:      modid needed to be inited
 *   @param cmdid:      cmdid needed to be inited
 *   @param err_msg:    error messange when return value<0
 *   @param hash_type:  e_hash_type, HASH_T_RANDOM as default
 *
 * @return success or fail, please record the err_msg.
 *   @retval 0 for success
 *   @retval <0 for errors,while err_msg will be filled, caller should record the err_msg.
 */

int InitThreadData(int modid, int cmdid, std::string &err_msg, e_hash_type hash_type = HASH_T_RANDOM);

/**
 * @brief Asynchronous get route which return IP:PORT in qos_req._host_ip and qos_req._host_port.
 * @note Release Version: 5.0.0 
 * Args:
 *   @param qos_req:   request route of sid(qos_req._modid,qos_req._cmd),
 *                     return IP:PORT in qos_req._host_ip and qos_req._host_port
 *   @param err_msg:   error messange when return value<0
 *
 * @return success or fail, please record the err_msg.
 *         return IP:PORT in qos_req._host_ip and qos_req._host_port when success
 *   @retval 0 for success
 *   @retval <0 for errors,while err_msg will be filled, caller should record the err_msg.
 */
int AsyncApiGetRoute(QOSREQUEST& qos_req, std::string& err_msg);

/**
 * @brief state route with hash
 * @note Release Version: 5.1.0 
 */
int AsyncApiGetRoute(QOSREQUEST_MTTCEXT_HASH& qos_req, std::string& err_msg);

/**
 * @brief Asynchronous report result and delay of qos_req._host_ip which return by ApiGetRoute.
 * @note delay = @usetime
 * @note Release Version: 5.0.0 
 * Args:
 *   @param qos_req:	  qos_req(include last _modid,_cmd,_host_ip,_host_port) which return by ApiGetRoute
 *   @param ret:		  report status to L5_agent,0 for normal,<0 for abnormal
 *   @param usetime_type: time unit(enum QOS_TIME_TYPE), microseconds or milliseconds or seconds
 *   @param usetime:      used time
 *   @param err_msg:	  error messange when return value<0
 * @return success or fail, please record the err_msg.
 *   @retval 0 for success
 *   @retval <0 for errors,while err_msg will be filled, caller should record the err_msg
 */

enum QOS_TIME_TYPE
{
    TIME_MICROSECOND = 0,
    TIME_MILLISECOND = 1,
    TIME_SECOND      = 2
};
void AsyncApiRouteResultUpdate(QOSREQUEST& qos_req, int ret, int usetime_type, int usetime);

/**
 * @brief Asynchronous report result and delay of qos_req._host_ip which return by ApiGetRoute
 * @note delay interval is between AsyncApiGetRoute and AsyncApiRouteResultUpdate
 * @note Release Version: 5.0.0 
 * Args:
 *   @param qos_req:	  qos_req(include last _modid,_cmd,_host_ip,_host_port) which return by ApiGetRoute
 *   @param ret:		  report status to L5_agent,0 for normal,<0 for abnormal
 */
void AsyncApiRouteResultUpdate(QOSREQUEST& qos_req, int ret);

/**
 * @brief state route with hash
 * @note Release Version: 5.1.0 
 */
void AsyncApiRouteResultUpdate(QOSREQUEST_MTTCEXT_HASH& qos_req, int ret, int usetime_type, int usetime);
/**
 * @brief state route with hash
 * @note Release Version: 5.1.0 
 */
void AsyncApiRouteResultUpdate(QOSREQUEST_MTTCEXT_HASH& qos_req,int ret);

/**
 * @brief Each api caller thread may call this function to uninitialize
 *        route table for sid(modid,cmdid) to release sid resource, when you do not use this sid any more.
 * @note Release Version: 5.0.0 
 * Args:
 *   @param modid:      modid needed to be inited
 *   @param cmdid:      cmdid needed to be inited
 *
 * @return success or fail.
 *   @retval 0 for success
 *   @retval <0 for errors
 */
int UnInitThreadData(int modid, int cmdid);

int ApiAntiParallelGetRoute(QOSREQUEST_MTTCEXT& qos_req,  std::string& err_msg, struct timeval* tm_val=NULL);
int ApiAntiParallelUpdate(QOSREQUEST_MTTCEXT& qos_req, int ret, int usetime_usec, std::string& err_msg, struct timeval* tm_val=NULL);

}

#endif
