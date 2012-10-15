/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SSFTP_H_
#define _SSFTP_H_
  

#include "ss_ftp_core.h"

#define SS_FTP_SEND_CMD           1
#define SS_FTP_RECEIVE_CMD        2
#define SS_FTP_FILENAME_BUF_LEN   1024

#define SS_FTP_CLEAR                     1
#define SS_FTP_SAFE                      2
#define SS_FTP_CONFIDENTIAL              3
#define SS_FTP_PRIVATE                   4

#define SS_SEND_CHAIN             1
#define SS_SEND_FILE              2 

#define SS_UNLOGGED_IN            0  
#define SS_LOGGED_IN              1  

#define  SS_AGAIN                         NGX_AGAIN
#define  SS_ERROR                         NGX_ERROR
#define  SS_OK                            NGX_OK
#define  SS_TRANSFER_COMPLETE             0 
#define  SS_BUF_FULL                      -100 
#define SS_FTP_REQUEST_DEFAULT_POOL_SIZE  1024*8
#define SS_FTP_CMD_DEFAULT_BUF_LEN        1024
#define SS_FTP_CMD_DEFAULT_POOL_LEN        1024

#define assert(expr)                          \
   ((expr)                              \
   ? __ASSERT_VOID_CAST (0)                    \
   : __assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION))



typedef struct ss_ftp_request ss_ftp_request;
typedef ngx_int_t (*ss_ftp_process_during_data_transmittion)(ngx_connection_t *c);
typedef void (*ss_ftp_process_after_data_transmittion)(ngx_connection_t *c);

typedef struct ss_ftp_send_receive_cmd {
  ss_ftp_request    *request;

  ngx_connection_t  *data_connection;
  ngx_connection_t  *data_listen_connection;

  /* chain to be sent or reveived */
  ngx_chain_t       *chain;
  ngx_int_t          type; /* receive or send chain  */
  ngx_int_t          cmd_arrived : 1;
  ngx_int_t          data_link_send_type;

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
  ss_ftp_send_receive_cmd *send_receive_cmd;

  ngx_pool_t        *pool;
  /* pool for each command */
  ngx_pool_t        *cmd_pool;

  ngx_log_t         *log;

  char               expected_cmd[20];

  ngx_int_t          state;
  u_char            *cmd_name_start;
  u_char            *cmd_name_end;
  u_char            *cmd_arg_start;
  u_char            *cmd_arg_end;
  ngx_array_t       *cmd_args;
  ngx_buf_t         *cmd_buf;
  ngx_int_t          skipped_tel_chars;

  char              *username;
  char              *password;

  ngx_int_t          logged_in;

  ss_path_t          current_dir;
  ss_path_t         *rename_from_filename;
  ngx_chain_t       *cmd_link_write;
//  ngx_chain_t       *data_link_write;
  ngx_int_t          protection_level;

  void              *pamh;

} ss_ftp_request;


char *ss_ftp_home_dir_l;
char *ss_ftp_home_dir;


void ss_ftp_init_control_connection(ngx_connection_t *c);
void ss_ftp_cmd_link_write(ngx_event_t *send);
ngx_int_t ss_ftp_data_link_write(ngx_connection_t *c);
void ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
ngx_int_t ss_ftp_data_link_add_chain(ngx_connection_t *c, ngx_chain_t *chain);
void ss_ftp_init_data_connection(ngx_connection_t *c);

#endif /* _SSFTP_H_ */
