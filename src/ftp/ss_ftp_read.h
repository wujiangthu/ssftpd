/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_READ_H_
#define _SS_FTP_READ_H_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ctype.h>
#include <assert.h>
#include "ss_ftp_core.h"


#define  SS_AGAIN                         NGX_AGAIN
#define  SS_ERROR                         NGX_ERROR
#define  SS_OK                            NGX_OK
#define  SS_TRANSFER_COMPLETE             0 
#define  SS_BUF_FULL                      -100 
#define SS_FTP_REQUEST_DEFAULT_POOL_SIZE  1024*8
#define SS_FTP_CMD_DEFAULT_BUF_LEN        1024


ngx_int_t ss_ftp_cmd_link_read(ngx_connection_t *c);
ngx_int_t ss_ftp_data_link_read(ngx_connection_t *c);
ngx_int_t ss_ftp_read_data(ngx_connection_t *c, ngx_buf_t *buf);


#endif /* _SS_FTP_READ_H_ */
