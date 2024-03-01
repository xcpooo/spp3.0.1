/**
 * @brief Nest agent net manager 
 */
 
#include <stdint.h>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"


#ifndef __NEST_AGENT_NET_MNG_H_
#define __NEST_AGENT_NET_MNG_H_

using std::map;
using std::vector;

namespace nest
{
  
    /** 
     * @brief ����Զ�̽�����������ȫ�ֹ���
     */
    class  TAgentNetMng : public TNestDispatch
    {
    public:

        ///< ��������ʺ���
        static TAgentNetMng* Instance();

        ///< ȫ�ֵ����ٽӿ�
        static void Destroy();
        
        ///< ����unix domain socket path
        static void RmUnixSockPath(uint32_t pid);

        ///< ��� unix domain socket path
        static bool CheckUnixSockPath(uint32_t pid, bool abstract = false);

        ///< �ж�Ŀ¼�Ƿ����
        static bool IsDirExist(const char* path);

        ///< �ж��ļ��Ƿ����
        static bool IsFileExist(const char* path);

        ///< ����������
        virtual ~TAgentNetMng();

        ///< ��ʼ���÷��������Ϣ
        int32_t InitService(uint32_t setid, TNestAddrArray& servers);

        ///< ���·����set��Ϣ, ��������
        int32_t UpdateSetInfo(uint32_t setid, TNestAddrArray& servers);

        ///< ��ȡ���ؼ�����ip��Ϣ
        char* GetListenIp() {
            static char dflt[] = "127.0.0.1";
            if (_tcp_listen) {
                return _tcp_listen->GetListenAddr().IPString(NULL, 0);
            } else {
                return dflt;
            }
        };

        ///< �����������ӹ�����Ϣ
        TNestChannel* InitKeepChannel();  

        ///< ��ȡ�������ӹ�����Ϣ
        TNestChannel* GetKeepChannel() {
            return _keep_conn;
        };

        ///< ��ѯ��̬������Ϣ
        TNestChannel* FindChannel(uint32_t fd);  

        ///< ���Ӽ�⴦����
        int32_t CheckKeepAlive(); 

        ///< ����ѡ��server�ӿ�
        void ChangeServerAddr();

        ///< ���ͱ��ĵ�����agent
        int32_t SendtoAgent(const void* data, uint32_t len);

        ///< ���ͱ��ĵ�����
        int32_t SendtoLocal(TNestAddress* addr, const void* data, uint32_t len);

        ///< ���Ͳ����ձ���
        int32_t SendRecvLocal(TNestAddress* addr, const void* data, uint32_t len, 
                                  void* buff, uint32_t& buff_len, uint32_t time_wait);

        /**
         * @brief �ַ�������
         */
        virtual int32_t DoRecv(TNestBlob* channel);
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr);
        virtual int32_t DoError(TNestChannel* channel);

        /**
         * @brief �������ݰ�������server
         */
        int32_t SendToServer(void* buff, uint32_t len);
        
        /**
         * @brief ִ����Ϣ�շ�, ������������
         */
        void RunService();

    private:

        // �������캯��
        TAgentNetMng();
        
        static TAgentNetMng*        _instance;       // ��������
        
        TNestChannel*               _keep_conn;      // ���ֳ�����
        TNestListen*                _tcp_listen;     // Զ��TCP socket����
        TNestUdpListen*             _unix_listen;    // ���س�unix ���� socket���� 
        TNestChannelMap             _channel_map;    // ���ӹ���map

        uint32_t                    _setid;          // setid
        TNestAddrArray              _server_addrs;   // server��ַ
        uint32_t                    _retry_count;    // ���Դ���
        uint32_t                    _server_index;   // server ��λ��
        TNestAddress                _server;         // ��ǰserver
    };


}

#endif

