#ifndef _H_DCAPI_H_
#define _H_DCAPI_H_

#include <string>
#include <map>
#include <queue>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stdint.h>

using namespace std;

namespace DataCollector {
	class Sampling;
	class SocketClient;

#undef LT_NORMAL
#undef STAT_TYPE_COUNT
#undef STAT_TYPE_SUM
#undef STAT_TYPE_MIN
#undef STAT_TYPE_MAX

	const int8_t LT_NORMAL = 0; // 一般数据上报（通道0）

	const int STAT_TYPE_COUNT = 0; //统计count
	const int STAT_TYPE_SUM = 1; //统计sum
	const int STAT_TYPE_MIN = 2; //统计min
	const int STAT_TYPE_MAX = 3; //统计max

	class CLogger {
	public:
		enum ProType {
			PRO_STRING = 0,
			PRO_BINARY = 1,
		};

/*-----------------------------------构造函数--------------------------------------*/
		/******************************************
		 名称：构造函数（有DCAgent）
		 参数：sockettype, 上报使用的通讯协议
			 0: unix domain socket
			 1: tcp
			 2: udp
		 	isLock, 是否加锁。如果多个线程共用一个clogger对象，需加锁，否则可以不加。
		 返回：无
		 说明：通常情况直接使用默认参数
		 *******************************************/
		CLogger(int8_t socketType = 0, bool isLock = true);

		/******************************************
		 名称：构造函数（无DCAgent）
		 参数：isTest, 是否测试上报
		 	cmlbAppid, cmlb的appid，具体值请咨询lphelper或DC负责人
		 	isLock, 是否加锁。如果多个线程共用一个clogger对象，需加锁，否则可以不加
		 返回：无
		 说明：此构造函数为本地无安装DCAgent专用
		 *******************************************/
		CLogger(uint16_t cmlbAppid, bool isTest, bool isLock);

		~CLogger();

/*----------------------------------初始化函数--------------------------------------*/

		/******************************************
		 名称：init
		 参数：logname, 即接口ID，DCID，需先在DC接口申请页面注册
		 返回：0 成功, 非0 失败
		 说明：初始化业务数据上报，只能调用一次
		 *******************************************/
		int init(string logName);

		/******************************************
		 名称：init
		 参数：mid, 主调ID
		 返回：0 成功, 非0 失败
		 说明：初始化模调数据上报，只能调用一次
		 *******************************************/
		int init(int mid);

/*--------------------------------数据上报函数-----------------------------------------*/

		/*******************************************
		 名称：write_baselog(通用情况)
		 参数：logType, 数据类型，一般填0或LT_NORMAL即可
			data, 编码后的数据，业务需要直接传递格式为 "k1=v1&k2=v2"的数据(v2已经做过编码)
			fallFlag    该参数已不能控制是否落地，由后端配置项决定
			timestamp   unix时间戳，默认值为系统当前时间
		 返回：0 成功, 非0 失败
		 说明：数据上报函数，一般业务上报用
		 ********************************************/
		int write_baselog(int8_t logType, const string& data,
				bool fallFlag = true, unsigned int timestamp = time(NULL));

		/*******************************************
		 名称：write_baselog
		 参数：data, 编码后的数据，业务需要直接传递格式为 "k1=v1&k2=v2"的数据(v2已经做过编码)或者其他二进制数据(proType参数为1)
		 	len, 数据的长度
			logType, 数据类型，一般填0或LT_NORMAL即可
			proType, 数据类型
				0：字符串
				1：二进制
			timestamp   unix时间戳，默认值为系统当前时间
		 返回：0 成功, 非0 失败
		 说明：数据上报函数，支持二进制PB格式等二进制上报
		 ********************************************/
		int write_baselog(const char* data, unsigned int len,
				int8_t logType = LT_NORMAL, int8_t proType = 0, unsigned int timestamp = time(NULL));

		/*******************************************
		 名称: write_baselog
		 参数: logType, 数据类型，一般填0或LT_NORMAL即可
		 	 timestamp, 数据时间戳，一般填当前时间戳
		 	 fmt, 上报的数据，如"key1=%s&key2=%s"
		 返回: 0 成功, 非0 失败
		 说明: 格式化上报数据的函数
		 *******************************************/
		int write_baselog(int8_t logType, unsigned int timestamp, const char* fmt, ...);


		/*******************************************
		 名称：write_modulelog
		 参数：	sid, 被调ID
		 	ifid, 接口ID
		 	mip, 主调IP
		 	sip, 被调IP
		 	retval, 返回值
		 	result, 调用结果
		 	delay, 调用延时
		 返回: 0 成功, 非0 失败
		 说明：模调数据上报函数
		 ********************************************/
		int write_modulelog(int sid, int ifid, string& mip, string& sip, int retval, int result, int delay);

		//重载以上接口，新增参数fip表来源IP, reqLen表请求包大小(选填)，rspLen表相应包大小(选填)
		int write_modulelog(int sid, int ifid, string& fip, string& mip, string& sip, int retval, int result, int delay,
				int reqLen = 0, int rspLen = 0);

		//重载以上接口，新增主调ID填入，但采样率会根据init时候的主调ID
		int write_modulelog(int mid, int sid, int ifid, string& fip, string& mip, string& sip, int retval, int result, int delay,
				int reqLen = 0,	int rspLen = 0);


