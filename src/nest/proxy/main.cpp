#include "nest_proxy.h"

int main(int argc, char* argv[])
{
    CServerBase* proxy = new CNestProxy;
    proxy->run(argc, argv);
    delete proxy;
    return 0;
}
