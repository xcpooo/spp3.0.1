/**
 *  @filename mt_cache.h
 *  @info   TCP����buffer������
 */

#ifndef ___MT_BUFFER_CACHE_H
#define ___MT_BUFFER_CACHE_H

#include <stdint.h>
#include <sys/queue.h>


namespace NS_MICRO_THREAD {


// Ĭ�ϵ�buff��С
#define SK_DFLT_BUFF_SIZE   64*1024   
#define SK_DFLT_ALIGN_SIZE  8 

#define SK_ERR_NEED_CLOSE   10000

/**
 * @brief  �û�̬ buffer �ṹ����
 */
typedef struct _sk_buffer_tag
{
    TAILQ_ENTRY(_sk_buffer_tag) entry;     // list entry buffer LRU��
    uint32_t                    last_time; // �ϴ�ʹ��ʱ���
    uint32_t                    size;      // buffer�ڵ�Ŀռ��С
    uint8_t*                    head;      // buff������ͷָ��
    uint8_t*                    end;       // buff����������ָ��
    uint8_t*                    data;      // ��Ч���ݵ�ͷָ��
    uint32_t                    data_len;  // ��Ч�����ݳ���
    uint8_t                     buff[0];   // ԭʼָ������
} TSkBuffer;
typedef TAILQ_HEAD(__sk_buff_list, _sk_buffer_tag) TSkBuffList;  // multi �����������


/**
 * @brief ����ָ����С��buff��
 * @param size ��Ч��������С
 * @return ��NULLΪ�ɹ����ص�buffָ��
 */
TSkBuffer* new_sk_buffer(uint32_t size = SK_DFLT_BUFF_SIZE);

/**
 * @brief �ͷ�ָ����buff��
 * @param ���ͷŵ�buffָ��
 */
void delete_sk_buffer(TSkBuffer* buff);


/**
 * @brief �������󳤶���Ϣ(����Դ�ػ���buff,����չ)
 * @param buff -���е�buffָ��
 * @param size -��Ҫ��չ�����ճ��ȴ�С
 * @return ʵ�ʵ�buff��Ϣ
 */
TSkBuffer* reserve_sk_buffer(TSkBuffer* buff, uint32_t size);


/**
 * @brief  buffer cache ����
 */
typedef struct _sk_buff_mng_tag
{
    TSkBuffList                 free_list;      // buff���� 
    uint32_t                    expired;        // ��ʱʱ��
    uint32_t                    size;           // buff��С
    uint32_t                    count;          // �����
} TSkBuffMng;


/**
 * @brief  cache �صĳ�ʼ���ӿ�
 * @param  mng -����ص�ָ��
 * @param  expired -�����ʱ��, ��λ��
 * @param  size -�������Ĭ�����ɵĿ��С
 */
void sk_buffer_mng_init(TSkBuffMng* mng, uint32_t expired, uint32_t size = SK_DFLT_BUFF_SIZE);

/**
 * @brief  cache �ص����ٽӿ�
 * @param  mng -����ص�ָ��
 */
void sk_buffer_mng_destroy(TSkBuffMng * mng);


/**
 * @brief  �������һ��buff
 * @param  mng -����ص�ָ��
 * @return ��NULLΪ�ɹ���ȡ��buff��ָ��
 */
TSkBuffer* alloc_sk_buffer(TSkBuffMng* mng);

/**
 * @brief �ͷ�ָ����buff��
 * @param  mng -����ص�ָ��
 * @param  buff -���ͷŵ�buffָ��
 */
void free_sk_buffer(TSkBuffMng* mng, TSkBuffer* buff);

/**
 * @brief ���չ��ڵ�buff��
 * @param  mng -����ص�ָ��
 * @param  now -��ǰ��ʱ��, �뼶��
 */
void recycle_sk_buffer(TSkBuffMng* mng, uint32_t now);


/**
 * @brief ԭʼ�� buffer cache ����
 */
typedef struct _sk_rw_cache_tag
{
    TSkBuffList                 list;      // buff���� 
    uint32_t                    len;       // ���ݳ���
    uint32_t                    count;     // �����
    TSkBuffMng                 *pool;      // ȫ��buff��ָ��
} TRWCache;


/**
 * @brief Cache��������ʼ��
 * @param cache -�����ָ��
 * @param pool -buff��ָ��
 */
void rw_cache_init(TRWCache* cache, TSkBuffMng* pool);

/**
 * @brief Cache����������
 * @param cache -�����ָ��
 */
void rw_cache_destroy(TRWCache* cache);

/**
 * @brief Cacheɾ����ָ����������
 * @param cache -�����ָ��
 * @param len -��ɾ���ĳ���
 */
void cache_skip_data(TRWCache* cache, uint32_t len);

/**
 * @brief Cache�Ƴ���һ���ڴ�
 * @param cache -�����ָ��
 */
TSkBuffer* cache_skip_first_buffer(TRWCache* cache);


/**
 * @brief Cache׷��ָ����������
 * @param cache -�����ָ��
 * @param data -��׷�ӵ�ָ��
 * @param len -��׷�ӵĳ���
 */
int32_t cache_append_data(TRWCache* cache, const void* data, uint32_t len);

/**
 * @brief Cache׷��ָ����������
 * @param cache -�����ָ��
 * @param buff -��׷�ӵĿ�ָ��
 */
void cache_append_buffer(TRWCache* cache, TSkBuffer* buff);

/**
 * @brief Cacheɾ��������ָ����������
 * @param cache -�����ָ��
 * @param buff -���buff��ָ��
 * @param len -��ɾ���ĳ���
 * @return ʵ�ʿ�������
 */
uint32_t cache_copy_out(TRWCache* cache, void* buff, uint32_t len);


/**
 * @brief Cache���ϵ�UDP�ձ��ӿ�, �����ڴ�Ƚ϶�, ������32λʹ��
 * @param cache -�����ָ��
 * @param fd - ׼���ձ���fd���
 * @param remote_addr -�Զ�ip��ַ
 * @return ʵ�ʽ��ճ���
 */
int32_t cache_udp_recv(TRWCache* cache, uint32_t fd, struct sockaddr_in* remote_addr);

/**
 * @brief Cache���ϵ�TCP�ձ��ӿ�
 * @param cache -�����ָ��
 * @param fd - ׼���ձ���fd���
 * @return ʵ�ʽ��ճ���
 */
int32_t cache_tcp_recv(TRWCache* cache, uint32_t fd);

/**
 * @brief Cache���ϵ�TCP���ͽӿ�
 * @param cache -�����ָ��
 * @param fd - ׼��������fd���
 * @return ʵ�ʷ��ͳ���
 */
int32_t cache_tcp_send(TRWCache* cache, uint32_t fd);

/**
 * @brief Cache���ϵ�TCP���ͽӿ�, δʹ��IOVEC
 * @param cache -�����ָ��
 * @param fd - ׼��������fd���
 * @param data -������cache��, �������͵�buff
 * @param len  -�������͵�buff����
 * @return ʵ�ʷ��ͳ���
 */
int32_t cache_tcp_send_buff(TRWCache* cache, uint32_t fd, const void* data, uint32_t len);





// interface
typedef void*  TBuffVecPtr;        ///< ���block��cache����ָ����
typedef void*  TBuffBlockPtr;      ///< ���������ָ����


/**
 * @brief ��ȡcache��Ч�����ܳ���
 * @param multi -�����ָ��
 * @return ʵ����Ч���ݳ���
 */
uint32_t get_data_len(TBuffVecPtr multi);

/**
 * @brief ��ȡcache��Ч���ݿ����
 * @param multi -�����ָ��
 * @return ʵ����Ч���ݿ����
 */
uint32_t get_block_count(TBuffVecPtr multi);

/**
 * @brief ��ȡcache�ĵ�һ������ָ��
 * @param multi -�����ָ��
 * @return ��һ������ָ��
 */
TBuffBlockPtr get_first_block(TBuffVecPtr multi);

/**
 * @brief ��ȡcache����һ������ָ��
 * @param multi -�����ָ��
 * @param block -��ǰ��ָ��
 * @return ��һ������ָ��
 */
TBuffBlockPtr get_next_block(TBuffVecPtr multi, TBuffBlockPtr block);

/**
 * @brief ��ȡ���ݿ��ָ�������ݳ���
 * @param block -��ǰ��ָ��
 * @param data -����ָ��-modify����
 * @param len  -����ָ�� modify����
 */
void get_block_data(TBuffBlockPtr block, const void** data, int32_t* len);


/**
 * @brief ��ȡ���ݿ��ָ�������ݳ���
 * @param multi -�����ָ��
 * @param data -����д������ָ��
 * @param len  -����
 * @return ���ݶ�ȡ�����ݳ���
 */
uint32_t read_cache_data(TBuffVecPtr multi, void* data, uint32_t len);


/**
 * @brief ��ȡ���ݿ��ָ�������ݳ���
 * @param multi -�����ָ��
 * @param data -����д������ָ��
 * @param len  -����
 * @return ���ݶ�ȡ�����ݳ���
 */
uint32_t read_cache_begin(TBuffVecPtr multi, uint32_t begin, void* data, uint32_t len);


};

#endif
