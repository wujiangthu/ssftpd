/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CORE_MODULE_C_
#define _SS_FTP_CORE_MODULE_C_


#include "ss_ftp_core.h"


static char *ss_ftp_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy);
static void *ss_ftp_core_create_main_conf(ngx_conf_t *cf);
static void *ss_ftp_core_create_srv_conf(ngx_conf_t *cf);


extern ngx_uint_t  ss_ftp_max_module;


static ngx_command_t ss_ftp_core_commands[] = {

  { ngx_string("server"),
    SS_FTP_MAIN_CONF | NGX_CONF_BLOCK | NGX_CONF_MULTI | NGX_CONF_NOARGS,
    ss_ftp_core_server,
    0,
    0,
    NULL },

  { ngx_string("server_ip"),
    SS_FTP_SRV_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    SS_FTP_SRV_CONF_OFFSET,
    offsetof(ss_ftp_core_srv_conf_t, ip),
    NULL },

  { ngx_string("listen"),
    SS_FTP_SRV_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    SS_FTP_SRV_CONF_OFFSET,
    offsetof(ss_ftp_core_srv_conf_t, port),
    NULL },

  { ngx_string("welcome_message"),
    SS_FTP_SRV_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    SS_FTP_SRV_CONF_OFFSET,
    offsetof(ss_ftp_core_srv_conf_t, welcome_message),
    NULL },

  { ngx_string("home_dir"),
    SS_FTP_SRV_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    SS_FTP_SRV_CONF_OFFSET,
    offsetof(ss_ftp_core_srv_conf_t, home_dir),
    NULL },

 
    ngx_null_command 
};

static ss_ftp_module_t ss_ftp_core_module_ctx = {
    NULL,
    NULL,

    ss_ftp_core_create_main_conf,
    NULL,

    ss_ftp_core_create_srv_conf,
    NULL
};

ngx_module_t ss_ftp_core_module = {
    NGX_MODULE_V1,
    &ss_ftp_core_module_ctx,
    ss_ftp_core_commands,
    SS_FTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};


static char *
ss_ftp_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
{
   assert(NULL != cf);
   assert(NULL != cmd);

   char                        *rv;
   void                        *mconf;
   ngx_uint_t                   i;
   ngx_conf_t                   pcf;
   ss_ftp_module_t             *module;
   //struct sockaddr_in          *sin;
   ss_ftp_conf_ctx_t           *ctx, *ftp_ctx;
//   ngx_http_listen_opt_t        lsopt;
   ss_ftp_core_srv_conf_t      *cscf, **cscfp;
   ss_ftp_core_main_conf_t     *cmcf;
 
   ctx = ngx_pcalloc(cf->pool, sizeof(ss_ftp_conf_ctx_t));
   if (ctx == NULL) {
       return NGX_CONF_ERROR;
   }
 
   ftp_ctx = cf->ctx;
   ctx->main_conf = ftp_ctx->main_conf;
 
   /* the server{}'s srv_conf */
 
   ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ss_ftp_max_module);
   if (ctx->srv_conf == NULL) {
       return NGX_CONF_ERROR;
   }
 
 
   for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != SS_FTP_MODULE) {
            continue;
        }
 
        module = ngx_modules[i]->ctx;

        if (module->create_srv_conf) {
            mconf = module->create_srv_conf(cf);
            if (mconf == NULL) {
                return NGX_CONF_ERROR;
            }

            ctx->srv_conf[ngx_modules[i]->ctx_index] = mconf;
        }
   }
 
 
   /* the server configuration context */
 
   cscf = ctx->srv_conf[ss_ftp_core_module.ctx_index];
   cscf->ctx = ctx;
 
 
   cmcf = ctx->main_conf[ss_ftp_core_module.ctx_index];
 
   cscfp = ngx_array_push(&cmcf->servers);
   if (cscfp == NULL) {
       return NGX_CONF_ERROR;
   }
 
   *cscfp = cscf;
 
 
   /* parse inside server{} */
 
   pcf = *cf;
   cf->ctx = ctx;
   cf->cmd_type = SS_FTP_SRV_CONF;

   rv = ngx_conf_parse(cf, NULL);
 
   *cf = pcf;
/*
 
   if (rv == NGX_CONF_OK && !cscf->listen) {
2694         ngx_memzero(&lsopt, sizeof(ngx_http_listen_opt_t));
2695 
2696         sin = &lsopt.u.sockaddr_in;
2697 
2698         sin->sin_family = AF_INET;
2699 #if (NGX_WIN32)
2700         sin->sin_port = htons(80);
2701 #else
2702         sin->sin_port = htons((getuid() == 0) ? 80 : 8000);
2703 #endif
2704         sin->sin_addr.s_addr = INADDR_ANY;
2705 
2706         lsopt.socklen = sizeof(struct sockaddr_in);
2707 
2708         lsopt.backlog = NGX_LISTEN_BACKLOG;
2709         lsopt.rcvbuf = -1;
2710         lsopt.sndbuf = -1;
2711 #if (NGX_HAVE_SETFIB)
2712         lsopt.setfib = -1;
2713 #endif
2714         lsopt.wildcard = 1;
2715 
2716         (void) ngx_sock_ntop(&lsopt.u.sockaddr, lsopt.addr,
2717                              NGX_SOCKADDR_STRLEN, 1);
2718 
2719         if (ngx_http_add_listen(cf, cscf, &lsopt) != NGX_OK) {
2720             return NGX_CONF_ERROR;
2721         }
2722     }
2723 
*/
   return rv;
}

static void *
ss_ftp_core_create_main_conf(ngx_conf_t *cf)
{
   assert(NULL != cf);

   ss_ftp_core_main_conf_t  *cmcf;

   cmcf = ngx_pcalloc(cf->pool, sizeof(ss_ftp_core_main_conf_t));
   if (NULL == cmcf) {
      return NULL;
   }

   /* TODO: init servers in ftp main conf*/
   if (ngx_array_init(&cmcf->servers, cf->pool, 4,
                       sizeof(ss_ftp_core_srv_conf_t *))
       != NGX_OK)
   {
       return NULL;
   }

   return cmcf;
}


static void *
ss_ftp_core_create_srv_conf(ngx_conf_t *cf)
{
   assert(NULL != cf);

   ss_ftp_core_srv_conf_t  *cscf;

   cscf = ngx_pcalloc(cf->pool, sizeof(ss_ftp_core_srv_conf_t));
   if (cscf == NULL) {
       return NULL;
   }
 
   /*
    * set by ngx_pcalloc():
    *
    *     conf->client_large_buffers.num = 0;
    */
/* 
   if (ngx_array_init(&cscf->server_names, cf->temp_pool, 4,
         sizeof(ngx_http_server_name_t))
3150         != NGX_OK)
3151     {
3152         return NULL;
3153     }
*/
   
   /* TODO */
   //cscf->ip = NGX_CONF_UNSET_STR;
   cscf->port = NGX_CONF_UNSET_UINT;

   return cscf;
}


#endif /* _SS_FTP_CORE_MODULE_C_ */

