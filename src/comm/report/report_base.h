#ifndef __REPORT_BASE_H__
#define __REPORT_BASE_H__

#define REPORTFILE "../stat/spp_report.dat"
#define MAXNUMBER (1*10000*60)

enum {
	REPORT_FIELD_CMD        = 0x1,
	REPORT_FIELD_RESULT     = 0x2,
	REPORT_FIELD_LOCAL      = 0x4,
	REPORT_FIELD_REMOTE     = 0x8,
	REPORT_FIELD_COST       = 0x10,
	REPORT_FIELD_USR_DEF    = 0x20,
	REPORT_FIELD_FRM        = 0x40,
};

#pragma pack(1)
typedef struct _spp_report_data
{
	uint32_t     bitmaps;           // 内容字段
	uint8_t      cmd[32];           // 命令字
	int32_t      result;            // 返回码
	uint32_t     remote_ipv4;       // 远端ip
	uint32_t     remote_port;       // 远端端口
	uint32_t     local_ipv4;        // 本地ip
	uint32_t     local_port;        // 本地端口
	uint32_t     msg_cost;          // 时延信息
	int32_t      frm_err;           // 框架错误码
	uint32_t     usr_define;        // 用户定义
	uint8_t      reserved[80];      // 扩展字段
} report_handle_t;

#endif
