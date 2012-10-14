/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_WRITE_C_
#define _SS_FTP_WRITE_C_


#include "ss_ftp_core.h"
#include "ss_ftp_write.h"


void ss_ftp_cmd_link_write(ngx_event_t *send);
ngx_int_t ss_ftp_data_link_send(ngx_connection_t *c);
ngx_int_t ss_ftp_data_link_send_chain(ngx_connection_t *c, ngx_chain_t *chain);
ngx_int_t ss_ftp_data_link_send_file(ngx_connection_t *c, ngx_chain_t *chain);
ngx_int_t ss_read_from_file(int fd, ngx_buf_t *buf, ngx_log_t *log);
void ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
ngx_int_t ss_ftp_data_link_add_chain(ngx_connection_t *c, ngx_chain_t *chain);
ngx_int_t ss_ftp_write(ngx_connection_t *c, ngx_chain_t *current_chain, ngx_chain_t *in);


void
ss_ftp_cmd_link_write(ngx_event_t *send)
{
   ngx_connection_t  *c;
   ss_ftp_request    *r;
   ngx_int_t         rc;

   c = (ngx_connection_t *) send->data;
   assert(c != NULL);
   r = (ss_ftp_request *) c->data;
   assert(r != NULL);

   rc = ss_ftp_write(c, r->cmd_link_write, NULL);
   assert(rc == NGX_AGAIN || rc == NGX_OK);
   if (NGX_AGAIN == rc) {
      ngx_handle_write_event(send, 0);

   /* Data transmited ok. */
   } else {
      /* TODO  */
     // ngx_del_event();    
   }
}

ngx_int_t
ss_ftp_data_link_send(ngx_connection_t *data_conn)
{
   assert(data_conn != NULL);
   assert(data_conn->data != NULL);

   ss_ftp_send_receive_cmd *srcmd;
   ngx_chain_t             *chain;
   ngx_int_t                send_type;

   srcmd = data_conn->send_receive_cmd;
   assert(NULL != srcmd);
   chain = srcmd->chain;

   if (chain == NULL) {
      return NGX_OK;
   }

   send_type = srcmd->data_link_send_type;
   if (SS_SEND_CHAIN == send_type) {
      return ss_ftp_data_link_send_chain(data_conn, chain);

   } else {

     assert(SS_SEND_FILE == send_type);
     return ss_ftp_data_link_send_file(data_conn, chain);
   }
}

ngx_int_t
ss_ftp_data_link_send_chain(ngx_connection_t *data_conn, ngx_chain_t *chain)
{
   assert(NULL != data_conn);

   return ss_ftp_write(data_conn, chain, NULL);
}

ngx_int_t 
ss_ftp_data_link_send_file(ngx_connection_t *c, ngx_chain_t *chain)
{
   assert(NULL != c);

   ngx_buf_t                **buf;
   ngx_int_t                  n;
   int                        fd;
   ss_ftp_send_receive_cmd   *srcmd;
   ss_ftp_request            *r;

   r = (ss_ftp_request *) c->data;
   buf = &chain->buf;
   if (NULL == *buf) {

      *buf = ngx_create_temp_buf(c->pool, 1024);
      if (NULL == *buf) {
         ss_ftp_process_out_of_memory(r);
         return NGX_ERROR;
      }
   }

   srcmd = c->send_receive_cmd;
   fd = srcmd->fd_retr;
   for ( ;; ) {
   
       /* buffer is full */
       if ((*buf)->end - (*buf)->last == 0) {
          
          /* all bytes have been processed */
          if ((*buf)->pos == (*buf)->last) {
             (*buf)->pos = (*buf)->start;
             (*buf)->last = (*buf)->start;
          }
       }

       n = ss_read_from_file(fd, *buf, c->log);       
       if (NGX_AGAIN == n) {
          return SS_READ_FILE_AGAIN;
       }

       if (NGX_OK == n || NGX_ERROR == n) {
          return n;
       }
    
       assert(n > 0);
       n = c->send(c, (*buf)->pos, (*buf)->last - (*buf)->pos);  
       if (NGX_ERROR == n) {
          return NGX_ERROR;
       }       
  
       if (NGX_AGAIN == n) {
          return SS_WRITE_SOCK_AGAIN;
       }

       assert(n > 0);
       (*buf)->pos += n;
   }
}

