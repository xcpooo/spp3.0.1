#include "keygen.h"
#include "crc32.h"

#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <linux/limits.h>

using namespace spp::comm;
using namespace platform::commlib;

namespace spp
{
namespace comm
{
//use pwd & id to generate shm/mq key
//if mq, id = 255
//if shm, id = groupid
key_t pwdtok(int id)
{
    char seed[PATH_MAX] = {0};
    readlink("/proc/self/exe", seed, PATH_MAX);
    dirname(seed);	//seed changed

    //把keyseed和id组合成一个字符串
    uint32_t seed_len = strlen(seed) + sizeof(id);
    memcpy(seed + strlen(seed), &id, sizeof(id));

    CCrc32 generator;
    return generator.Crc32((unsigned char*)seed, seed_len);
}
} // end namespace comm
} // end namespace spp

