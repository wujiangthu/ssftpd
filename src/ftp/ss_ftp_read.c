/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_READ_C_
#define _SS_FTP_READ_C_


#include "ss_ftp_core.h"
#include "ss_ftp_read.h"


ngx_int_t ss_ftp_cmd_link_read(ngx_connection_t *c);
ngx_int_t ss_ftp_data_link_read(ngx_connection_t *c);
ngx_int_t ss_ftp_read_data(ngx_connection_t *c, ngx_buf_t *buf);


ngx_int_t
ss_ftp_cmd_link_read(ngx_connection_t *c)
{
   assert(c != NULL);
   ngx_int_t         rc;
   ss_ftp_request   *r;

   r = (ss_ftp_request *) c->data;
   assert(r != NULL);

   /*if (NULL == r->cmd_buf) {
      printf("%s\n", "ss_error in read data");
      return SS_ERROR;
   }*/
   assert(r->cmd_buf != NULL);
   rc = ss_ftp_read_data(c, r->cmd_buf);
   assert(rc > 0 || rc == SS_AGAIN || rc == SS_ERROR || rc == SS_BUF_FULL || rc == SS_TRANSFER_COMPLETE);

   /* All buffered data have been processed, and buffer becomes full */
   if (SS_BUF_FULL == rc) {
      /* TODO : save command to another buffer */
   }

   /* TODO  */
   /* just for passing compilation */
   return rc;
}

ngx_int_t
ss_ftp_data_link_read(ngx_connection_t *c)
{
//   printf("%s\n", "data link read");

   assert(c != NULL);
   ngx_int_t         rc;
   ss_ftp_send_receive_cmd *srcmd;
   ngx_buf_t        *buf;

   srcmd = c->send_receive_cmd;
   assert(srcmd->chain != NULL);
   buf = srcmd->chain->buf;
   assert(srcmd != NULL);
   assert(buf != NULL);
   rc = ss_ftp_read_data(c, buf);
   //printf("%s %d\n", "data rc", rc);
   assert(rc > 0 || rc == SS_AGAIN || rc == SS_TRANSFER_COMPLETE || rc == SS_ERROR || rc == SS_BUF_FULL);

   /* All buffered data have been processed, and buffer becomes full */
   if (SS_BUF_FULL == rc) {
      buf->pos = buf->start;
      buf->last = buf->start;

      rc = ss_ftp_read_data(c, buf);
printf("%d\n", rc);
      assert(rc > 0 || rc == SS_AGAIN || rc == SS_ERROR || rc == SS_TRANSFER_COMPLETE);
   }

   if (rc > 0 || rc == SS_AGAIN) {
      return rc;
   }

   /* Error */
   /* TODO : consider putting error handling here or throw to caller */
    return rc;
}

ngx_int_t
ss_ftp_read_data(ngx_connection_t *c, ngx_buf_t *buf)
{
   /* TODO : modify to make it suitable for both cmd and data link reading  */
   /* TODO : allocate more space if needed */

   assert(c != NULL);
   assert(buf != NULL);

   ngx_int_t           n, n_free;
   ngx_event_t        *rev;

   rev = c->read;

   n = buf->last - buf->pos;
   assert(n >= 0);
   if (n > 0) {
      return n;
   }

   n_free = buf->end - buf->last;
   assert(n_free >= 0);
   if (0 == n_free) {
      return SS_BUF_FULL;
   }

   if (rev->ready) {
      n = c->recv(c,
                  buf->last,
                  buf->end - buf->last);
   } else {
      n = SS_AGAIN;
   }
   if (SS_AGAIN == n) {
      ngx_handle_read_event(rev, 0);

      /* TODO : error handling  */

      return SS_AGAIN;
   }

   if (0 == n || SS_ERROR == n) {

      /* TODO : error handling  */

      return n;
   }

   buf->last += n;

   return n;
}


#endif /* _SS_FTP_READ_C_ */

