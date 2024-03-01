#ifndef IPC_H_
#define IPC_H_

namespace spp
{

    namespace ipc
    {
        class IPC
        {
        public:
            static void* map_file(const char* filename, int size);
        };

    }

}



#endif
