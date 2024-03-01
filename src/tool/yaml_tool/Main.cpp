#include <iostream>
#include <string.h>
#include <unistd.h>
#include "Megatron.h"

using namespace std;

const char* C_SPP_YAML_TOOL_VERSION = "yaml_tool_1.2.4";

int getInstanceName(std::string& name)
{
    char buf[1024] = {0};
    if(NULL == getcwd(buf, sizeof(buf)))
    {
        cerr << "getcwd error:%m" << endl;
        return -1;
    }
    // buf = "/usr/local/services/spp_xxx-1.0/bin"
    name = buf;
    size_t pos = name.rfind('/');
    if(pos == string::npos)
    {
        cerr << "yaml_tool must put into spp bin directory." << endl;
        return -2;
    }
    // name = "/usr/local/services/spp_xxx-1.0"
    name = name.substr(0, pos);

    pos = name.rfind('/');
    if(pos != string::npos)
    {
        // name = "spp_xxx-1.0"
        name = name.substr(pos + 1);
    }
    // else be compatible with root directory

    pos = name.rfind('-');
    if(pos != string::npos)
    {
        // name = "spp_xxx"
        name = name.substr(0, pos);
    }
    // else be compatible with dev directory
    else
    {
        name = "spp";
    }

    return 0;
}

int main(int argc, char** argv)
{
    stringstream ss;
    ss << C_SPP_YAML_TOOL_VERSION << " by aoxu@tencent.com" << endl
        << "spp yaml/xml config files generator." << endl
        << "usage:\t" << argv[0] << " [x|y]" << endl
        << "\tx: generate xml (from yaml)" << endl
        << "\ty: generate yaml (from xml)" << endl;
    if(argc == 1)
    {
        cout << ss.str();
        return 0;
    }

    int ret = 0;
    std::string instanceName;
    ret = getInstanceName(instanceName);
    if(0 != ret)
    {
        return ret;
    }
    else
    {
        cout << "Instance name: " << instanceName << endl;
    }

    std::string sppYAMLFile = "../etc/spp.yaml";
    std::string serviceYAMLFile = "../etc/service.yaml";
    std::string XMLFile = "../etc/spp_ctrl.xml";

    // 根据参数决定是YAML to XML 还是 XML to YAML
    Megatron holder;
    ret = holder.init(instanceName);
    if(0 != ret)
    {
        cerr << "Init error: " << ret << endl;
        return ret;
    }

    if(*argv[1] == 'x')
    {
        ret = holder.YAMLtoXML(sppYAMLFile, serviceYAMLFile, XMLFile);
    }
    else if(*argv[1] == 'y')
    {
        ret = holder.XMLtoYAML(XMLFile, sppYAMLFile, serviceYAMLFile);
    }
    else
    {
        cout << ss.str();
        return 0;
    }

    if(0 != ret)
    {
        cerr << "Transform failed: " << ret << endl;
        return ret;
    }
    else
    {
        cout << "Transform OK!" << endl;
    }

    return 0;
}
