
#include "tinyxmlcomm.h"
namespace spp
{
    namespace tinyxml
    {
int QueryStringAttributex(const TiXmlElement* element, const char* name, std::string* _value, const std::string& _defvalue /*= ""*/)
{
    if (element == NULL)
        return TINYXML_COMM_NULL_POINTER;
    std::string tmp = "";
    int ret = element->QueryStringAttribute(name, &tmp);
    if (tmp != "")
        *_value = tmp;
    else
        *_value = _defvalue;

    return ret;
}
}
}
