/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_SSL_C_
#define _SS_FTP_SSL_C_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

#include "ss_ftp_core.h"


ngx_int_t ss_ftp_ssl_create(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_certificate(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_create_connection(ngx_connection_t *c, ngx_ssl_t *ssl);


ngx_int_t 
ss_ftp_ssl_create(ss_ftp_request *r, ngx_ssl_t *ssl)
{
   assert(NULL != r);
   assert(NULL != ssl);

   if (ngx_ssl_create(ssl, 14, NULL) != NGX_OK) {
      /* make "14" to be more elegant */
      ngx_log_debug(NGX_LOG_DEBUG_FTP, r->connection->log, 0, "ftp:ngx_ssl_create error");
      ss_ftp_reply(r, "451", "Server error in processing");
      return NGX_ERROR;
   }
 
   return NGX_OK;
}

ngx_int_t 
ss_ftp_ssl_certificate(ss_ftp_request *r, ngx_ssl_t *ssl)
{
   assert(NULL != r);
   assert(NULL != ssl);

   ngx_conf_t  cf;
   //ngx_str_t   cert;
   //ngx_str_t   key;

   cf.cycle = *cycle_g;
   ngx_str_t cert = ngx_string("/home/usher/ftp/ssl/server.crt");
   ngx_str_t key  = ngx_string("/home/usher/ftp/ssl/server.key");
   if (ngx_ssl_certificate(&cf, ssl, &cert, &key) != NGX_OK) {

      ngx_log_debug(NGX_LOG_DEBUG_FTP, r->connection->log, 0, "ftp:ngx_ssl_certificate error");
      ss_ftp_reply(r, "451", "Server error in processing");
      return NGX_ERROR;
   }
 
   return NGX_OK;
}

ngx_int_t 
ss_ftp_ssl_create_connection(ngx_connection_t *c, ngx_ssl_t *ssl)
{
   assert(NULL != c);
   assert(NULL != ssl);

   ss_ftp_request *r;
 
   r = (ss_ftp_request *) c->data;
   assert(NULL != r);

   if (ngx_ssl_create_connection(ssl, c, 1) != NGX_OK) {
      /* TODO : make "1" to be more elegant */
      ngx_log_debug(NGX_LOG_DEBUG_FTP, r->connection->log, 0, "ftp:ngx_ssl_create_connection error");
      ss_ftp_reply(r, "451", "Server error in processing");
      return NGX_ERROR;
   }
  
   return NGX_OK;
}


#endif /* _SS_FTP_SSL_C_ */ 


