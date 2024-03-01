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

	const int8_t LT_NORMAL = 0; // һ�������ϱ���ͨ��0��

	const int STAT_TYPE_COUNT = 0; //ͳ��count
	const int STAT_TYPE_SUM = 1; //ͳ��sum
	const int STAT_TYPE_MIN = 2; //ͳ��min
	const int STAT_TYPE_MAX = 3; //ͳ��max

	class CLogger {
	public:
		enum ProType {
			PRO_STRING = 0,
			PRO_BINARY = 1,
		};

/*-----------------------------------���캯��--------------------------------------*/
		/******************************************
		 ���ƣ����캯������DCAgent��
		 ������sockettype, �ϱ�ʹ�õ�ͨѶЭ��
			 0: unix domain socket
			 1: tcp
			 2: udp
		 	isLock, �Ƿ�������������̹߳���һ��clogger�����������������Բ��ӡ�
		 ���أ���
		 ˵����ͨ�����ֱ��ʹ��Ĭ�ϲ���
		 *******************************************/
		CLogger(int8_t socketType = 0, bool isLock = true);

		/******************************************
		 ���ƣ����캯������DCAgent��
		 ������isTest, �Ƿ�����ϱ�
		 	cmlbAppid, cmlb��appid������ֵ����ѯlphelper��DC������
		 	isLock, �Ƿ�������������̹߳���һ��clogger�����������������Բ���
		 ���أ���
		 ˵�����˹��캯��Ϊ�����ް�װDCAgentר��
		 *******************************************/
		CLogger(uint16_t cmlbAppid, bool isTest, bool isLock);

		~CLogger();

/*----------------------------------��ʼ������--------------------------------------*/

		/******************************************
		 ���ƣ�init
		 ������logname, ���ӿ�ID��DCID��������DC�ӿ�����ҳ��ע��
		 ���أ�0 �ɹ�, ��0 ʧ��
		 ˵������ʼ��ҵ�������ϱ���ֻ�ܵ���һ��
		 *******************************************/
		int init(string logName);

		/******************************************
		 ���ƣ�init
		 ������mid, ����ID
		 ���أ�0 �ɹ�, ��0 ʧ��
		 ˵������ʼ��ģ�������ϱ���ֻ�ܵ���һ��
		 *******************************************/
		int init(int mid);

/*--------------------------------�����ϱ�����-----------------------------------------*/

		/*******************************************
		 ���ƣ�write_baselog(ͨ�����)
		 ������logType, �������ͣ�һ����0��LT_NORMAL����
			data, ���������ݣ�ҵ����Ҫֱ�Ӵ��ݸ�ʽΪ "k1=v1&k2=v2"������(v2�Ѿ���������)
			fallFlag    �ò����Ѳ��ܿ����Ƿ���أ��ɺ�����������
			timestamp   unixʱ�����Ĭ��ֵΪϵͳ��ǰʱ��
		 ���أ�0 �ɹ�, ��0 ʧ��
		 ˵���������ϱ�������һ��ҵ���ϱ���
		 ********************************************/
		int write_baselog(int8_t logType, const string& data,
				bool fallFlag = true, unsigned int timestamp = time(NULL));

		/*******************************************
		 ���ƣ�write_baselog
		 ������data, ���������ݣ�ҵ����Ҫֱ�Ӵ��ݸ�ʽΪ "k1=v1&k2=v2"������(v2�Ѿ���������)������������������(proType����Ϊ1)
		 	len, ���ݵĳ���
			logType, �������ͣ�һ����0��LT_NORMAL����
			proType, ��������
				0���ַ���
				1��������
			timestamp   unixʱ�����Ĭ��ֵΪϵͳ��ǰʱ��
		 ���أ�0 �ɹ�, ��0 ʧ��
		 ˵���������ϱ�������֧�ֶ�����PB��ʽ�ȶ������ϱ�
		 ********************************************/
		int write_baselog(const char* data, unsigned int len,
				int8_t logType = LT_NORMAL, int8_t proType = 0, unsigned int timestamp = time(NULL));

		/*******************************************
		 ����: write_baselog
		 ����: logType, �������ͣ�һ����0��LT_NORMAL����
		 	 timestamp, ����ʱ�����һ���ǰʱ���
		 	 fmt, �ϱ������ݣ���"key1=%s&key2=%s"
		 ����: 0 �ɹ�, ��0 ʧ��
		 ˵��: ��ʽ���ϱ����ݵĺ���
		 *******************************************/
		int write_baselog(int8_t logType, unsigned int timestamp, const char* fmt, ...);


		/*******************************************
		 ���ƣ�write_modulelog
		 ������	sid, ����ID
		 	ifid, �ӿ�ID
		 	mip, ����IP
		 	sip, ����IP
		 	retval, ����ֵ
		 	result, ���ý��
		 	delay, ������ʱ
		 ����: 0 �ɹ�, ��0 ʧ��
		 ˵����ģ�������ϱ�����
		 ********************************************/
		int write_modulelog(int sid, int ifid, string& mip, string& sip, int retval, int result, int delay);

		//�������Ͻӿڣ���������fip����ԴIP, reqLen���������С(ѡ��)��rspLen����Ӧ����С(ѡ��)
		int write_modulelog(int sid, int ifid, string& fip, string& mip, string& sip, int retval, int result, int delay,
				int reqLen = 0, int rspLen = 0);

		//�������Ͻӿڣ���������ID���룬�������ʻ����initʱ�������ID
		int write_modulelog(int mid, int sid, int ifid, string& fip, string& mip, string& sip, int retval, int result, int delay,
				int reqLen = 0,	int rspLen = 0);


		/*******************************************
		 ���ƣ�write_statlogs
		 ������ data, ���������ݣ�ҵ����Ҫֱ�Ӵ��ݸ�ʽΪ "k1=v1&k2=v2"������(v2�Ѿ���������)��������ͳ���ֶΣ��൱��ά��
			statistics, ��ͳ�Ƶ��ֶΣ�keyΪָ�꣬valueΪָ��ֵ
			stattype, ͳ������
				STAT_TYPE_COUNT����ָ�����
				STAT_TYPE_SUM����ָ���ۼ�
				STAT_TYPE_MIN��ָ����Сֵ
				STAT_TYPE_MAX��ָ�����ֵ
			timestamp, unixʱ�����Ĭ��ֵΪϵͳ��ǰʱ��
		 ����: 0 �ɹ�, ��0 ʧ��
		 ˵���������ϱ�����ͳ�ƣ�����ǰ�������set_stat(true)
		 ********************************************/
		int write_statlogs(const string& data, map<string, int>& statistics, int stattype, unsigned int timestamp = time(NULL));


/*-----------------------------------һ�㺯��--------------------------------------*/

		/******************************************
		 ����: set_socketype
		 ����: socketType, ͨѶЭ��
			  0: unix domain socket
			  1: tcp
			  2: udp
		 ����: ��
		 ˵��: ���� init ��������֮ǰ������Ч !Ҳ�����ڹ��캯��ʱ�趨
		 ******************************************/
		void set_socktype(int8_t st);

		/******************************************
		 ���ƣ�encode
		 ������dataSrc ԭ�ַ���
			dataDst ������ַ���
		 ���أ�0 �ɹ�, ��0 ʧ��
		 ˵�����ַ������뺯�������ϱ�����ֵ����=��&��\t��\n ʱ����Ҫ�ȱ��룬�������ݻ����
		 *******************************************/
		int encode(string& dataSrc, string& dataDst);

		/*******************************************
		 ���ƣ�set_stat
		 ������isStat, �Ƿ����ͳ���ϱ�
			logType, �������ͣ�һ����0��LT_NORMAL����
			interval, ͳ��ʱ������s�����������ֵ��������
			count, ͳ�ƺ������¼�����������ֵ�������ݣ����ֵ���ڴ�ʹ���йأ�
		 ���أ�0 �ɹ�, ��0 ʧ��
		 ˵����Ĭ���ϱ����ݲ�ͳ�ƣ����ô˺���ǰ�����ȵ���init������ʼ��
		 ********************************************/
		int set_stat(bool isStat, int8_t logType,
				unsigned int interval = 60, unsigned int count = 1000000);

		/*******************************************
		 ���ƣ�pack_send_buff
		 ������pBuff, �ⲿ�����ָ�룬����ɹ����ָ��һ����ڴ���Ϊ���ͻ�������ʹ�������Ҫ�ⲿdelete
		 	retLen, ����ɹ��󣬷��ط��ͻ������ĳ���
		 	data, ���������ݣ�ҵ����Ҫֱ�Ӵ��ݸ�ʽΪ "k1=v1&k2=v2"������(v2�Ѿ���������)
		 	len, ���ݵĳ���
		 	logtype, �����ϱ������ͣ�һ����0��LT_NORMAL����
		 	proType, ��������
		 		0:�ַ���
		 		1:������
		 	timestamp, unixʱ�����Ĭ��ֵΪϵͳ��ǰʱ��
		 ����: 0 �ɹ�, ��0 ʧ��
		 ˵�������ͻ�������������������ⲿ����ʵ���Լ���dcagentͨѶ,���سɹ����newһ���ڴ棬�������ⲿdelete
		 ********************************************/
		int pack_send_buff(char*& pBuff, int& retLen, const char* data, unsigned int len,
				int8_t logType = LT_NORMAL, int8_t proType = 0, unsigned int timestamp = time(NULL));

		/******************************************************
		 ����: set_no_sample
		 ����: result, ��������resultֵ����resultΪģ�������ϱ�write_modulelog�������result
		 ����: ��
		 ˵��: ����ĳ��ģ���ķ�������Ϊ������(ֻ��ģ������Ч��write_baselog��Ч)
		 *******************************************************/
		void set_no_sample(int result);

		/*****************************************************
		 ���ò������ο�set_no_sample, ֻ���ģ���ϱ���Ч������ĳ��ģ���ķ������������
		 *****************************************************/
		void set_sample(int result);

		/*******************************************
		 ���ƣ�get_errmsg
		 ��������
		 ���أ�������Ϣ
		 ˵�������ص���ʧ��ʱ�Ĵ�����Ϣ
		 ********************************************/
		string get_errmsg(void);

		/*******************************************
		 ���ƣ�get_logtype
		 ��������
		 ���أ���ǰ�������ϱ�������
		 ˵������
		 ********************************************/
		int8_t get_logtype() {return m_type;}

		/*******************************************
		 ���ƣ�get_lock
		 ��������
		 ���أ���ǰ�����Ƿ����
		 ˵������
		 ********************************************/
		bool get_lock() {return m_needLock;}


		//���ڸ���ͳ�ƴ洢��ÿ��mapΪn���ӵ�ͳ�����ݽ��
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
		//Ĭ��Ϊtrue�������������̰߳�ȫ��false�������������̰߳�ȫ��
		bool m_needLock;

		//�Ƿ�ʹ��ͳ��
		bool m_isStat;
		int8_t m_type;
		unsigned int m_stat_last_time;
		unsigned int m_stat_interval;
		unsigned int m_stat_count;
		map<string, int> m_cur_stat_log;
		map<string, map<string, int> > m_cur_stat_logs;

		//�Ƿ���dcagent
		bool m_noAgent;

		//��dcagent��ʽʹ�õ�cmlb
		uint16_t m_cmlbAppid;
		bool m_cmlbIsTest;
	};

};

#endif
