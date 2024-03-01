#include "../comm/tbase/misc.h"
#include "defaultctrl.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

using namespace spp::ctrl;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("usage: ./spp_ctrl config_file\n");
        exit(-1);
    }

    mkdir("../log", 0700);
    mkdir("../stat", 0700);
    mkdir("../moni", 0700);
    CServerBase* ctrl = new CDefaultCtrl;
    ctrl->run(argc, argv);
    delete ctrl;
    
    return 0;
}
