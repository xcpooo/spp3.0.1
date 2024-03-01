#include "tmempool.h"

//////////////////////////////////////////////////////////////////////////

CMemPool::CMemPool()
{
}

CMemPool::~CMemPool()
{
}

void* CMemPool::allocate(unsigned size, unsigned& result_size)
{
    result_size = size;
    return malloc(size);
}


int CMemPool::recycle(void* mem, unsigned mem_size)
{
	if(!mem || mem_size == 0) {
		return -1;	
	}
	else {
		free(mem);
		mem = NULL;
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////
///:~
