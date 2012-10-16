/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_SSL_MODULE_C_
#define _SS_FTP_SSL_MODULE_C_


#include "ss_ftp_core.h"


static void *ss_ftp__ssl_create_srv_conf(ngx_conf_t *cf);
//static char *ngx_http_ssl_merge_srv_conf(ngx_conf_t *cf,
//      void *parent, void *child);
 
static char *ngx_http_ssl_enable(ngx_conf_t *cf, ngx_command_t *cmd,
            void *conf);
 
static ngx_command_t  ss_ftp_ssl_commands[] = {

     { ngx_string("ssl"),
       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_FLAG,
       ngx_http_ssl_enable,
       NGX_HTTP_SRV_CONF_OFFSET,
       offsetof(ngx_http_ssl_srv_conf_t, enable),
       NULL },
 
     { ngx_string("ssl_certificate"),
       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
       ngx_conf_set_str_slot,
       NGX_HTTP_SRV_CONF_OFFSET,
       offsetof(ngx_http_ssl_srv_conf_t, certificate),
       NULL },
 
     { ngx_string("ssl_certificate_key"),
       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
       ngx_conf_set_str_slot,
       NGX_HTTP_SRV_CONF_OFFSET,
       offsetof(ngx_http_ssl_srv_conf_t, certificate_key),
       NULL },
/* 
 75     { ngx_string("ssl_dhparam"),
 76       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
 77       ngx_conf_set_str_slot,
 78       NGX_HTTP_SRV_CONF_OFFSET,
 79       offsetof(ngx_http_ssl_srv_conf_t, dhparam),
 80       NULL },
 81 
 82     { ngx_string("ssl_ecdh_curve"),
 83       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
 84       ngx_conf_set_str_slot,
 85       NGX_HTTP_SRV_CONF_OFFSET,
 86       offsetof(ngx_http_ssl_srv_conf_t, ecdh_curve),
 87       NULL },
 88 
 89     { ngx_string("ssl_protocols"),
 90       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_1MORE,
 91       ngx_conf_set_bitmask_slot,
 92       NGX_HTTP_SRV_CONF_OFFSET,
 93       offsetof(ngx_http_ssl_srv_conf_t, protocols),
 94       &ngx_http_ssl_protocols },
 95 
 96     { ngx_string("ssl_ciphers"),
 97       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
 98       ngx_conf_set_str_slot,
 99       NGX_HTTP_SRV_CONF_OFFSET,
100       offsetof(ngx_http_ssl_srv_conf_t, ciphers),
101       NULL },
102 
103     { ngx_string("ssl_verify_client"),
104       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
105       ngx_conf_set_enum_slot,
106       NGX_HTTP_SRV_CONF_OFFSET,
107       offsetof(ngx_http_ssl_srv_conf_t, verify),
108       &ngx_http_ssl_verify },
109 
110     { ngx_string("ssl_verify_depth"),
111       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_1MORE,
112       ngx_conf_set_num_slot,
113       NGX_HTTP_SRV_CONF_OFFSET,
114       offsetof(ngx_http_ssl_srv_conf_t, verify_depth),
115       NULL },
116 
117     { ngx_string("ssl_client_certificate"),
118       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
119       ngx_conf_set_str_slot,
120       NGX_HTTP_SRV_CONF_OFFSET,
121       offsetof(ngx_http_ssl_srv_conf_t, client_certificate),
122       NULL },
123 
124     { ngx_string("ssl_prefer_server_ciphers"),
125       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_FLAG,
126       ngx_conf_set_flag_slot,
127       NGX_HTTP_SRV_CONF_OFFSET,
128       offsetof(ngx_http_ssl_srv_conf_t, prefer_server_ciphers),
129       NULL },
130 
131     { ngx_string("ssl_session_cache"),
132       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE12,
133       ngx_http_ssl_session_cache,
134       NGX_HTTP_SRV_CONF_OFFSET,
135       0,
136       NULL },
137 
138     { ngx_string("ssl_session_timeout"),
139       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
140       ngx_conf_set_sec_slot,
141       NGX_HTTP_SRV_CONF_OFFSET,
142       offsetof(ngx_http_ssl_srv_conf_t, session_timeout),
143       NULL },
144 
145     { ngx_string("ssl_crl"),
146       NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
147       ngx_conf_set_str_slot,
148       NGX_HTTP_SRV_CONF_OFFSET,
149       offsetof(ngx_http_ssl_srv_conf_t, crl),
150       NULL },
151 
*/
        ngx_null_command
};
 
 
static ss_ftp_module_t  ss_ftp_ssl_module_ctx = {
        //ngx_http_ssl_add_variables,            /* preconfiguration */
        NULL,                                  /* preconfiguration */
        NULL,                                  /* postconfiguration */
 
        NULL,                                  /* create main configuration */
        NULL,                                  /* init main configuration */

        ss_ftp_ssl_create_srv_conf,          /* create server configuration */
        NULL,                                   /* merge server configuration */
        //ngx_http_ssl_merge_srv_conf,           /* merge server configuration */
 
};
 
 
ngx_module_t  ss_ftp_ssl_module = {
        NGX_MODULE_V1,
        &ss_ftp_ssl_module_ctx,              /* module context */
        ss_ftp_ssl_commands,                 /* module directives */
        SS_FTP_MODULE,                       /* module type */
        NULL,                                  /* init master */
        NULL,                                  /* init module */
        NULL,                                  /* init process */
        NULL,                                  /* init thread */
        NULL,                                  /* exit thread */
        NULL,                                  /* exit process */
        NULL,                                  /* exit master */
        NGX_MODULE_V1_PADDING
};


#endif /* _SS_FTP_SSL_MODULE_C_ */

