
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SSFTP_H_
#define _SSFTP_H_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


typedef struct ss_ftp_request {
  /* ftp control connection  */
  ngx_connection_t  *connection;
  /* ftp data connection  */
  ngx_connection_t  *data_connection;

  ngx_pool_t        *pool;

  ngx_int_t          state;
  u_char            *cmd_name_start;
  u_char            *cmd_name_end;
  u_char            *cmd_arg_start;
  u_char            *cmd_arg_end;
  ngx_array_t       *cmd_args;
  ngx_buf_t         *cmd_buf;
  ngx_int_t          skipped_tel_chars;

  ngx_str_t         *current_dir;

  ngx_chain_t       *cmd_link_write;
  ngx_chain_t       *data_link_write;
} ss_ftp_request;


char ss_ftp_home_dir[] = "/home/usher/ftp/";

void ss_ftp_init_connection(ngx_connection_t *c);
void ss_ftp_cmd_link_write(ngx_event_t *send);
void ss_ftp_data_link_write(ngx_event_t *send);
void ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
void ss_ftp_data_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
void ss_ftp_init_data_connection(ngx_connection_t *c);

#endif /* _SSFTP_H_  */