ngx_int_t
ss_read_from_file(int fd, ngx_buf_t *buf, ngx_log_t *log)
{
   assert(fd >= 0);
   assert(NULL != buf);

   ngx_int_t    n;
   ngx_int_t    n_free;
   ngx_int_t    error_num;

   n = buf->last - buf->pos;
   assert(n >= 0);   
   if (n > 0) {
      return n;
   }

   n_free = buf->end - buf->last;
   assert(n_free >= 0);
   if (0 == n_free) {

      /* all bytes have been processed */
       buf->pos = buf->start;
       buf->last = buf->start;
   }

   n = read(fd, buf->last, buf->end - buf->last);
   if (0 == n) {
      return NGX_OK;
   }

   if (-1 == n) {
      error_num = errno;
      if (EAGAIN == error_num) {
         return NGX_AGAIN;
      }
 
      ngx_log_debug2(NGX_LOG_DEBUG_FTP, log, 0, "read from file fd: %d failed, errno is %d", fd, error_num);
      return NGX_ERROR;
   }

   buf->last += n;

   return n; 
}

void
ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain)
{
   assert(r != NULL);

   ngx_event_t  *send;
   ngx_connection_t *conn;
   ngx_int_t     rc;

   conn = (ngx_connection_t *) r->connection;
   rc = ss_ftp_write(conn, r->cmd_link_write, chain);
   assert(rc == NGX_AGAIN || rc == NGX_OK || rc == NGX_ERROR);
   if (NGX_AGAIN == rc) {
      send = conn->write;
      ngx_handle_write_event(send, 0);

   /* Data transmited ok. */
   } else if (NGX_OK == rc){
      /* TODO  */
      //ngx_del_event();    
   } else {
     assert(rc == NGX_ERROR);
     /* TODO : error handling, such as peer shut down */
     ngx_log_debug0(NGX_LOG_DEBUG_FTP, conn->log, 0, "ftp:write error");
   }
}

ngx_int_t
ss_ftp_data_link_add_chain(ngx_connection_t *data_conn, ngx_chain_t *chain)
{
   assert(data_conn != NULL);

   ss_ftp_send_receive_cmd *srcmd;
   srcmd = data_conn->send_receive_cmd;
   assert(srcmd != NULL);

   return ss_ftp_write(srcmd->data_connection, srcmd->chain, chain);
}

ngx_int_t
ss_ftp_write(ngx_connection_t *c, ngx_chain_t *current_chain, ngx_chain_t *in)
{
   assert(c != NULL);

   ngx_chain_t       **ll;
   ngx_chain_t       **fl;
   ngx_chain_t       *temp;
   ngx_chain_t       *temp2;
   ngx_chain_t       *chain;
   ngx_int_t          ctype;
   ss_ftp_request    *r;

   r = (ss_ftp_request *) c->data;
   ctype = c->ftp_conn_type;
   assert(SS_FTP_CONTROL_CONN == ctype || SS_FTP_DATA_CONN == ctype);

   if (SS_FTP_CONTROL_CONN == ctype) {
      ll = &r->cmd_link_write;

   } else {
      ll = &((ss_ftp_send_receive_cmd *) (c->send_receive_cmd))->chain;
   }

   fl = ll;

   for (temp = current_chain; temp; temp = temp->next) {
       ll = &temp->next;
   }

   *ll = in;

   chain = c->send_chain(c, *fl, 0);
   if (NGX_CHAIN_ERROR == chain) {
       return NGX_ERROR;
   }

   for (temp = *fl; temp && temp != chain; /* void */){
       temp2 = temp->next;
       ngx_free_chain(r->pool, temp);
       temp = temp2;
   }

   *fl = temp;
   /* All chains have been sent */
   if (NULL == *fl) {
      return NGX_OK;
   }

   /* check whether it is suitable to put it here  */
   /* error handling  */

   return NGX_AGAIN;
}


#endif /* _SS_FTP_WRITE_C_ */

