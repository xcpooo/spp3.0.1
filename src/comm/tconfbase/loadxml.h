#ifndef _SPP_LOADXML_H__
#define _SPP_LOADXML_H__

#include "loadconfbase.h"
#include "tinyxmlcomm.h"
namespace spp
{
namespace tconfbase
{
class CLoadXML: public CLoadConfBase
{
    public:
        CLoadXML(const std::string& filename);
        virtual ~CLoadXML();
        void fini();

        // 传入的vector表示读取XML信息的路径
        // 比如 <proxy> <log/></proxy>
        // 则传入的vector为 ["proxy", "log"]
        
        int getAcceptorValue(AcceptorSock& acceptor, std::vector<std::string> vec);
        int getLogValue(Log& log, std::vector<std::string> vec);
        int getLimitValue(Limit& limit, std::vector<std::string> vec);
        int getStatValue(Stat& stat, std::vector<std::string> vec);
        int getProcmonValue(Procmon& procmon, std::vector<std::string> vec); 
        int getModuleValue(Module& module, std::vector<std::string> vec);
        int getResultValue(Result& result, std::vector<std::string> vec);
        int getConnectShmValue(ConnectShm& connect, std::vector<std::string> vec);
        int getMoniValue(Moni& moni, std::vector<std::string> vec);
        int getSessionConfigValue(SessionConfig& conf, std::vector<std::string> vec);
        int getReportValue(Report& report, std::vector<std::string> vec);
        
        int getProxyValue(Proxy& proxy, std::vector<std::string> vec);
        int getWorkerValue(Worker& worker, std::vector<std::string> vec);
        int getCtrlValue(Ctrl& ctrl, std::vector<std::string> vec);
        int getIptableValue(Iptable& iptable, std::vector<std::string> vec);

        int getAsyncSessionValue(AsyncSession& asyncSession, std::vector<std::string> vec) ;
		int getExceptionValue(Exception& exception, std::vector<std::string> vec);
        int getClusterValue(ClusterConf& cluster, std::vector<std::string> vec);

	    int getCGroupConfValue(CGroupConf& cgroupconf, std::vector<std::string> vec);
		
        int loadConfFile();
    protected:
        // 调用从配置文件中读取信息前的检查操作
        // vec: 就是get***Value中的vec, 用于检查
        spp::tinyxml::TiXmlElement* checkBeforeGet(const std::vector<std::string>& vec); 
        // 把传入的vector(例如["proxy", "log"])转化成tinyxml 固有的类型TiXmlElement*
        // vec: 类似 ["proxy", "log"]的列表
        spp::tinyxml::TiXmlElement* getElemByKey(const std::vector<std::string>& vec);
    private:
        spp::tinyxml::TiXmlDocument* _pDoc;
};
}
}
#endif

