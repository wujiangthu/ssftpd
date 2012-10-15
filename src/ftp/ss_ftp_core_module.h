/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CORE_MODULE_H_
#define _SS_FTP_CORE_MODULE_H_


#include "ss_ftp_core.h"


typedef struct {
    ngx_array_t  servers;   /* ss_ftp_core_srv_conf_t */
    ngx_str_t    welcome_message;

} ss_ftp_core_main_conf_t;

typedef struct {
    /* array of the ngx_http_server_name_t, "server_name" directive */
    ngx_array_t          server_names;

    /* server context */
    ss_ftp_conf_ctx_t   *ctx;

} ss_ftp_core_srv_conf_t;


extern ngx_module_t ss_ftp_core_module;


#endif /* _SS_FTP_CORE_MODULE_H_ */

