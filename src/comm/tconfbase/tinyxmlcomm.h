#ifndef _SPP_TINYXMLCOMM_H__
#define _SPP_TINYXMLCOMM_H__
#include <string>
#include "../tinyxml/tinyxml.h"

#define TINYXML_COMM_NULL_POINTER -100 
#ifdef TIXML_USE_STL
namespace spp
{
    namespace tinyxml
    {
// 从element表示的节点信息中读取属性为name的值, 并存放在_value之中, 如果该值不存在, 或者为空, 则_value被填写为_defvalue
// 该方法所对应的都是string类型
// element: tinyxml 固有的节点元素
// name: xml中属性的字符串信息
// _value: 获得属性所对应的值
// _defvalue: 默认的_value值

int QueryStringAttributex(const TiXmlElement* element, const char* name, std::string* _value, const std::string& _defvalue = "");

// 从element表示的节点信息中读取属性为name的值, 并存放在val之中, 如果该值不存在, 或者为空, 则val被填写为defval
// 这里T的类型可以为 bool, int, short, unsigned int, unsigned short.
// element: tinyxml 固有的节点元素
// name: xml中属性的字符串信息
// _value: 获得属性所对应的值
// _defvalue: 默认的_value值
template<typename T>
int QueryNumberAttributex(const TiXmlElement* element, const char* name, T* val, const T& defval = 0)
{
    if (element == NULL)
            return TINYXML_COMM_NULL_POINTER;
    int tmp = 0;
    int ret = element->QueryIntAttribute( name, &tmp );
    if (ret == TIXML_NO_ATTRIBUTE || ret == TIXML_WRONG_TYPE)
    {
            *val = defval;
    }
    else
    {
            *val = (T)tmp;
    }
    return ret;
}
}
}
#endif
#endif
