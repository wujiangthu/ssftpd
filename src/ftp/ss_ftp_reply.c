
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_REPLY_C_
#define _SS_FTP_REPLY_C_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include "ssftp.h"
#include "ss_ftp_reply.h"
#include <assert.h>


void ss_ftp_reply(ss_ftp_request *r, const char *reply_code, char *reply_message);
void ss_ftp_process_insufficient_memory(ngx_connection_t *c);
void ss_ftp_close_connection(ngx_connection_t *c);


void
ss_ftp_reply(ss_ftp_request *r, const char *reply_code, char *reply_message)
{
   ngx_chain_t  *chain;

   chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t));
   
   ngx_int_t reply_code_len = strlen(reply_code);
   ngx_int_t reply_message_len = strlen(reply_message);
   
   /* 3 extra characters: 1 space + \r + \n + terminating \0 */
   chain->buf = ngx_create_temp_buf(r->pool, 
                                    reply_code_len + reply_message_len + 4); 
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
ss_ftp_process_insufficient_memory(ngx_connection_t *c) 
{
  assert(NULL != c);
 
  ss_ftp_request *r;

  r = (ss_ftp_request *) c->data;
  assert(NULL != r);
  ss_ftp_reply(r, STORAGE_INSUFFICIENT, STORAGE_INSUFFICIENT_M);
  ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->log, 0, "ftp:memory allocated failed");
  /*TODO : logging*/
  ss_ftp_close_connection(c); 

     
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
     //assert(0 != fd);
     if (0 != fd) {
        close(fd);
     }

     cc = ((ss_ftp_request *) (c->data))->connection; 
     num_conn = cc->num_data_conns;
     assert(num_conn > 0);
     cc->num_data_conns--;
     if (0 == cc->num_data_conns && CONTROL_CONN_CAN_BE_CLOSED == cc->to_be_closed) {
        r = (ss_ftp_request *) cc->data;
        ngx_close_connection(cc); 
        ngx_destroy_pool(r->pool);
     }
      
     ngx_close_connection(c);
}
}


#endif /* _SS_FTP_REPLY_C_  */
