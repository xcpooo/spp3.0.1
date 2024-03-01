/**
 *  @file mt_session.h
 *  @info ΢�̵߳��¼��Ự������, ÿ��������ӹ���һsession��Ϣ
 *  @time 20130924
 **/

#ifndef __MT_SESSION_H__
#define __MT_SESSION_H__

#include "hash_list.h"

namespace NS_MICRO_THREAD {

class MicroThread;
class IMtConnection;

enum SESSION_FLAG
{
    SESSION_IDLE    = 0,    ///< SESSION δ����hash����
    SESSION_INUSE   = 1,    ///< SESSION �������״̬
};

/**
 * @brief ��������session�ӿڶ���, ����session��ӳ���thread,action��
 */
class ISession : public HashKey
{
public:

    /**
     * @brief ��������������
     */
    ISession() : _session_id(0), _session_flg(0), _thread(NULL), _connection(NULL) {};
    virtual ~ISession();

public:

    /**
     * @brief �ỰID���������ȡ
     */
    void SetSessionId(int id) {
        _session_id = id;    
    };
    int GetSessionId() {
        return _session_id;
    };

    /**
     * @brief �����̵߳��������ȡ
     */
    MicroThread* GetOwnerThread(){
        return _thread;
    };
    void SetOwnerThread(MicroThread* thread) {
        _thread = thread;
    };

    /**
     * @brief �������ӵ��������ȡ
     */
    IMtConnection* GetSessionConn(){
        return _connection;
    };
    void SetSessionConn(IMtConnection* conn) {
        _connection = conn;
    };

    /**
     * @brief �Ựflag���������ȡ
     */
    void SetSessionFlag(int flag) {
        _session_flg = flag;    
    };
    int GetSessionFlag() {
        return _session_flg;
    };

    /**
     *  @brief �ڵ�Ԫ�ص�hash�㷨, ��ȡkey��hashֵ
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual uint32_t HashValue(){
        return _session_id;
    }; 

    /**
     *  @brief �ڵ�Ԫ�ص�cmp����, ͬһͰID��, ��key�Ƚ�
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual int HashCmp(HashKey* rhs){
        return this->_session_id - (int)rhs->HashValue();
    };         

protected:

    int  _session_id;               // �Ựid��Ϣ
    int  _session_flg;              // ��¼session״̬ 0 -����hash��, 1 -hash������
    MicroThread* _thread;           // �Ự������session  
    IMtConnection* _connection;     // �Ự����������
};

/**
 * @brief ȫ�ֵ�session����ṹ
 */
class SessionMgr
{
public:

    /**
     * @brief �Ự�����ĵ�ȫ�ֹ������ӿ�
     * @return ȫ�־��ָ��
     */
    static SessionMgr* Instance (void);

    /**
     * @brief ȫ�ֵ�ɾ���ӿ�
     */
    static void Destroy();

    /**
     * @brief ��ȡsessionid
     * @return ȫ�־��ָ��
     */
    int GetSessionId(void) {
        _curr_session++;
        if (!_curr_session) {
            _curr_session++;
        }
        return _curr_session;
    };

    /**
     * @brief Session���ݴ洢
     */
    int InsertSession(ISession* session);

    /**
     * @brief ��ѯsession����
     */
    ISession* FindSession(int session_id);

    /**
     * @brief ɾ��session����
     */
    void RemoveSession(int session_id);

    /**
     * @brief ��������
     */
    ~SessionMgr();
    
private:

    /**
     * @brief ��Ϣbuff�Ĺ��캯��
     */
    SessionMgr();

    static SessionMgr * _instance;          ///<  ��������
    int       _curr_session;                ///<  session����
    HashList* _hash_map;                    ///<  ��sessionid hash�洢
};

}

#endif


