/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_SSL_MODULE_H_
#define _SS_FTP_SSL_MODULE_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

#include "ss_ftp_core.h"


ngx_int_t ss_ftp_ssl_create(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_certificate(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_create_connection(ngx_connection_t *c, ngx_ssl_t *ssl);


typedef struct {
    ngx_int_t  (*create_ssl)(ss_ftp_request *r, ngx_ssl_t *ssl);
    ngx_int_t  (*certificate)(ss_ftp_request *r, ngx_ssl_t *ssl);
    ngx_int_t  (*create_connection)(ngx_connection_t *c, ngx_ssl_t *ssl);
    ngx_int_t  (*handshake)(ngx_connection_t *c);

} ss_ftp_ssl_handlers_t;

typedef struct {

    ngx_flag_t              enable;

    ngx_str_t               certificate;
    ngx_str_t               certificate_key;

    ss_ftp_ssl_handlers_t   *handlers;

    /* For configuration file parsing */

    u_char                 *file;
    ngx_uint_t              line;

} ss_ftp_ssl_srv_conf_t;


extern ngx_module_t  ss_ftp_ssl_module;


#endif /* _SS_FTP_SSL_MODULE_H_ */

