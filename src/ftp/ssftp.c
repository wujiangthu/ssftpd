/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SSFTP_C_
#define _SSFTP_C_
   

#include "ss_ftp_core.h"
#include <ctype.h>


#define  SS_AGAIN                         NGX_AGAIN
#define  SS_ERROR                         NGX_ERROR
#define  SS_OK                            NGX_OK
#define  SS_TRANSFER_COMPLETE             0 
#define  SS_BUF_FULL                      -100 
#define SS_FTP_REQUEST_DEFAULT_POOL_SIZE  1024*8
#define SS_FTP_CMD_DEFAULT_BUF_LEN        1024

#define assert(expr)                          \
   ((expr)                              \
   ? __ASSERT_VOID_CAST (0)                    \
   : __assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION))


char ss_ftp_home_dir_l_bak[] = "\"/tmp/ftp\"";
char ss_ftp_home_dir_bak[] = "/tmp/ftp/";


void ss_ftp_init_control_connection(ngx_connection_t *c);
static void ss_ftp_init_request(ngx_event_t *rev);
void ss_ftp_process_cmds(ngx_event_t *rev);
static ngx_int_t ss_ftp_process_command(ss_ftp_request *r); 
void ss_ftp_init_data_connection(ngx_connection_t *c);
void ss_ftp_data_conn_set_handler(ngx_connection_t *c);
static void ss_ftp_process_data_link_read(ngx_event_t *rev);
static void ss_ftp_process_data_link_write(ngx_event_t *rev);


void 
ss_ftp_init_control_connection(ngx_connection_t *c)
{
  dup2(stand_err, STDERR_FILENO);

  assert(c != NULL);
  assert(SS_FTP_CONTROL_CONN == c->ftp_conn_type);

  ss_ftp_home_dir_l = ss_ftp_home_dir_l_bak;
  ss_ftp_home_dir = ss_ftp_home_dir_bak;

  ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->pool->log, 0, "ftp:init control conn");  
  ngx_event_t *rev;
  ngx_event_t *send;

  rev = c->read;
  send = c->write;
  rev->handler = ss_ftp_init_request;
  send->handler = ss_ftp_cmd_link_write;

  c->cmd_link_read = ss_ftp_cmd_link_read;

  ngx_handle_read_event(rev, 0);
  ngx_handle_write_event(send, 0);

  ss_ftp_init_request(rev);
}

static void
ss_ftp_init_request(ngx_event_t *rev)
{

  ngx_connection_t  *c;
  ngx_int_t          rc;
  ss_ftp_request    *r;
  ngx_int_t          home_dir_len;

  c = rev->data;
  assert(c != NULL && c->pool != NULL);
  ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->log, 0, "ftp:init ftp request");  
   
  r = ngx_pcalloc(c->pool, sizeof(ss_ftp_request));
  c->data = r;

  r->pool = ngx_create_pool(SS_FTP_REQUEST_DEFAULT_POOL_SIZE, c->log);
  /* TODO : error handling  */

  rc = ss_ftp_create_commands_hash_table(c->pool); 
  assert(OUT_OF_MEMORY == rc || NGX_OK == rc);
  if (OUT_OF_MEMORY == rc) {
     ss_ftp_process_out_of_memory(r);
     return;
  }
  
  r->connection = c;
  r->state = 0;
  r->log = c->log;
  r->protection_level = SS_FTP_CLEAR;
  /* The first command received must be "user" */
  strncpy(r->expected_cmd, "user", sizeof("user"));
  r->cmd_buf = ngx_create_temp_buf(r->pool, SS_FTP_CMD_DEFAULT_BUF_LEN); 
  r->cmd_link_write = ngx_pcalloc(r->pool, sizeof(ngx_chain_t)); 
  r->cmd_link_write->buf = ngx_create_temp_buf(r->pool, SS_FTP_CMD_DEFAULT_BUF_LEN); 
  r->cmd_link_write->next = NULL;

  /* TODO : choose a better size  */
  home_dir_len = strlen(ss_ftp_home_dir);
  r->current_dir.path  = ngx_pcalloc(r->pool, SS_FTP_FILENAME_BUF_LEN);
  r->current_dir.plen  = home_dir_len;
  r->current_dir.psize = home_dir_len + 1;
  ngx_memcpy(r->current_dir.path, ss_ftp_home_dir, r->current_dir.psize);

ngx_cycle_t volatile *cy = ngx_cycle;
  ss_ftp_conf_ctx_t *mcf = (ss_ftp_conf_ctx_t *) ((void **) cy->conf_ctx)[ss_ftp_module.index];
  printf("%s\n", (char *) (((ss_ftp_core_main_conf_t *) (mcf->main_conf[ss_ftp_core_module.ctx_index]))->welcome_message.data));

  ss_ftp_core_main_conf_t *mmcf = mcf->main_conf[ss_ftp_core_module.ctx_index];
  ngx_str_t str = mmcf->welcome_message;
  char *wel_msg = ngx_pcalloc(r->pool, str.len + 1);
  strncpy(wel_msg, (char *)str.data, str.len);
  wel_msg[str.len] = '\0';

  ss_ftp_reply(r, SERVICE_READY, wel_msg);
  //ss_ftp_reply(r, SERVICE_READY, SERVICE_READY_M);

  rev->handler = ss_ftp_process_cmds;
  ss_ftp_process_cmds(rev);
}

