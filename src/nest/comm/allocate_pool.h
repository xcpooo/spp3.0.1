/**
  *   @filename  allocate_pool.h
  *   @info  动态分配管理的管理池
  */

#ifndef __ALLOCATE_POOL_FILE__
#define __ALLOCATE_POOL_FILE__

#include <stdint.h>
#include <queue>

using std::queue;

namespace nest 
{

/**
 * @brief 动态内存池模板类, 对于反复new/delete的对象操作, 可一定程度上提高性能
 */
template<typename value_type>
class CAllocatePool
{
public:
    typedef typename std::queue<value_type*>  CPtrQueue; ///< 内存指针队列
    
public:

    /**
     * @brief 动态内存池构造函数
     * @param max 最大空闲队列保存的指针元素, 默认500
     */
    explicit CAllocatePool(uint32_t max = 500) : _max_free(max), _total(0){};
    
    /**
     * @brief 动态内存池析构函数, 仅仅清理掉freelist
     */
    ~CAllocatePool()    {
        value_type* ptr = NULL;
        while (!_ptr_list.empty()) {
            ptr = _ptr_list.front();
            _ptr_list.pop();
            delete ptr;
        }
    };

    /**
     * @brief 分配内存指针, 优先从缓存获取, 无空闲可用则动态 new 申请
     * @return 模板类型的指针元素, 空表示内存申请失败
     */
    value_type* alloc_ptr() {
        value_type* ptr = NULL;
        if (!_ptr_list.empty()) {
            ptr = _ptr_list.front();
            _ptr_list.pop();
        } else {
            ptr = new value_type;
            _total++;
        }

        return ptr;
    };

    /**
     * @brief 释放内存指针, 若空闲队列超过配额, 则直接释放, 否则队列缓存
     */
    void free_ptr(value_type* ptr) {
        if ((uint32_t)_ptr_list.size() >= _max_free) {
            delete ptr;
            _total--;
        } else {
            _ptr_list.push(ptr);
        }
    };    
    
protected:

    CPtrQueue _ptr_list;           ///<  空闲队列
    uint32_t  _max_free;           ///<  最大空闲元素 
    uint32_t  _total;              ///<  所有new的对象个数统计
};


}


#endif

