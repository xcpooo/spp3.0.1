/**
 *  @file mt_session.cpp
 *  @info ΢�̺߳�����ӻỰ����ʵ�ֲ���
 *  @time 20130924
 **/

#include "micro_thread.h"
#include "mt_session.h"

using namespace std;
using namespace NS_MICRO_THREAD;

/**
 * @brief session�ӿ���Դ�Ի��մ���
 */
ISession::~ISession()
{
    if (_session_flg) {
        SessionMgr* sessionmgr = SessionMgr::Instance();
        sessionmgr->RemoveSession(_session_id);
        _session_flg = (int)SESSION_IDLE;   // ���⴦��, ��remove�����ڴ����Ӵ���
    }
}


/**
 * @brief sessionȫ�ֹ�����
 * @return ȫ�־��ָ��
 */
SessionMgr* SessionMgr::_instance = NULL;
SessionMgr* SessionMgr::Instance (void)
{
    if (NULL == _instance)
    {
        _instance = new SessionMgr;
    }

    return _instance;
}

/**
 * @brief session����ȫ�ֵ����ٽӿ�
 */
void SessionMgr::Destroy()
{
    if( _instance != NULL )
    {
        delete _instance;
        _instance = NULL;
    }
}

/**
 * @brief ��Ϣbuff�Ĺ��캯��
 */
SessionMgr::SessionMgr()
{
    _curr_session = 0;
    _hash_map = new HashList(100000);
}

/**
 * @brief ��������, ��������Դ, ������������
 */
SessionMgr::~SessionMgr()
{
    if (_hash_map) {
        delete _hash_map;
        _hash_map = NULL;
    }
}

/**
 * @brief Session���ݴ洢
 */
int SessionMgr::InsertSession(ISession* session)
{
    if (!_hash_map || !session) {
        MTLOG_ERROR("Mngr not init(%p), or session null(%p)", _hash_map, session);
        return -100;
    }

    int flag = session->GetSessionFlag();
    if (flag & SESSION_INUSE) {
        MTLOG_ERROR("Session already in hash, bugs, %p, %d", session, flag);
        return -200;
    }    
    
    session->SetSessionFlag((int)SESSION_INUSE);
    return _hash_map->HashInsert(session);
}

/**
 * @brief ��ѯsession����
 */
ISession* SessionMgr::FindSession(int session_id)
{
    if (!_hash_map) {
        MTLOG_ERROR("Mngr not init(%p)", _hash_map);
        return NULL;
    }

    ISession key;
    key.SetSessionId(session_id);    
    return dynamic_cast<ISession*>(_hash_map->HashFind(&key));
}

/**
 * @brief ɾ��session����
 */
void SessionMgr::RemoveSession(int session_id)
{
    if (!_hash_map) {
        MTLOG_ERROR("Mngr not init(%p)", _hash_map);
        return;
    }

    ISession key;
    key.SetSessionId(session_id);    
    return _hash_map->HashRemove(&key);
}


