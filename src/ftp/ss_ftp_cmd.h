
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_COMMAND_H_
#define _SS_FTP_COMMAND_H_


#include "ss_ftp_core.h"


#define OPEN_DIR_ERROR          -1
#define STAT_ERROR              -2
#define OUT_OF_MEMORY           -3
#define SS_FTP_FILE_NOT_FOUND   -4
#define SS_FTP_OK                1

typedef struct ss_ftp_request ss_ftp_request;

typedef struct ss_ftp_command {
  ngx_str_t   name;
  ngx_uint_t  type;
  void        (*execute)(ss_ftp_request *r);
} ss_ftp_command;

#define ss_null_command { ngx_string(""),  \
                          NGX_CONF_NOARGS, \
                          NULL }

ngx_hash_t *ss_ftp_cmds_hash_table;

ngx_int_t ss_ftp_create_commands_hash_table(ngx_pool_t *pool);
void ss_ftp_undefined_cmd(ss_ftp_request *r);

#endif /* _SS_FTP_COMMAND_H_ */
