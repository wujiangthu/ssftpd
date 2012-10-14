/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_WRITE_H_
#define _SS_FTP_WRITE_H_


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

#define  SS_READ_FILE_AGAIN  -20
#define  SS_WRITE_SOCK_AGAIN -21
#define  SS_WRITE_FILE_AGAIN -22



void ss_ftp_cmd_link_write(ngx_event_t *send);
ngx_int_t ss_ftp_data_link_send(ngx_connection_t *c);
ngx_int_t ss_ftp_data_link_send_chain(ngx_connection_t *c, ngx_chain_t *chain);
ngx_int_t ss_ftp_data_link_send_file(ngx_connection_t *c, ngx_chain_t *chain);
void ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
ngx_int_t ss_ftp_data_link_add_chain(ngx_connection_t *c, ngx_chain_t *chain);
ngx_int_t ss_ftp_write(ngx_connection_t *c, ngx_chain_t *current_chain, ngx_chain_t *in);


#endif /* _SS_FTP_WRITE_H_ */

