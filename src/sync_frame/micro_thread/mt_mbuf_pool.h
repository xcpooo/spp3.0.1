/**
 *  @file mt_mbuf_pool.h
 *  @info ΢�߳�ͬ����Ϣbuf��
 **/

#ifndef __MT_MBUF_POOL_H__
#define __MT_MBUF_POOL_H__

#include <netinet/in.h>
#include <queue>
#include "hash_list.h"

namespace NS_MICRO_THREAD {

using std::queue;

enum BUFF_TYPE
{
    BUFF_UNDEF          =  0,           ///< δ��������
    BUFF_RECV           =  1,           ///< ����buff
    BUFF_SEND           =  2,           ///< ����buff
};

/**
 * @brief ��ϢͶ�ݵ�buffer��
 */
typedef TAILQ_ENTRY(MtMsgBuf) MsgBufLink;
typedef TAILQ_HEAD(__MtbuffTailq, MtMsgBuf) MsgBufQueue;
class MtMsgBuf
{
private:
    int   _max_len;         // ���Ŀռ䳤��
    int   _msg_len;         // ʵ�ʵ���Ϣ����
    int   _buf_type;        // buff�Ƿ��ͻ��ǽ���
    int   _recv_len;        // �ѽ��յ���Ϣ����
    int   _send_len;        // �ѷ��͵���Ϣ����
    void* _msg_buff;        // buffer ʵ��ͷָ��

public:

    MsgBufLink _entry;

    /**
     * @brief ���캯��, ָ�����buff����
     */
    MtMsgBuf(int max_len) {
        _max_len  = max_len;
        _msg_len  = 0;
        _buf_type = BUFF_UNDEF;
        _recv_len = 0;
        _send_len = 0;
        _msg_buff = malloc(max_len);
    };

    ~MtMsgBuf() {
        if (_msg_buff) {
            free(_msg_buff);
            _msg_buff = NULL;
        }
    };

    /**
     * @brief ��Ϣ���͵��������ȡ
     */
    void SetBuffType(BUFF_TYPE type) {
        _buf_type = (int)type;
    };
    BUFF_TYPE GetBuffType() {
        return (BUFF_TYPE)_buf_type;
    };

    /**
     * @brief ���ýӿ�, �ָ���ʼ״̬
     */
    void Reset() {
        _msg_len  = 0;
        _recv_len = 0;
        _send_len = 0;
        _buf_type = BUFF_UNDEF;
    };

    /**
     * @brief ��Ϣ���ȵ��������ȡ
     */
    void SetMsgLen(int msg_len) {
        _msg_len = msg_len;
    };
    int GetMsgLen() {
        return _msg_len;
    };

    /**
     * @brief ��󳤶���bufferָ���ȡ
     */
    int GetMaxLen() {
        return _max_len;
    };
    void* GetMsgBuff() {
        return _msg_buff;
    };

    /**
     * @brief �м�״̬��ȡ�����
     */
    int GetHaveSndLen() {
        return _send_len;
    };
    void SetHaveSndLen(int snd_len) {
        _send_len = snd_len;
    };

    /**
     * @brief �м�״̬��ȡ�����
     */
    int GetHaveRcvLen() {
        return _recv_len;
    };
    void SetHaveRcvLen(int rcv_len) {
        _recv_len = rcv_len;
    };
};

/**
 * @brief ָ����С��buffer, ����󳤶�ӳ��ɿ��ж���
 */
class MsgBufMap : public HashKey
{
public:

    /**
     *  @brief ��Ϣbuff����Ĺ���
     *  @param buff_size ��mapԪ���ϵ�����buff, �����buff�ռ��Сֵ
     *  @param max_free �ö��й���Ԫ��, ��󱣳ֵ�free��Ŀ
     */
    MsgBufMap(int buff_size, int max_free) {
        _max_buf_size = buff_size;
        _max_free     = max_free;
        this->SetDataPtr(this);
        _queue_num    = 0;
        TAILQ_INIT(&_msg_queue);
    };

