#ifndef HIDE_PRIVATE_TP
#define HIDE_PRIVATE_TP
#include "atomic.h"

#define ALL_MAX_PROC 2001

typedef struct mg_proc_info {
    int id;
    int groupid;
    int pid;
    int state;  //0: read ，1: process
    int curr_seq;
    int check_count;
    int last_check_sep;
    int tmsp;
    int heartbt;
    char name[64];

} MG_PROC_INFO;

typedef MG_PROC_INFO MON_PROC_INFO;

#endif
