/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_SSL_MODULE_C_
#define _SS_FTP_SSL_MODULE_C_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

#include "ss_ftp_core.h"


static void *ss_ftp_ssl_create_srv_conf(ngx_conf_t *cf);
static char *ss_ftp_ssl_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ss_ftp_ssl_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 
ngx_int_t ss_ftp_ssl_create(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_certificate(ss_ftp_request *r, ngx_ssl_t *ssl);
ngx_int_t ss_ftp_ssl_create_connection(ngx_connection_t *c, ngx_ssl_t *ssl);


static ss_ftp_ssl_handlers_t ss_ftp_ssl_handlers = {

        ss_ftp_ssl_create,
        ss_ftp_ssl_certificate,
        ss_ftp_ssl_create_connection,
        ngx_ssl_handshake

};
 
static ngx_command_t  ss_ftp_ssl_commands[] = {

     { ngx_string("ssl"),
       SS_FTP_MAIN_CONF | SS_FTP_SRV_CONF|NGX_CONF_FLAG,
       ss_ftp_ssl_enable,
       SS_FTP_SRV_CONF_OFFSET,
       offsetof(ss_ftp_ssl_srv_conf_t, enable),
       NULL },
 
     { ngx_string("ssl_certificate"),
       SS_FTP_SRV_CONF|NGX_CONF_TAKE1,
       ngx_conf_set_str_slot,
       SS_FTP_SRV_CONF_OFFSET,
       offsetof(ss_ftp_ssl_srv_conf_t, certificate),
       NULL },
 
     { ngx_string("ssl_certificate_key"),
       SS_FTP_SRV_CONF|NGX_CONF_TAKE1,
       ngx_conf_set_str_slot,
       SS_FTP_SRV_CONF_OFFSET,
       offsetof(ss_ftp_ssl_srv_conf_t, certificate_key),
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
        ss_ftp_ssl_merge_srv_conf,           /* merge server configuration */
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


static void *
ss_ftp_ssl_create_srv_conf(ngx_conf_t *cf)
{
    ss_ftp_ssl_srv_conf_t  *sscf;

    sscf = ngx_pcalloc(cf->pool, sizeof(ss_ftp_ssl_srv_conf_t));
    if (sscf == NULL) {
        return NULL;
    }
 
    /*
     * set by ngx_pcalloc():
     *
     *     sscf->protocols = 0;
     *     sscf->certificate = { 0, NULL };
     *     sscf->certificate_key = { 0, NULL };
     */
 
     sscf->enable = NGX_CONF_UNSET;
 
     return sscf;
}
 
 
static char *
ss_ftp_ssl_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
   ss_ftp_ssl_srv_conf_t *prev = parent;
   ss_ftp_ssl_srv_conf_t *conf = child;
 
//    ngx_pool_cleanup_t  *cln;

   if (conf->enable == NGX_CONF_UNSET) {
       if (prev->enable == NGX_CONF_UNSET) {
           conf->enable = 0;

        } else {
          conf->enable = prev->enable;
          conf->file = prev->file;
          conf->line = prev->line;
        }
    }

/*
360     ngx_conf_merge_value(conf->session_timeout,
361                          prev->session_timeout, 300);
362 
363     ngx_conf_merge_value(conf->prefer_server_ciphers,
364                          prev->prefer_server_ciphers, 0);
365 
366     ngx_conf_merge_bitmask_value(conf->protocols, prev->protocols,
367                          (NGX_CONF_BITMASK_SET|NGX_SSL_SSLv3|NGX_SSL_TLSv1));
368 
369     ngx_conf_merge_uint_value(conf->verify, prev->verify, 0);
370     ngx_conf_merge_uint_value(conf->verify_depth, prev->verify_depth, 1);
371 
*/

   ngx_conf_merge_str_value(conf->certificate, prev->certificate, "");
   ngx_conf_merge_str_value(conf->certificate_key, prev->certificate_key, "");
 
/*
375     ngx_conf_merge_str_value(conf->dhparam, prev->dhparam, "");
376 
377     ngx_conf_merge_str_value(conf->client_certificate, prev->client_certificate,
378                          "");
379     ngx_conf_merge_str_value(conf->crl, prev->crl, "");
380 
381     ngx_conf_merge_str_value(conf->ecdh_curve, prev->ecdh_curve,
382                          NGX_DEFAULT_ECDH_CURVE);
383 
384     ngx_conf_merge_str_value(conf->ciphers, prev->ciphers, NGX_DEFAULT_CIPHERS);
*/
 
 
   //conf->ssl.log = cf->log;

   if (conf->enable) {
 
       if (conf->certificate.len == 0) {
           ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                        "no \"ssl_certificate\" is defined for "
                        "the \"ssl\" directive in %s:%ui",
                         conf->file, conf->line);
           return NGX_CONF_ERROR;
       }
 
       if (conf->certificate_key.len == 0) {
           ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                         "no \"ssl_certificate_key\" is defined for "
                         "the \"ssl\" directive in %s:%ui",
                          conf->file, conf->line);
           return NGX_CONF_ERROR;
       }
 
   } else {

       if (conf->certificate.len == 0) {
           return NGX_CONF_OK;
        }
 
       if (conf->certificate_key.len == 0) {
           ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                         "no \"ssl_certificate_key\" is defined "
                         "for certificate \"%V\"", &conf->certificate);
           return NGX_CONF_ERROR;
       }
   }
 
