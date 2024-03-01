/**
  *   @filename  hash_list.h
  *   @info  ����hash��, ��hash�洢ʵ��; �̳���ʵ��hashӳ��, ע�����Ԫ�ص�
  *          ��������, ��ջ�����Ȼ��Զ�������Ԫ��, ��Ҫ����ڴ�, ����ỵ��
  *   @time  2013-06-11
  */

#ifndef __HASH_LIST_FILE__
#define __HASH_LIST_FILE__

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

namespace nest {

/**
 *  @brief Hash���Ԫ�صĻ���, �̳и�Ԫ�ؼ���ʵ����չ
 */
class HashKey
{
private:
    HashKey*  _next_entry;          ///< ����hash������Ԫ��
    uint32_t  _hash_value;          ///< hash value��Ϣ, ��Լ�Ƚϵ�ʱ��
    void*     _data_ptr;            ///< hash data����ָ��, ��key - value �ۺϴ洢
    
public:

    friend class HashList;          ///< hash�����ֱ�ӷ���nextָ��

    /**
     *  @brief ����������������
     */
    HashKey():_next_entry(NULL), _hash_value(0), _data_ptr(NULL) {};
    virtual ~HashKey(){};

    /**
     *  @brief �ڵ�Ԫ�ص�hash�㷨, ��ȡkey��hashֵ
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual uint32_t HashValue() = 0; 

    /**
     *  @brief �ڵ�Ԫ�ص�cmp����, ͬһͰID��, ��key�Ƚ�
     *  @return �ڵ�Ԫ�ص�hashֵ
     */
    virtual int HashCmp(HashKey* rhs) = 0; 
    
    /**
     *  @brief �ѱ����ӿ�, ���ڵ���, �ڱ���ÿ��Ԫ��ʱ������, ��ѡʵ��
     */
    virtual void HashIterate() {  
        return;
    };

    /**
     *  @brief �ڵ�Ԫ�ص�ʵ������ָ���������ȡ
     */
    void* GetDataPtr() {
        return _data_ptr;
    }; 
    void SetDataPtr(void* data) {
        _data_ptr = data;
    };
    

};


/**
 *  @brief Hash������, ����ʽhash, ע��ѡ����ʵ�hash����, �����ͻ����
 */
class HashList
{
public:

    /**
     *  @brief ���캯������������
     */
	explicit HashList(int max = 100000) {
        _max = GetMaxPrimeNum((max > 2) ? max : 100000);
		_buckets = (HashKey**)calloc(_max, sizeof(HashKey*));
		_count = 0;
	};
	virtual ~HashList()  {
		if (_buckets) {
		    free(_buckets);
			_buckets = NULL;
		}
        _count = 0;
	};

    /**
     *  @brief ��ȡhash��Ԫ�ظ���
     *  @return ��Ԫ��ʵ����Ŀ
     */
    int HashSize() {
        return _count;
    };

    /**
     *  @brief hash����Ԫ��, Ҫ�ڸ�Ԫ������ǰ, ����remove
     *  @param key �������Ԫ��ָ��, ע��Ԫ�ص���������, ��Ҫ����ջ����
     *  @return 0 �ɹ�, -1 ������Ч��δ��ʼ��, -2 �ظ������������
     */
    int HashInsert(HashKey* key) {
        if (!key || !_buckets) {
            return -1;
        }

        if ((key->_hash_value != 0) || (key->_next_entry != NULL)) {
            return -2;
        }        
        
        key->_hash_value = key->HashValue();
        int idx = (key->_hash_value) % _max;        

        HashKey* next_item = _buckets[idx];
        _buckets[idx]      = key;
        key->_next_entry   = next_item;
        _count++;
        return 0; 
    }

    /**
     *  @brief hash����Ԫ��
     *  @param key ����ѯ��keyָ��
     *  @return ��ѯ�������ָ��, NULL����������
     */
    HashKey* HashFind(HashKey* key) {
        if (!key || !_buckets) {
            return NULL;
        }
        
        uint32_t hash = key->HashValue();
        int idx = hash % _max;
        HashKey* item = _buckets[idx];
        
        for (; item != NULL; item = item->_next_entry) {
            if (item->_hash_value != hash) {
                continue;
            }

            if (item->HashCmp(key) == 0) {
                break;
            }
        }
        
        return item; 
    }
    
    /**
     *  @brief hash����Ԫ��
     *  @param key ����ѯ��keyָ��
     *  @return ��ѯ�������ָ��, NULL����������
     */
    void* HashFindData(HashKey* key) {
        HashKey* item = HashFind(key);
        if (!item) {
            return NULL;
        } else {
            return item->_data_ptr;
        }
    };
    

    /**
     *  @brief hashɾ��Ԫ��
     *  @param key ��ɾ����keyָ��
     */
    void HashRemove(HashKey* key) {
        if (!key || !_buckets) {
            return;
        }
        
        uint32_t hash = key->HashValue();
        int idx = hash % _max;
        HashKey* item = _buckets[idx];
        HashKey* prev = NULL;
        
        for (; item != NULL; prev = item, item = item->_next_entry) {
            if ((item->_hash_value == hash) && (item->HashCmp(key) == 0)){
                if (prev == NULL) {
                    _buckets[idx] = item->_next_entry;
                } else {
                    prev->_next_entry = item->_next_entry;
                }
                item->_hash_value = 0;
                item->_next_entry = NULL;                
                _count--;
                break;
            }
        }
    }

    /**
     *  @brief hash����Ԫ��, ���õ�������
     */
    void HashForeach() {
        if (!_buckets) {
            return;
        }
        
        for (int i = 0; i < _max; i++) {
            HashKey* item = _buckets[i];
            for (; item != NULL; item = item->_next_entry) {
                item->HashIterate();
            }
        }
    }
    
    /**
     *  @brief hash�������, ���ܵ���, ֻ�������ձ�������
     */
    HashKey* HashGetFirst() {
        if (!_buckets) {
            return NULL;
        }
        
        for (int i = 0; i < _max; i++) {
            if (_buckets[i]) {
                return _buckets[i];
            }
        }
        
        return NULL;
    }    

private:

    /**
     *  @brief ��ȡͰ���ȵ��������
     */
    int GetMaxPrimeNum(int num) 
    {
    	int sqrt_value = (int)sqrt(num);
    	for (int i = num; i > 0; i--)
    	{
    		int flag = 1;
    		for (int k = 2; k <= sqrt_value; k++)
    		{
    			if (i % k == 0)
    			{
    				flag = 0;
    				break;
    			}
    		}

    		if (flag == 1)
    		{
    			return i;
    		}
    	}

    	return 0;
    };


private:
    HashKey** _buckets;             ///< Ͱָ��
    int       _count;               ///< ��ЧԪ�ظ���
    int       _max;                 ///< ���ڵ����
};

}


#endif