    /**
     *  @brief ��Ϣbuff����Ĺ���, �򵥹���, ������key��Ϣ
     *  @param buff_size ��mapԪ���ϵ�����buff, �����buff�ռ��Сֵ
     */
    explicit MsgBufMap(int buff_size) {
        _max_buf_size = buff_size;
        TAILQ_INIT(&_msg_queue);
    };

    /**
     *  @brief ��Ϣbuff�������������
     */
    ~MsgBufMap() {
        MtMsgBuf* ptr = NULL;
        MtMsgBuf* tmp = NULL;
        TAILQ_FOREACH_SAFE(ptr, &_msg_queue, _entry, tmp)
        {
            TAILQ_REMOVE(&_msg_queue, ptr, _entry);
            delete ptr;
            _queue_num--;
        }
        
        TAILQ_INIT(&_msg_queue);
    };
    
    /**
     *  @brief ��ȡ��ϢbuffԪ��
     *  @return msgbufָ��, ʧ��ΪNULL
     */
    MtMsgBuf* GetMsgBuf(){
        MtMsgBuf* ptr = NULL;        
        if (!TAILQ_EMPTY(&_msg_queue)) {
            ptr = TAILQ_FIRST(&_msg_queue);
            TAILQ_REMOVE(&_msg_queue, ptr, _entry);
            _queue_num--;
        } else {
            ptr = new MtMsgBuf(_max_buf_size);
        }
        
        return ptr;
    };

    /**
     *  @brief �ͷ���ϢbuffԪ��
     *  @param msgbufָ��
     */
    void FreeMsgBuf(MtMsgBuf* ptr){
        if (_queue_num >= _max_free) {
            delete ptr;
        } else {
            ptr->Reset();
            TAILQ_INSERT_TAIL(&_msg_queue, ptr, _entry);
            _queue_num++;
        }
    };

    /**
     *  @brief �ڵ�Ԫ�ص�hash�㷨, ��ȡkey��hashֵ
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual uint32_t HashValue(){
        return _max_buf_size;
    }; 

    /**
     *  @brief �ڵ�Ԫ�ص�cmp����, ͬһͰID��, ��key�Ƚ�
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual int HashCmp(HashKey* rhs){
        return this->_max_buf_size - (int)rhs->HashValue();
    }; 

private:
    int _max_free;              ///< �����б�������
    int _max_buf_size;          ///< ����������buffsize
    int _queue_num;             ///< ���ж��и���
    MsgBufQueue _msg_queue;     ///< ʵ�ʵĿ��ж���
};


/**
 * @brief ȫ�ֵ�buffer�ض���, ͳһ���������buffer
 */
class MsgBuffPool
{
public:

    /**
     * @brief ��Ϣbuff��ȫ�ֹ������ӿ�
     * @return ȫ�־��ָ��
     */
    static MsgBuffPool* Instance (void);

    /**
     * @brief ��Ϣ����ӿ�
     */
    static void Destroy(void);

    /**
     * @brief ��Ϣbuff��ȫ�ֹ�������Ĭ�����Ŀ��и���
     * @param max_free �����б�����Ŀ, ��Ҫ�ڷ���Ԫ��ǰ����
     */
    void SetMaxFreeNum(int max_free) {
        _max_free = max_free;
    };

    /**
     *  @brief ��ȡ��ϢbuffԪ��
     *  @return msgbufָ��, ʧ��ΪNULL
     */
    MtMsgBuf* GetMsgBuf(int max_size);

    /**
     *  @brief �ͷ���ϢbuffԪ��
     *  @param msgbufָ��
     */
    void FreeMsgBuf(MtMsgBuf* msg_buf);

    /**
     * @brief ��Ϣbuff��ȫ������������
     */
    ~MsgBuffPool();

private:

    /**
     * @brief ��Ϣbuff�Ĺ��캯��
     */
    explicit MsgBuffPool(int max_free = 300);

    static MsgBuffPool * _instance;         ///<  ��������    
    int  _max_free;                         ///<  �����������Ŀ
    HashList* _hash_map;                    ///<  ��size hashmap ������ж���

};



}

#endif


