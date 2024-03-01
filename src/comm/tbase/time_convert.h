/**
 * @filename time_convert.h
 * @brief 时间转化，时区固定为北京时区  
 * @info  拷贝自glibc-__offtime，由于glibc内部实现localtime_r加了线程锁，可能导致一些无法预料的问题
 */


#ifndef __TIME_CONVERT_H__
#define __TIME_CONVERT_H__

struct tm *localtime_r_nl(const time_t *timep, struct tm *result);

#endif

