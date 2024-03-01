/*
 * =====================================================================================
 *
 *       Filename:  interface.h
 *
 *    Description:  the async interface
 *
 *        Version:  1.0
 *        Created:  06/17/2009 09:41:16 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  allanhuang (for fun), allanhuang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __INTERFACE_H_
#define __INTERFACE_H_
extern "C" int ttc_recv(int event, int sessionId, void* comm_param, char* buf, int len);
extern "C" int tdb_recv(int event, int sessionId, void* comm_param, char* buf, int len);
extern "C" int tcp_recv(int event, int sessionId, void* comm_param, char* buf, int len);
extern "C" int udp_recv(int event, int sessionId, void* comm_param, char* buf, int len);

extern "C" int ttc_input(void* input_param, unsigned sessionId , void* blob, void* server);
//extern "C" int tdb_input(void* input_param, unsigned sessionId , void* blob, void* server);
extern "C" int tcp_input(void* input_param, unsigned sessionId , void* blob, void* server);
extern "C" int udp_input(void* input_param, unsigned sessionId , void* blob, void* server);
#endif
