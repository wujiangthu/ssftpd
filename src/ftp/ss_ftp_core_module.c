/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CORE_MODULE_C_
#define _SS_FTP_CORE_MODULE_C_


#include "ss_ftp_core.h"


static void *ss_ftp_core_create_main_conf(ngx_conf_t *cf);


static ngx_command_t ss_ftp_core_commands[] = {

  { ngx_string("welcome_message"),
    SS_FTP_MAIN_CONF | NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    SS_FTP_MAIN_CONF_OFFSET,
    offsetof(ss_ftp_core_main_conf_t, welcome_message),
    NULL },
 
    ngx_null_command 
};

static ss_ftp_module_t ss_ftp_core_module_ctx = {
    NULL,
    NULL,

    ss_ftp_core_create_main_conf,
    NULL,

    NULL,
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


static void *
ss_ftp_core_create_main_conf(ngx_conf_t *cf)
{
   ss_ftp_core_main_conf_t  *cmcf;

   cmcf = ngx_pcalloc(cf->pool, sizeof(ss_ftp_core_main_conf_t));
   if (NULL == cmcf) {
      return NULL;
   }

   /* TODO: init servers in ftp main conf*/

   return cmcf;
}


#endif /* _SS_FTP_CORE_MODULE_C_ */

