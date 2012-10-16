/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_C_
#define _SS_FTP_C_


#include "ss_ftp_core.h"


static char *ss_ftp_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


ngx_uint_t  ss_ftp_max_module;


static ngx_command_t ss_ftp_commands[] = {

  { ngx_string("ftp"),
    NGX_MAIN_CONF | NGX_CONF_BLOCK | NGX_CONF_NOARGS,
    ss_ftp_block,
    0,
    0,
    NULL },

    ngx_null_command
};

static ngx_core_module_t ss_ftp_module_ctx = {
  ngx_string("ftp"),
  NULL,
  NULL
};

ngx_module_t ss_ftp_module = {
    NGX_MODULE_V1,
    &ss_ftp_module_ctx,
    ss_ftp_commands,
    NGX_CORE_MODULE,
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
ss_ftp_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
   char                        *rv;
   ngx_uint_t                   mi, m;
   ngx_conf_t                   pcf;
//   ngx_http_module_t           *module;
   ss_ftp_module_t           *module;
//   ngx_http_conf_ctx_t         *ctx;
   ss_ftp_conf_ctx_t         *ctx;
//   ngx_http_core_loc_conf_t    *clcf;
//   ngx_http_core_srv_conf_t   **cscfp;
    /* TODO */
  // ss_ftp_core_srv_conf_t   **cscfp;
//   ngx_http_core_main_conf_t   *cmcf;
   /* TODO */ 
 // ss_ftp_core_main_conf_t   *cmcf;
 
   /* the main http context */
 
   ctx = ngx_pcalloc(cf->pool, sizeof(ss_ftp_conf_ctx_t));
   if (ctx == NULL) {
      return NGX_CONF_ERROR;
   }
 
   *(ss_ftp_conf_ctx_t **) conf = ctx;
 
 
   /* count the number of the http modules and set up their indices */
 
   ss_ftp_max_module = 0;
   for (m = 0; ngx_modules[m]; m++) {
       if (ngx_modules[m]->type != SS_FTP_MODULE) {
          continue;
       }
 
       ngx_modules[m]->ctx_index = ss_ftp_max_module++;
   }
 
 
   /* the ftp main_conf context, it is the same in the all ftp contexts */
 
   ctx->main_conf = ngx_pcalloc(cf->pool,
                                sizeof(void *) * ss_ftp_max_module);
   if (ctx->main_conf == NULL) {
      return NGX_CONF_ERROR;
   }
 
 
   /*
    * the http null srv_conf context, it is used to merge
    * the server{}s' srv_conf's
    */
 
   ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ss_ftp_max_module);
   if (ctx->srv_conf == NULL) {
       return NGX_CONF_ERROR;
   }
 
 
   /*
    * create the main_conf's, the null srv_conf's, and the null loc_conf's
    * of the all http modules
    */
 
   for (m = 0; ngx_modules[m]; m++) {
       if (ngx_modules[m]->type != SS_FTP_MODULE) {
           continue;
       }
 
       module = ngx_modules[m]->ctx;
       mi = ngx_modules[m]->ctx_index;
 
       if (module->create_main_conf) {
           ctx->main_conf[mi] = module->create_main_conf(cf);
           if (ctx->main_conf[mi] == NULL) {
               return NGX_CONF_ERROR;
           }
       }
 
       if (module->create_srv_conf) {
           ctx->srv_conf[mi] = module->create_srv_conf(cf);
           if (ctx->srv_conf[mi] == NULL) {
               return NGX_CONF_ERROR;
           }
       }
   }
 
   pcf = *cf;
   cf->ctx = ctx;
 
   for (m = 0; ngx_modules[m]; m++) {
        if (ngx_modules[m]->type != SS_FTP_MODULE) {
            continue;
        }
 
        module = ngx_modules[m]->ctx;
 
        if (module->preconfiguration) {
            if (module->preconfiguration(cf) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        }
   }
 
   /* parse inside the ftp{} block */
 
   cf->module_type = SS_FTP_MODULE;
   cf->cmd_type = SS_FTP_MAIN_CONF;
   rv = ngx_conf_parse(cf, NULL);
 
printf("***********------------------****\n");
ss_ftp_core_main_conf_t *mm=ctx->main_conf[ss_ftp_core_module.ctx_index];
ss_ftp_core_srv_conf_t  *ss=mm->servers.elts;
printf("%u\n", ss->port);

   if (rv != NGX_CONF_OK) {
       goto failed;
   }
 
   /* init ftp{} main_conf's, merge the server{}s' srv_conf's */
 
  /* TODO*/
   //cmcf = ctx->main_conf[ss_ftp_core_module.ctx_index];
   //cscfp = cmcf->servers.elts;
 
   for (m = 0; ngx_modules[m]; m++) {
       if (ngx_modules[m]->type != SS_FTP_MODULE) {
           continue;
       }

       module = ngx_modules[m]->ctx;
       mi = ngx_modules[m]->ctx_index;
 
       /* init ftp{} main_conf's */

       if (module->init_main_conf) {
           rv = module->init_main_conf(cf, ctx->main_conf[mi]);
           if (rv != NGX_CONF_OK) {
               goto failed;
           }
       }
 
      /* TODO : replace this function */
//       rv = ngx_http_merge_servers(cf, cmcf, module, mi);
//       if (rv != NGX_CONF_OK) {
//           goto failed;
//       }
   }
 
//     if (ngx_http_init_headers_in_hash(cf, cmcf) != NGX_OK) {
//         return NGX_CONF_ERROR;
//    }
 
   for (m = 0; ngx_modules[m]; m++) {
        if (ngx_modules[m]->type != SS_FTP_MODULE) {
            continue;
        }
 
        module = ngx_modules[m]->ctx;
 
        if (module->postconfiguration) {
            if (module->postconfiguration(cf) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        }
   }
 
//   if (ngx_http_variables_init_vars(cf) != NGX_OK) {
//       return NGX_CONF_ERROR;
//   }
 
   /*
    * http{}'s cf->ctx was needed while the configuration merging
    * and in postconfiguration process
    */
 
   *cf = pcf;
 
//printf("%u\n",  ((ss_ftp_core_srv_conf_t *) (ctx->main_conf[ss_ftp_core_module.ctx_index])->servers->elts))->port);

 
//   if (ngx_http_init_phase_handlers(cf, cmcf) != NGX_OK) {
//       return NGX_CONF_ERROR;
//   }
 
 
   /* optimize the lists of ports, addresses and server names */

//    if (ngx_http_optimize_servers(cf, cmcf, cmcf->ports) != NGX_OK) {
//       return NGX_CONF_ERROR;
//    }
 
    return NGX_CONF_OK;
 
    failed:
 
    *cf = pcf;
 
    return rv;
}


#endif /* _SS_FTP_C_ */



