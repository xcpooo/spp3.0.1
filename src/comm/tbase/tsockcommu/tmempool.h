#ifndef _TBASE_TSOCKCOMMU_TMEMPOOL_H_
#define _TBASE_TSOCKCOMMU_TMEMPOOL_H_

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
//	memory poll, allocate fixed size memory
class CMemPool
{
public:
	CMemPool(); 
	~CMemPool();

	void* allocate(unsigned size, unsigned& result_size);
	int recycle(void* mem, unsigned mem_size);
};

#endif
