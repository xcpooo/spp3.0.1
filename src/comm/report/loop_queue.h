#ifndef __LOOP_QUEUE_H__
#define __LOOP_QUEUE_H__

#include <cstddef>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>

#include "shm_queue.h"

class CLoopQueue
{
	public:
		/**
		 * @brief  单例
		 */
		static CLoopQueue* Instance(void);
		
		/**
		 * @brief  初始化
		   @param arg1 挂载的文件名
		   @param arg2 队列大小
		   @param arg3 单个存储空间大小
		   @return 0-成功, <0 失败上报异常
		 */
		int init(const char *file, int32_t max_unit_num, int32_t unit_size);
		
		/**
		 * @brief  反初始化
		 */
		void term();

		/**
		 * @brief  压入数据
		   @return 0-成功, <0 失败
		 */
		int push(void* data, uint32_t size);
		
		/**
		 * @brief  弹出数据
		   @return 0-成功, <0 失败
		 */
		int pop(void* data, uint32_t size, struct timeval *tv = NULL);

	private:
		CLoopQueue();
		~CLoopQueue();

		static CLoopQueue* _instance;          /// 单例指针
		struct sq_head_t * _queue;                   /// 基地址
		
};

#endif
