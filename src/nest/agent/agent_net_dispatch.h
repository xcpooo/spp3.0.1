/**
 * @brief Nest agent net manager 
 */
 
#include <stdint.h>
#include <map>
#include "nest_channel.h"
#include "nest_proto.h"


#ifndef __NEST_AGENT_NET_DISPATCH_H_
#define __NEST_AGENT_NET_DISPATCH_H_

using std::map;
using std::vector;

namespace nest
{
	class TMsgDispatch
	{
	public:
        virtual ~TMsgDispatch(){}
		virtual int32_t dispatch(void* msg) { return -1; };	
	};

	typedef std::map<uint32_t, TNestListen *>  TNestListenMap;
    typedef std::vector<TNestListen*>          TNestListenList;

  
    /** 
     * @brief ����Զ�̽�����������ȫ�ֹ���
     */
    class  TAgentNetDispatch : public TNestDispatch
    {
    public:

        ///< ��������ʺ���
        static TAgentNetDispatch* Instance();

        ///< ȫ�ֵ����ٽӿ�
        static void Destroy();
        

        ///< ����������
        virtual ~TAgentNetDispatch();

		int32_t AddListen(const TNestAddress& addr, bool tcp = true);
		int32_t SetMsgDispatch(TMsgDispatch *msg_handler);

        ///< ��ѯ��̬������Ϣ
        TNestChannel* FindChannel(uint32_t fd);  


        /**
         * @brief �ַ�������
         */
        virtual int32_t DoRecv(TNestBlob* channel);
        virtual int32_t DoAccept(int32_t cltfd, TNestAddress* addr);
        virtual int32_t DoError(TNestChannel* channel);

        
        /**
         * @brief ִ����Ϣ�շ�, ������������
         */
        void RunService();

    private:

        // �������캯��
        TAgentNetDispatch();
        
        static TAgentNetDispatch*        _instance;       // ��������

        TNestChannelMap             _channel_map;    // ���ӹ���map
		TNestListenMap             _listen_map;     // ���ؼ����б�
        

		TMsgDispatch*				_msg_dispatch;     // ��Ϣ�������
    };


}

#endif