void 
ss_ftp_process_cmds(ngx_event_t *rev)
{
  ngx_int_t         rc;
  ngx_connection_t  *c;
  ss_ftp_request    *r;

  c = (ngx_connection_t *) rev->data;
  assert(c != NULL && c->data != NULL);
  r = c->data; 

  for ( ;; ) {

      if (-1 == c->fd || CONTROL_CONN_CAN_BE_CLOSED == c->to_be_closed) {
         return;
      }

      rc = c->cmd_link_read(c);
      assert(SS_AGAIN == rc || SS_ERROR == rc || rc >= 0);
      
      if (SS_AGAIN == rc) {
          return; 
      }

      if (SS_ERROR == rc) {
printf("%s\n", "ss error");
        /* TODO : stop, not return  */
          return; 
      }

      //r->cmd_args = ngx_array_create(r->pool, 10, sizeof(ngx_str_t));
      rc =  ss_ftp_parse_command(r);      
      assert(SS_AGAIN == rc || SS_FTP_INVALID_COMMAND == rc || SS_OK == rc || SS_FTP_PARSE_ONE_COMMAND_DONE == rc);

      
      if (SS_AGAIN == rc) {
          continue; 
      }

     /* TODO :error handling */

      if (SS_FTP_INVALID_COMMAND == rc) {

          /*  TODO : hanlde invalid command */ 

         return;
      }

      rc = ss_ftp_process_command(r);
      /* TODO : decide a better place to put it*/
      //r->cmd_args = ngx_array_create(r->pool, 10, sizeof(ngx_str_t));
  }
}

static ngx_int_t
ss_ftp_process_command(ss_ftp_request *r) 
{
   assert(r != NULL);

   ss_ftp_command *sfcmd;
   ngx_uint_t     key;
   u_char         *cmd_name;
   ngx_int_t      cmd_len;

   cmd_name = r->cmd_name_start;
   cmd_len  = r->cmd_name_end - r->cmd_name_start +1;
   assert(cmd_name != NULL && cmd_len > 0);

   key = ngx_hash_key_lc(cmd_name, cmd_len);
   ss_to_lower(cmd_name, cmd_len);
   sfcmd = (ss_ftp_command *) ngx_hash_find(ss_ftp_cmds_hash_table,
                                            key,
			                    cmd_name,
			                    cmd_len);

   if (NULL != sfcmd) {
      /* A PASV or PORT command arrived before */
      if (NULL != r->send_receive_cmd) {
          r->send_receive_cmd->cmd = sfcmd; 
      }

      if (strncmp(r->expected_cmd, "any", sizeof("any")) == 0 || strncmp(r->expected_cmd, (const char *) cmd_name, cmd_len) == 0) {

         sfcmd->execute(r);

         return NGX_OK; 
      }

      ss_ftp_reply(r, "503", "Bad sequence of commands");

   } else {
      ss_ftp_undefined_cmd(r);  
   }
  
   return NGX_OK;

   /* handler return value  */
} 

void 
ss_ftp_init_data_connection(ngx_connection_t *c) 
{
   assert(c != NULL);
   assert(SS_FTP_DATA_CONN == c->ftp_conn_type);

   ss_ftp_request *r;
   ss_ftp_send_receive_cmd **r_srcmd;
   ngx_connection_t        *cc;

   r = (ss_ftp_request *) c->data;
   assert(r != NULL);
   r->send_receive_cmd->data_connection = c;
   r_srcmd = &r->send_receive_cmd;
   assert(*r_srcmd != NULL);
   /* Corresponding command have't come yet */
   if (false == (*r_srcmd)->cmd_arrived) {
      ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->pool->log, 0, "ftp:command not come yet");  
      return;
   }

   cc = r->connection;
   cc->num_data_conns++;
   assert(cc->num_data_conns ==1);

   c->send_receive_cmd = *r_srcmd; 
   *r_srcmd = NULL; 

   ngx_ssl_t   ssl;
   if (SS_FTP_CLEAR != r->protection_level) {

      if (ss_ftp_ssl_create(r, &ssl) != NGX_OK) {
         return;
      }

      if (ss_ftp_ssl_certificate(r, &ssl) != NGX_OK) {
         return;
      }

      if (ss_ftp_ssl_create_connection(c, &ssl) != NGX_OK) {
         return;
      }

      c->ssl->handler = ss_ftp_data_conn_set_handler;
      c->ssl->buffer = 0;

     ngx_ssl_handshake(c);
/*
     int rc =  ngx_ssl_handshake(c);
     printf("%d\n", rc);
     while (rc == NGX_AGAIN) {

     rc =  ngx_ssl_handshake(c);
     
     printf("%d\n", rc);
    }
     ss_ftp_data_conn_set_handler(c);
*/
      return;
   }

   ss_ftp_data_conn_set_handler(c);
}