		/*******************************************
		 名称：write_statlogs
		 参数： data, 编码后的数据，业务需要直接传递格式为 "k1=v1&k2=v2"的数据(v2已经做过编码)，不包含统计字段，相当于维度
			statistics, 需统计的字段，key为指标，value为指标值
			stattype, 统计类型
				STAT_TYPE_COUNT：对指标计数
				STAT_TYPE_SUM：对指标累加
				STAT_TYPE_MIN：指标最小值
				STAT_TYPE_MAX：指标最大值
			timestamp, unix时间戳，默认值为系统当前时间
		 返回: 0 成功, 非0 失败
		 说明：数据上报进行统计，调用前必须调用set_stat(true)
		 ********************************************/
		int write_statlogs(const string& data, map<string, int>& statistics, int stattype, unsigned int timestamp = time(NULL));


/*-----------------------------------一般函数--------------------------------------*/

		/******************************************
		 名称: set_socketype
		 参数: socketType, 通讯协议
			  0: unix domain socket
			  1: tcp
			  2: udp
		 返回: 无
		 说明: 仅在 init 函数调用之前设置有效 !也可以在构造函数时设定
		 ******************************************/
		void set_socktype(int8_t st);

		/******************************************
		 名称：encode
		 参数：dataSrc 原字符串
			dataDst 编码后字符串
		 返回：0 成功, 非0 失败
		 说明：字符串编码函数，当上报的数值包括=、&、\t、\n 时，需要先编码，否则数据会出错。
		 *******************************************/
		int encode(string& dataSrc, string& dataDst);

		/*******************************************
		 名称：set_stat
		 参数：isStat, 是否采用统计上报
			logType, 数据类型，一般填0或LT_NORMAL即可
			interval, 统计时间间隔（s），超过这个值则发送数据
			count, 统计后的最大记录数，超过这个值则发送数据（这个值和内存使用有关）
		 返回：0 成功, 非0 失败
		 说明：默认上报数据不统计，调用此函数前必须先调用init函数初始化
		 ********************************************/
		int set_stat(bool isStat, int8_t logType,
				unsigned int interval = 60, unsigned int count = 1000000);

		/*******************************************
		 名称：pack_send_buff
		 参数：pBuff, 外部传入的指针，打包成功后会指向一块堆内存做为发送缓冲区，使用完后需要外部delete
		 	retLen, 打包成功后，返回发送缓冲区的长度
		 	data, 编码后的数据，业务需要直接传递格式为 "k1=v1&k2=v2"的数据(v2已经做过编码)
		 	len, 数据的长度
		 	logtype, 数据上报的类型，一般填0或LT_NORMAL即可
		 	proType, 数据类型
		 		0:字符串
		 		1:二进制
		 	timestamp, unix时间戳，默认值为系统当前时间
		 返回: 0 成功, 非0 失败
		 说明：发送缓冲区打包函数，用于外部代码实现自己与dcagent通讯,返回成功后会new一块内存，必须在外部delete
		 ********************************************/
		int pack_send_buff(char*& pBuff, int& retLen, const char* data, unsigned int len,
				int8_t logType = LT_NORMAL, int8_t proType = 0, unsigned int timestamp = time(NULL));

		/******************************************************
		 名称: set_no_sample
		 参数: result, 不采样的result值，此result为模调数据上报write_modulelog调用里的result
		 返回: 无
		 说明: 设置某个模调的返回类型为不采样(只有模调才有效，write_baselog无效)
		 *******************************************************/
		void set_no_sample(int result);

		/*****************************************************
		 设置采样，参考set_no_sample, 只针对模调上报有效，设置某个模调的返回类型需采样
		 *****************************************************/
		void set_sample(int result);

		/*******************************************
		 名称：get_errmsg
		 参数：空
		 返回：错误信息
		 说明：返回调用失败时的错误信息
		 ********************************************/
		string get_errmsg(void);

		/*******************************************
		 名称：get_logtype
		 参数：空
		 返回：当前的数据上报的类型
		 说明：无
		 ********************************************/
		int8_t get_logtype() {return m_type;}

		/*******************************************
		 名称：get_lock
		 参数：空
		 返回：当前对象是否加锁
		 说明：无
		 ********************************************/
		bool get_lock() {return m_needLock;}


		//用于复杂统计存储，每个map为n分钟的统计数据结果
		queue<map<string, map<string, int> > > m_stat_log_queues;
		pthread_rwlock_t m_queue_lock;


	private:
		int init(const string logName, bool isModule);

		void update_errmsg(const char* cErrmsg, ...);

		int comm_write(const char* data, unsigned int len,
				int8_t logType = -1, int8_t proType = -1, unsigned int tm = time(NULL));
		void write_bin_file(const char* data, unsigned int len, const char* fileName);
		int write_modulelog(int mid, int sid, int ifid, string& mip, string& sip, int retval, int result,
				int delay, int reservedInt0, int reservedInt1, int reqLen, int rspLen);

		int get_filesize(char* filename, unsigned int& size);
		bool need_disaster(char* fileName);
		int need_sample(int result);

		SocketClient *m_pSocketClient;
		Sampling *m_pSampling;

		pthread_rwlock_t m_errMsgrwlock;

		string m_appname;
		string m_errmsg; //last errmsg
		int m_rspMsgLen;
		int m_reqMsgHdrLen;

		int m_addrLen;
		bool m_needRsp;
		int8_t m_socketType;

		int m_iMid;

		bool m_isInit;

		unsigned short m_appnameLen;
		unsigned short m_appnamePartLen;

		int16_t m_not_sample_flag;

		//add by rockyyao
		//默认为true，加锁，属于线程安全；false，不加锁，非线程安全；
		bool m_needLock;

		//是否使用统计
		bool m_isStat;
		int8_t m_type;
		unsigned int m_stat_last_time;
		unsigned int m_stat_interval;
		unsigned int m_stat_count;
		map<string, int> m_cur_stat_log;
		map<string, map<string, int> > m_cur_stat_logs;

		//是否无dcagent
		bool m_noAgent;

		//无dcagent方式使用的cmlb
		uint16_t m_cmlbAppid;
		bool m_cmlbIsTest;
	};

};

#endif
