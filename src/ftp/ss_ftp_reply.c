
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_REPLY_C_
#define _SS_FTP_REPLY_C_



#include "ss_ftp_core.h"
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include "ssftp.h"
#include "ss_ftp_reply.h"
#include <assert.h>


void ss_ftp_reply(ss_ftp_request *r, const char *reply_code, char *reply_message);
void ss_ftp_reply_realpath_error(ss_ftp_request *r, int err_no);
void ss_ftp_reply_mkdir_error(ss_ftp_request *r, int err_no);
void ss_ftp_process_out_of_memory(ss_ftp_request *r);
void ss_ftp_close_connection(ngx_connection_t *c);


void
ss_ftp_reply(ss_ftp_request *r, const char *reply_code, char *reply_message)
{
   ngx_chain_t  *chain;

   chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t));
   if (NULL == chain) {
      ss_ftp_process_out_of_memory(r);
      /* TODO : caller need to be processed */
      return;
   }
   
   ngx_int_t reply_code_len = strlen(reply_code);
   ngx_int_t reply_message_len = strlen(reply_message);
   
   /* 3 extra characters: 1 space + \r + \n + terminating \0 */
   chain->buf = ngx_create_temp_buf(r->pool, 
                                    reply_code_len + reply_message_len + 4); 
   if (NULL == chain->buf) {
      ss_ftp_process_out_of_memory(r);
      /* TODO : caller need to be processed */
      return;
   }
   
   snprintf((char *) chain->buf->pos, 
            reply_code_len + reply_message_len + 4,
            "%s %s\r\n",
            reply_code,
            reply_message);

   /* leave terminating '\0' */
   chain->buf->last = chain->buf->last + reply_code_len + reply_message_len + 3; 

   ss_ftp_cmd_link_add_chain(r, chain);   
}

void 
ss_ftp_reply_realpath_error(ss_ftp_request *r, int err_no)
{
   assert(NULL != r);

   switch (err_no) {

   case EACCES:
      ss_ftp_reply(r, "550", "No access permission.");
      break;

   case ENAMETOOLONG:
      ss_ftp_reply(r, "500", "File name too long.");
      break;

   case ENOENT:
      ss_ftp_reply(r, "550", "File not exists.");
      break;

   case EINVAL:
   case EIO:
   case ELOOP:
      ss_ftp_reply(r, "550", "System error.");
      break;
   }

}

void 
ss_ftp_reply_mkdir_error(ss_ftp_request *r, int err_no) 
{
   assert(NULL != r);

   switch (err_no) {

   case EACCES:
      ss_ftp_reply(r, "550", "No access permission.");
      break;

   case EEXIST:
      ss_ftp_reply(r, "550", "Directory exists.");
      break;

   case ENOSPC:
      ss_ftp_reply(r, "550", "No enough disk room.");
      break;

   case EROFS:
      ss_ftp_reply(r, "550", "Parent directory read only.");
      break;

   case EMLINK:
      ss_ftp_reply(r, "550", "Parent directory has too many links (entries).");
      break;

   /* TODO : make it to be variable arguments */
/*
   default:
      ngx_log_debug2(NGX_LOG_DEBUG_FTP, r->connection->log, 0, "ftp:make directory %s failed, errno is unknow: %d", dir->path, errno);
      ss_ftp_reply(r, "550", "System error");
      break;
*/

   }
}

void 
ss_ftp_process_out_of_memory(ss_ftp_request *r) 
{
  assert(NULL != r);
 
  printf("%s\n", "out of memory");
  exit(1);
  ss_ftp_reply(r, STORAGE_INSUFFICIENT, STORAGE_INSUFFICIENT_M);
  ss_ftp_close_connection(r->connection); 
}
 
void 
ss_ftp_close_connection(ngx_connection_t *c)
{
  assert(NULL != c);

  ngx_connection_t *lc;
  ngx_connection_t *cc;
  ngx_int_t         fd;
  ngx_int_t         num_conn;
  ss_ftp_request   *r;

  if (SS_FTP_DATA_LISTEN_CONN == c->ftp_conn_type || SS_FTP_CONTROL_CONN == c->ftp_conn_type) {
     ngx_close_connection(c);
  }

  if (SS_FTP_DATA_CONN == c->ftp_conn_type) {
     lc = ((ss_ftp_send_receive_cmd *) c->send_receive_cmd)->data_listen_connection;
     assert(NULL != lc);
     ngx_close_connection(lc);

     fd = ((ss_ftp_send_receive_cmd *) c->send_receive_cmd)->fd_retr;
     if (fd >0) {
        printf("%s\n","****go to close fd for retr file");
        close(fd);
     }
     fd = ((ss_ftp_send_receive_cmd *) c->send_receive_cmd)->fd_stor;
     if (fd>0) {
        printf("%d\n",fd);
        printf("%s\n","****go to close fd for storing file");
        close(fd);
     }

     cc = ((ss_ftp_request *) (c->data))->connection; 
     num_conn = cc->num_data_conns;
     assert(num_conn > 0);
        printf("before num conn == %d\n", cc->num_data_conns);
     cc->num_data_conns--;
        printf("after num conn == %d\n", cc->num_data_conns);

     assert(0 == cc->num_data_conns);
     if (CONTROL_CONN_CAN_BE_CLOSED == cc->to_be_closed) {
        printf("%s\n","****before");
        r = (ss_ftp_request *) cc->data;
        ss_ftp_reply(r, CLOSING_CONTROL_CONNECTION, CLOSING_CONTROL_CONNECTION_M);
        ngx_close_connection(cc); 
        ngx_destroy_pool(r->pool);
      
        printf("%s\n","****after");
     }

     ngx_close_connection(c);
   }
}


#endif /* _SS_FTP_REPLY_C_  */