/*
   if (ngx_ssl_create(&conf->ssl, conf->protocols, conf) != NGX_OK) {
422         return NGX_CONF_ERROR;
423     }
424 
425 #ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
426 
427     if (SSL_CTX_set_tlsext_servername_callback(conf->ssl.ctx,
428                                                ngx_http_ssl_servername)
429         == 0)
430     {
431         ngx_log_error(NGX_LOG_WARN, cf->log, 0,
432             "nginx was built with SNI support, however, now it is linked "
433             "dynamically to an OpenSSL library which has no tlsext support, "
434             "therefore SNI is not available");
435     }
436 
437 #endif
438 
439     cln = ngx_pool_cleanup_add(cf->pool, 0);
440     if (cln == NULL) {
441         return NGX_CONF_ERROR;
442     }
443 
444     cln->handler = ngx_ssl_cleanup_ctx;
445     cln->data = &conf->ssl;
446 
447     if (ngx_ssl_certificate(cf, &conf->ssl, &conf->certificate,
448                             &conf->certificate_key)
449         != NGX_OK)
450     {
451         return NGX_CONF_ERROR;
452     }
453 
454     if (SSL_CTX_set_cipher_list(conf->ssl.ctx,
455                                 (const char *) conf->ciphers.data)
456         == 0)
457     {
458         ngx_ssl_error(NGX_LOG_EMERG, cf->log, 0,
459                       "SSL_CTX_set_cipher_list(\"%V\") failed",
460                       &conf->ciphers);
461     }
462 
463     if (conf->verify) {
464 
465         if (conf->client_certificate.len == 0) {
466             ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
467                           "no ssl_client_certificate for ssl_client_verify");
468             return NGX_CONF_ERROR;
469         }
470 
471         if (ngx_ssl_client_certificate(cf, &conf->ssl,
472                                        &conf->client_certificate,
473                                        conf->verify_depth)
474             != NGX_OK)
475         {
476             return NGX_CONF_ERROR;
477         }
478 
479         if (ngx_ssl_crl(cf, &conf->ssl, &conf->crl) != NGX_OK) {
480             return NGX_CONF_ERROR;
481         }
482     }
483 
484     if (conf->prefer_server_ciphers) {
485         SSL_CTX_set_options(conf->ssl.ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
486     }
487 
// a temporary 512-bit RSA key is required for export versions of MSIE 
489     SSL_CTX_set_tmp_rsa_callback(conf->ssl.ctx, ngx_ssl_rsa512_key_callback);
490 
491     if (ngx_ssl_dhparam(cf, &conf->ssl, &conf->dhparam) != NGX_OK) {
492         return NGX_CONF_ERROR;
493     }
494 
495     if (ngx_ssl_ecdh_curve(cf, &conf->ssl, &conf->ecdh_curve) != NGX_OK) {
496         return NGX_CONF_ERROR;
497     }
498 
499     ngx_conf_merge_value(conf->builtin_session_cache,
500                          prev->builtin_session_cache, NGX_SSL_NONE_SCACHE);
501 
502     if (conf->shm_zone == NULL) {
503         conf->shm_zone = prev->shm_zone;
504     }
505 
506     if (ngx_ssl_session_cache(&conf->ssl, &ngx_http_ssl_sess_id_ctx,
507                               conf->builtin_session_cache,
508                               conf->shm_zone, conf->session_timeout)
509         != NGX_OK)
510     {
511         return NGX_CONF_ERROR;
512     }
513 
*/
   return NGX_CONF_OK;
}

static char *
ss_ftp_ssl_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
   ss_ftp_ssl_srv_conf_t *sscf = conf;

   char  *rv;

   rv = ngx_conf_set_flag_slot(cf, cmd, conf);

   if (rv != NGX_CONF_OK) {
       return rv;
   }

   sscf->handlers = &ss_ftp_ssl_handlers;
   sscf->file = cf->conf_file->file.name.data;
   sscf->line = cf->conf_file->line;

   return NGX_CONF_OK;
}

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

   ngx_conf_t                 cf;
   ss_ftp_conf_ctx_t         *context;
   ss_ftp_ssl_srv_conf_t     *scf;
   ngx_str_t                  cert;
   ngx_str_t                  key;

   context = r->ctx;
   scf = context->srv_conf[ss_ftp_ssl_module.ctx_index];

   cert = scf->certificate;
   key = scf->certificate_key;
   cf.cycle = *cycle_g;
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


#endif /* _SS_FTP_SSL_MODULE_C_ */