void
ss_ftp_data_conn_set_handler(ngx_connection_t *c)
{
   assert(NULL != c);

   ss_ftp_send_receive_cmd *dc_srcmd;
   ngx_event_t *rev;
   ngx_event_t *send; 
  
   dc_srcmd = c->send_receive_cmd;
   assert( dc_srcmd->type == SS_FTP_RECEIVE_CMD
        || dc_srcmd->type == SS_FTP_SEND_CMD);

   if (SS_FTP_RECEIVE_CMD == dc_srcmd->type) {

      rev = c->read;
      rev->handler = ss_ftp_process_data_link_read;
     // ngx_handle_read_event(rev, 0);
      ss_ftp_process_data_link_read(c->read);

   } else {

     send = c->write;
     send->handler = ss_ftp_process_data_link_write;

     /* For send file, not send_file()  */
     rev = c->read;
     rev->handler = ss_ftp_process_data_link_write;
     //ngx_handle_write_event(send, 0);
     ss_ftp_process_data_link_write(c->write);
   }
}

static void 
ss_ftp_process_data_link_read(ngx_event_t *rev) 
{
   ngx_int_t          rc;
   ngx_int_t          n;
   int                file_fd;
   int                fd_bak;
   ngx_connection_t  *data_conn;
   ss_ftp_send_receive_cmd  *srcmd;

   data_conn = (ngx_connection_t *) rev->data;
   assert(SS_FTP_DATA_CONN == data_conn->ftp_conn_type);
   srcmd = (ss_ftp_send_receive_cmd *) data_conn->send_receive_cmd;
   file_fd = srcmd->fd_stor;

   for ( ;; ) {

     rc = ss_ftp_data_link_read(data_conn);
     assert(rc == NGX_AGAIN || rc == SS_ERROR || rc == SS_TRANSFER_COMPLETE || rc > 0);

     if (NGX_AGAIN == rc) {
        ngx_handle_read_event(rev, 0); 
        return;
     }

     if (SS_ERROR == rc) {
        /* error handling */
        ngx_log_debug0(NGX_LOG_DEBUG_FTP, data_conn->log, 0, "read data link error");
        return;
     }

     if (rc > 0) {

        if (NULL == srcmd->process) {
           continue;
        }

        n = srcmd->process(data_conn);
        if (NGX_ERROR == n) {
           /* ERROR handling */
    
           return;
        }

        if (NGX_OK == n) {
           continue;
        }
        
        if (SS_WRITE_FILE_AGAIN == n) {
           fd_bak = data_conn->fd;
           data_conn->fd = file_fd;
           ngx_handle_write_event(data_conn->write, 0);
           data_conn->fd = fd_bak;

           return;
        }
     }

     if (SS_TRANSFER_COMPLETE == rc) {
        assert(NULL != srcmd->clean_up);
        srcmd->clean_up(data_conn);
        return;
     }
   }
}

static void 
ss_ftp_process_data_link_write(ngx_event_t *send) 
{
   ngx_int_t                 rc;
   ngx_connection_t         *data_conn;
   ss_ftp_send_receive_cmd  *srcmd;
   ss_ftp_request           *r;
   int                       file_fd;
   int                       fd_bak;

   data_conn = (ngx_connection_t *) send->data;
   assert(SS_FTP_DATA_CONN == data_conn->ftp_conn_type);
   srcmd = (ss_ftp_send_receive_cmd *) data_conn->send_receive_cmd;
   file_fd = srcmd->fd_retr;
   r = srcmd->request;

   rc = ss_ftp_data_link_send(data_conn);
   assert(rc == NGX_AGAIN || rc == SS_READ_FILE_AGAIN || rc == SS_WRITE_SOCK_AGAIN || rc == NGX_OK || rc == NGX_ERROR);

   if (NGX_ERROR == rc) {
printf("%s\n", "nginx errorrrrrrrrr");
      ngx_log_debug0(NGX_LOG_DEBUG_FTP, data_conn->log, 0, "ftp:write data link error");
      ss_ftp_reply(r, "451", "Server error in processing");
      return;
   }

   if (SS_READ_FILE_AGAIN == rc) {
      fd_bak = data_conn->fd;
      data_conn->fd = file_fd;
      ngx_handle_read_event(data_conn->read, 0);
      data_conn->fd = fd_bak;

      return;
   }

   if (SS_WRITE_SOCK_AGAIN == rc) {
      ngx_handle_write_event(data_conn->write, 0);
      return;
   }

   if (NGX_AGAIN == rc) {
      /* For sending without ssl */
      if (NULL != srcmd->process) {
         srcmd->process(data_conn);
      }

      ngx_handle_write_event(data_conn->write, 0); 
      return;
   }

   assert(NGX_OK == rc);

   if (srcmd->request->protection_level != SS_FTP_CLEAR) {
      SSL_shutdown(data_conn->ssl->connection);
   }

   assert(NULL != srcmd->clean_up);
   srcmd->clean_up(data_conn);
}


#endif /* _SSFTP_C */
