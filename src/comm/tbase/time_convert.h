/**
 * @filename time_convert.h
 * @brief ʱ��ת����ʱ���̶�Ϊ����ʱ��  
 * @info  ������glibc-__offtime������glibc�ڲ�ʵ��localtime_r�����߳��������ܵ���һЩ�޷�Ԥ�ϵ�����
 */


#ifndef __TIME_CONVERT_H__
#define __TIME_CONVERT_H__

struct tm *localtime_r_nl(const time_t *timep, struct tm *result);

#endif

