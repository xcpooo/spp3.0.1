#include "defaultproxy.h"

using namespace spp::proxy;

int main(int argc, char* argv[])
{
    CServerBase* proxy = new CDefaultProxy;
    proxy->run(argc, argv);
    delete proxy;
    return 0;
}
