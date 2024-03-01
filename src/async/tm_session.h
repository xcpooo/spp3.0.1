/*
 * =====================================================================================
 *
 *       Filename:  interface.h
 *
 *    Description:  The Declaration of  TmSession ,which aims to implement periodic task
 *
 *        Version:  1.0
 *        Created:  10/28/2009 09:41:16 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lifengzeng (for fun), allanhuang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TM_SESSION_H__
#define	__TM_SESSION_H__

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "spp_async_def.h"
#include "noncopyable.h"
#include "timerlist.h"
#include "asyn_api.h"
#include "interface.h"
#include "../comm/singleton.h"
#include "spp_async_interface.h"

namespace  ASYNC_SPP_II
{

    class  TmSession: public CTimerObject
    {
    public:
        TmSession(const int sid, const int cb_interval, void* cb_param, time_task_func cb_func):
                _id(sid), _cb_interval(cb_interval), _cb_param(cb_param), _cb_func(cb_func), _status(SESSION_IDLE) {
        }

        virtual void TimerNotify(void);

        virtual ~TmSession() {
        }

        void	SetStatus(SESSION_STATUS s) {
            _status = s;
        }

        SESSION_STATUS GetStatus() {
            return _status;
        }

    protected:
        int 	   _id;
        int	   _cb_interval;
        void*	   _cb_param;
        time_task_func _cb_func;
        SESSION_STATUS _status;
        friend class CSessionMgr;
    };

}
#endif
