#include "nest_agent.h"

using namespace spp;
using namespace nest;

int main(int argc, char* argv[])
{
    NestAgent* agent = new NestAgent;
    agent->run(argc, argv);
    delete agent;
    return 0;
}
