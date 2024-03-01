#include "nest_channel.h"
#include "nest_proto.h"


#ifndef __NEST_MSG_INCL_H_
#define __NEST_MSG_INCL_H_

using std::map;
using std::vector;

namespace nest
{

    /**
     * @brief ÏϢ·ַ¢Ö¼ä¹¹Ì, agentרÓ¶¨Ò
     */
    typedef struct _nest_msg_info
    {
        TNestBlob*      blob_info;          // ÏϢ4ԴµÈÅ¢
        NestPkgInfo*    pkg_info;           // ÏϢpb¸ñÅª
        nest_msg_head*  nest_head;          // pb¸ñÄûÐϢ       
    }NestMsgInfo;

    /**
     * @brief ÏϢ·ַ¢µĽӿڶ¨Ò
     */
    class TNestMsgHandler
    {
    public:
    
        // ÐÄµÄ����¯Ê
        virtual ~TNestMsgHandler(){};

        // ʵ¼ʵÄûÀ½ӿں¯Ê, ¼̳ÐµÏ
        virtual int32_t Handle(void* args) = 0;
    };
    typedef std::map<uint64_t, TNestMsgHandler*>  MsgDispMap;

};

#endif
