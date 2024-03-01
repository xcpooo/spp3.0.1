/**
 *  @file mt_action.cpp
 *  @info ΢�߳�ACTION����ʵ��
 *  @time 20130924
 **/

#include "micro_thread.h"
#include "mt_notify.h"
#include "mt_connection.h"
#include "mt_session.h"
#include "mt_action.h"

using namespace std;
using namespace NS_MICRO_THREAD;


/**
 * @brief ��ʼitem״̬
 */
void IMtAction::Init()
{
    _flag       = MULTI_FLAG_UNDEF;
    _proto      = MT_UDP;
    _conn_type  = CONN_TYPE_SHORT;
    _errno      = ERR_NONE;
    _time_cost  = 0;
    _buff_size  = 0;
    _msg        = NULL;
    _conn       = NULL;
    _ntfy_name  = 0;
    memset(&_addr, 0, sizeof(_addr));
}

/**
 * @brief ������, ����item״̬
 */
void IMtAction::Reset()
{
    // ������, ����ɹ��Ÿ���, ����ǿ�ƹر�
    bool force_free = false;
    if (_errno != ERR_NONE) {
        force_free = true;
    }

    if (_conn) {
        ConnectionMgr::Instance()->FreeConnection(_conn, force_free);
        _conn = NULL;
    }
}

/**
 * @brief ��ȡ���Ӷ���, ֪ͨ����, ��Ϣ����
 */
EpollerObj* IMtAction::GetNtfyObj() {
    IMtConnection* conn = GetIConnection();
    if (conn) {
        return conn->GetNtfyObj();
    } else {
        return NULL;
    }
};


/**
 * @brief ������ӻ�����ʼ��
 */
int IMtAction::InitConnEnv()
{
    MtFrame* mtframe = MtFrame::Instance();
    ConnectionMgr* connmgr = ConnectionMgr::Instance();
    MsgBuffPool* msgmgr = MsgBuffPool::Instance();
    NtfyObjMgr* ntfymgr = NtfyObjMgr::Instance();
    SessionMgr* sessionmgr = SessionMgr::Instance();

    if (_conn != NULL) {
        MTLOG_ERROR("Action init failed, maybe action reused in actionlist, check it!!");
        return -100;
    }

    // 1. �����ȡconn���
    CONN_OBJ_TYPE conn_obj_type = OBJ_CONN_UNDEF;
    NTFY_OBJ_TYPE ntfy_obj_type = NTFY_OBJ_UNDEF;
    
    MULTI_PROTO proto = this->GetProtoType();
    MULTI_CONNECT type = this->GetConnType();
    if ((MT_UDP == proto) && (CONN_TYPE_SESSION == type))   // UDP sessionģʽ
    {
        conn_obj_type = OBJ_UDP_SESSION;
        ntfy_obj_type = NTFY_OBJ_SESSION;
    }
    else if (MT_UDP == proto)  // UDP ����ģʽ
    {   
        conn_obj_type = OBJ_SHORT_CONN;
        ntfy_obj_type = NTFY_OBJ_THREAD;
    }
    else    // TCP ģʽ
    {
        conn_obj_type = OBJ_TCP_KEEP;
        ntfy_obj_type = NTFY_OBJ_THREAD;
    }

    _conn = connmgr->GetConnection(conn_obj_type, this->GetMsgDstAddr());
    if (!_conn) {
        MTLOG_ERROR("Get conn failed, type: %d", conn_obj_type);
        return -1;
    }
    _conn->SetIMtActon(this);

    // 2. ��ȡmsg buff���
    int max_len = this->GetMsgBuffSize();
    MtMsgBuf* msg_buff = msgmgr->GetMsgBuf(max_len);
    if (!msg_buff) {
        MTLOG_ERROR("Maybe no memory, buffsize: %d, get failed", max_len);
        return -2;
    }
    msg_buff->SetBuffType(BUFF_SEND);
    _conn->SetMtMsgBuff(msg_buff);

    // 3. ��ȡ ntfy ������
    EpollerObj* ntfy_obj = ntfymgr->GetNtfyObj(ntfy_obj_type, _ntfy_name);
    if (!ntfy_obj) {
        MTLOG_ERROR("Maybe no memory, ntfy type: %d, get failed", ntfy_obj_type);
        return -3;
    }
    _conn->SetNtfyObj(ntfy_obj);

    // 4. SESSIONģ��, ����session
    MicroThread* thread = mtframe->GetActiveThread();
    ntfy_obj->SetOwnerThread(thread);
    this->SetIMsgPtr((IMtMsg*)thread->GetThreadArgs());
    if (conn_obj_type == OBJ_UDP_SESSION)
    {
        this->SetOwnerThread(thread);
        this->SetSessionConn(_conn);
        this->SetSessionId(sessionmgr->GetSessionId());
        sessionmgr->InsertSession(this);
    }
    
    return 0;
}


/**
 * @brief �����麯��, �򻯽ӿ���ʵ�ֲ���
 */
int IMtAction::DoEncode()
{
    MtMsgBuf* msg_buff = NULL;
    if (_conn) {
        msg_buff = _conn->GetMtMsgBuff();
    }
    if (!_conn || !msg_buff) {
        MTLOG_ERROR("conn(%p) or msgbuff(%p) null", _conn, msg_buff);
        return -100;
    }

    int msg_len = msg_buff->GetMaxLen();
    int ret = this->HandleEncode(msg_buff->GetMsgBuff(), msg_len, _msg);
    if (ret < 0)
    {
        MTLOG_DEBUG("handleecode failed, ret %d", ret);
        return ret;
    }
    msg_buff->SetMsgLen(msg_len);

    return 0;
}

/**
 * @brief �����麯��, �򻯽ӿ���ʵ�ֲ���
 */
int IMtAction::DoInput()
{
    MtMsgBuf* msg_buff = NULL;
    if (_conn) {
        msg_buff = _conn->GetMtMsgBuff();
    }
    if (!_conn || !msg_buff) {
        MTLOG_ERROR("conn(%p) or msgbuff(%p) null", _conn, msg_buff);
        return -100;
    }

    int msg_len = msg_buff->GetHaveRcvLen();
    int ret = this->HandleInput(msg_buff->GetMsgBuff(), msg_len, _msg);
    if (ret < 0)
    {
        MTLOG_DEBUG("HandleInput failed, ret %d", ret);
        return ret;
    }

    return ret;
}


int IMtAction::DoProcess()
{
    MtMsgBuf* msg_buff = NULL;
    if (_conn) {
        msg_buff = _conn->GetMtMsgBuff();
    }
    if (!_conn || !msg_buff) {
        MTLOG_ERROR("conn(%p) or msgbuff(%p) null", _conn, msg_buff);
        return -100;
    }

    int ret = this->HandleProcess(msg_buff->GetMsgBuff(), msg_buff->GetMsgLen(), _msg);
    if (ret < 0)
    {
        MTLOG_DEBUG("handleprocess failed, ret %d", ret);
        return ret;
    }

    return 0;

}

int IMtAction::DoError()
{
    return this->HandleError((int)_errno, _msg);
}



/**
 * @brief ��������������
 */
IMtAction::IMtAction()
{
    Init();    
}
IMtAction::~IMtAction()
{
    Reset();
    Init();
}

