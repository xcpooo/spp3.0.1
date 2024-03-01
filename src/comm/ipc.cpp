#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include"IPC.h"
using namespace::spp::ipc;
void* IPC::map_file(const char* filename, int size)
{
    int fd = ::open(filename, O_RDWR | O_CREAT, 0666);
    void *map = NULL;

    if (fd >= 0) {
        if (size > 0)
            ftruncate(fd, size);
        else
            size = lseek(fd, 0L, SEEK_END);

        if (size > 0)
            map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        ::close(fd);
    } else if (size > 0) {
        map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    }

    if (map == MAP_FAILED) {
        map = NULL;
    }

    return map;

}
