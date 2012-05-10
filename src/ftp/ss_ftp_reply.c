
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


void ss_ftp_reply(ss_ftp_request *r, const char *reply_code, char *reply_message);


void
ss_ftp_reply(ss_ftp_request *r, const char *reply_code, char *reply_message)
{
   ngx_chain_t  *chain;

   chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t));
   
   ngx_int_t reply_code_len = strlen(reply_code);
   ngx_int_t reply_message_len = strlen(reply_message);
   
   /* 3 extra characters: 1 space + \r + \n + terminating \0 */
   chain->buf = ngx_create_temp_buf(r->pool, 
                                    reply_code_len + reply_message_len +4); 
   snprintf((char *) chain->buf->pos, 
            reply_code_len + reply_message_len + 4,
            "%s %s\r\n",
            reply_code,
            reply_message);

 //  strcpy((char *) chain->buf->pos, reply_code); 
  // chain->buf->pos[reply_code_len] = ' ';
   //strcpy((char *) &chain->buf->pos[reply_code_len +1], reply_message); 
  // chain->buf->pos[reply_code_len + 1 + reply_message_len] = '\r';
  // chain->buf->pos[reply_code_len + 1 + reply_message_len + 1] = '\n';

   /* leave terminating '\0' */
   chain->buf->last = chain->buf->last + reply_code_len + reply_message_len + 3; 

   ss_ftp_cmd_link_add_chain(r, chain);   
}


#endif /* _SS_FTP_REPLY_C_  */
