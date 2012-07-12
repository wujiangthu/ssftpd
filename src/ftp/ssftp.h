
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SSFTP_H_
#define _SSFTP_H_
  

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include "ss_ftp_cmd.h"

#define SS_FTP_SEND_CMD     1
#define SS_FTP_RECEIVE_CMD  2


typedef struct ss_ftp_request ss_ftp_request;
typedef void (*ss_ftp_process_during_data_transmittion)(ngx_connection_t *c);
typedef void (*ss_ftp_process_after_data_transmittion)(ngx_connection_t *c);

typedef struct ss_ftp_send_receive_cmd {
  ss_ftp_request    *request;

  ngx_connection_t  *data_connection;
  ngx_connection_t  *data_listen_connection;

  /* chain to be sent or reveived */
  ngx_chain_t       *chain;
  ngx_int_t          type; /* receive or send chain  */

  ngx_buf_t         *read_buf;

  ss_ftp_command    *cmd;
  /* file descrptor for file opened in retrieve commmand */
  ngx_int_t          fd_retr;
  ngx_int_t          fd_stor;

  ss_ftp_process_during_data_transmittion process; 
  ss_ftp_process_after_data_transmittion  clean_up;

} ss_ftp_send_receive_cmd; 

typedef struct ss_ftp_request {
  /* ftp control connection  */
  ngx_connection_t  *connection;
  /* ftp data connection  */
  //ngx_connection_t  *data_connection;
  ss_ftp_send_receive_cmd *send_receive_cmd;

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
//  ngx_chain_t       *data_link_write;
} ss_ftp_request;


char ss_ftp_home_dir_l[] = "\"/home/usher/ftp\"";
char ss_ftp_home_dir[] = "/home/usher/ftp/";

void ss_ftp_init_control_connection(ngx_connection_t *c);
void ss_ftp_cmd_link_write(ngx_event_t *send);
ngx_int_t ss_ftp_data_link_write(ngx_connection_t *c);
void ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
ngx_int_t ss_ftp_data_link_add_chain(ngx_connection_t *c, ngx_chain_t *chain);
void ss_ftp_init_data_connection(ngx_connection_t *c);

#endif /* _SSFTP_H_  */
