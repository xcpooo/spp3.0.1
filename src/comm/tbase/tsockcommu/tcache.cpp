#include <string.h>
#include "tcache.h"
#include "tlog.h"
#include "global.h"
#include "singleton.h"

using namespace spp::singleton;
using namespace tbase::tlog;
using namespace tbase::tcommu::tsockcommu;

#define RAW_CACHE_DEFAULT_SIZE 65536

#ifndef min
#define min(x, y) ({                \
    typeof(x) _min1 = (x);          \
    typeof(y) _min2 = (y);          \
    (void) (&_min1 == &_min2);      \
    _min1 < _min2 ? _min1 : _min2; })
#endif

#ifndef max
#define max(x, y) ({                \
    typeof(x) _max1 = (x);          \
    typeof(y) _max2 = (y);          \
    (void) (&_max1 == &_max2);      \
    _max1 > _max2 ? _max1 : _max2; })
#endif

CRawCache::CRawCache(CMemPool& mp)
        : _mp(mp), _mem(NULL), _block_size(0), _data_head(0), _data_len(0), _fin_bit(false)
{}

CRawCache::~CRawCache() {}

// 返回数据基地址
char* CRawCache::data()
{
    if (_data_len == 0)
        return NULL;

    if (_data_head >= _block_size) {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "CRawCache _data_head:%u >= _block_size:%u\n", _data_head, _block_size);
        exit(-1);
    }

    return _mem + _data_head;
}

// 返回数据长度
unsigned CRawCache::data_len()
{
    return _data_len;
}


// 向RawCache追加数据，如果空闲内存不足则向mempool申请
void CRawCache::append(const char* data, unsigned data_len)
{
    if (_fin_bit) {
        return;
    }

    if (_mem == NULL) {
        // fix 2.3.78 proxy coredump       
        _mem = (char*) _mp.allocate(max(((unsigned)RAW_CACHE_DEFAULT_SIZE),data_len), 
                                   _block_size);

        if (!_mem)
            throw(0);

        memcpy(_mem, data, data_len);

        _data_head = 0;
        _data_len = data_len;

        // modified by tomxiao 2006.10.16
        return;
    }

    //
    //	data_len < _block_size - _data_head - _data_len
    //	append directly
    //
    if (data_len + _data_head + _data_len <= _block_size) {
        memcpy(_mem + _data_head + _data_len, data, data_len);
        _data_len += data_len;
    }
    //
    //	_block_size-_data_len <= data_len
    //	reallocate new block. memmove, recycle
    //
    else if (data_len + _data_len > _block_size) {
        unsigned new_block_size = 0;
        char* mem = (char*) _mp.allocate(max(_data_len+data_len, _block_size * 2), 
                                         new_block_size);

        if (!mem)
            throw(0);

        memcpy(mem, _mem + _data_head, _data_len);
        memcpy(mem + _data_len, data, data_len);
        _mp.recycle(_mem, _block_size);

        _mem = mem;
        _block_size = new_block_size;
        _data_head = 0;
        _data_len += data_len;
    }
    //
    //	_block_size - _data_head - _data_len < data_len < _block_size-_datalen
    //	memmove data to block head, append new data
    //
    else
        //if ((data_len + _data_head + _data_len > _block_size) && (data_len + _data_len < _block_size))
    {
        memmove(_mem, _mem + _data_head, _data_len);
        memcpy(_mem + _data_len, data, data_len);

        _data_head = 0;
        _data_len += data_len;
    }
}

// 跳过（忽略）固定长度
void CRawCache::skip(unsigned length)
{
    //	empty data
    if (_mem == NULL)
        return;

    //	not enough data
    else if (length >= _data_len) {
        _mp.recycle(_mem, _block_size);
        _mem = NULL;
        _block_size = _data_head = _data_len = 0;
        _data_head = 0;
        _data_len = 0;
    }

    //	skip part of data
    else {
        _data_head += length;
        _data_len -= length;
    }
}
