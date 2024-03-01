#include "loadconfbase.h"

using namespace spp::tconfbase;
CLoadConfBase::CLoadConfBase(const std::string& filename)
{
    _fileName = filename;
}
CLoadConfBase::~CLoadConfBase()
{
    return;
}
std::string CLoadConfBase::trans(const std::vector<std::string>& vec)
{
    std::string res;
    for (size_t i = 0; i < vec.size(); ++ i)
    {
        res += vec[i];
        res += "/";
    }
    return res;
}
