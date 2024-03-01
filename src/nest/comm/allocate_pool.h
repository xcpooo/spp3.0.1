/**
  *   @filename  allocate_pool.h
  *   @info  ��̬�������Ĺ����
  */

#ifndef __ALLOCATE_POOL_FILE__
#define __ALLOCATE_POOL_FILE__

#include <stdint.h>
#include <queue>

using std::queue;

namespace nest 
{

/**
 * @brief ��̬�ڴ��ģ����, ���ڷ���new/delete�Ķ������, ��һ���̶����������
 */
template<typename value_type>
class CAllocatePool
{
public:
    typedef typename std::queue<value_type*>  CPtrQueue; ///< �ڴ�ָ�����
    
public:

    /**
     * @brief ��̬�ڴ�ع��캯��
     * @param max �����ж��б����ָ��Ԫ��, Ĭ��500
     */
    explicit CAllocatePool(uint32_t max = 500) : _max_free(max), _total(0){};
    
    /**
     * @brief ��̬�ڴ����������, ���������freelist
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
     * @brief �����ڴ�ָ��, ���ȴӻ����ȡ, �޿��п�����̬ new ����
     * @return ģ�����͵�ָ��Ԫ��, �ձ�ʾ�ڴ�����ʧ��
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
     * @brief �ͷ��ڴ�ָ��, �����ж��г������, ��ֱ���ͷ�, ������л���
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

    CPtrQueue _ptr_list;           ///<  ���ж���
    uint32_t  _max_free;           ///<  ������Ԫ�� 
    uint32_t  _total;              ///<  ����new�Ķ������ͳ��
};


}


#endif

