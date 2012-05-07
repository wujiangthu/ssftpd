
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_REPLY_C_
#define _SS_FTP_REPLY_C_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include "ssftp.h"
#include "ssftp.c"
#include "ss_ftp_reply.h"


void ss_ftp_reply(ss_ftp_request *r, const char *reply_code);


void
ss_ftp_reply(ss_ftp_request *r, const char *reply_code)
{
   ngx_chain_t  *chain;

   chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t));
   chain->buf = ngx_create_temp_buf(r->pool, strlen(reply_code) + 1); 
   strcpy((char *) chain->buf->pos, reply_code); 
//   chain->buf->pos[strlen(reply_code)] = ' ';
 //  chain->buf->pos[strlen(reply_code)] = '\r';
  // chain->buf->pos[strlen(reply_code)] = '\n';

   /* leave terminating '\0' */
   chain->buf->last += strlen(reply_code); 

   ss_ftp_cmd_link_add_chain(r, chain);   
}


#endif /* _SS_FTP_REPLY_C_  */
