#include "loop_queue.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <string.h>
#include <stdio.h>

CLoopQueue *CLoopQueue::_instance = NULL;

CLoopQueue* CLoopQueue::Instance()
{
	if (NULL == _instance )
	{
		_instance = new CLoopQueue();
	}

	return _instance;
}

CLoopQueue::CLoopQueue()
{
	_queue = NULL;
}

CLoopQueue::~CLoopQueue()
{
}

int CLoopQueue::init(const char *file, int32_t max_unit_num, int32_t unit_size)
{
	if (!file || !unit_size || !max_unit_num){   	
		printf("loop queue arg error\n");
		return -1;
	}

	if (_queue){
		printf("loop queue already init\n");
		return -1;
	}

	_queue = sq_open(file, unit_size, max_unit_num);

	return _queue ? 0 : -1;
}

void CLoopQueue::term(void)
{
}

int CLoopQueue::push(void *unit, uint32_t size)
{
	if (!_queue)
		return -2;

	if (!unit)
		return -3;

	int iret = sq_put(_queue, unit, size);

	return iret == 0 ? 0 : -1;
}

int CLoopQueue::pop(void *unit, uint32_t size, struct timeval *tv)
{
	if (!_queue)
		return -2;

	if (!unit)
		return -3;

	int iret = sq_get(_queue, unit, size, tv);

	return iret > 0 ? 0 : -1;
}
