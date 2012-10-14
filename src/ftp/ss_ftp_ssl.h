/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_SSL_H_
#define _SS_FTP_SSL_H_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

#include "ss_ftp_core.h"


ngx_int_t ss_ftp_ssl_create(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_certificate(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_create_connection(ngx_connection_t *c, ngx_ssl_t *ssl);


#endif /* _SS_FTP_SSL_H_ */ 
