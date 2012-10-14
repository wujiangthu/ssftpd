/*
 * Copyright (c) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_MEMORY_C_
#define _SS_MEMORY_C_


#include "ss_ftp_core.h"


ngx_chain_t * ss_create_temp_chain(ngx_pool_t *pool, ngx_int_t size);


ngx_chain_t *
ss_create_temp_chain(ngx_pool_t *pool, ngx_int_t size)
{
   assert(NULL != pool);
   assert(size > 0);

   ngx_chain_t   *chain;

   chain = ngx_pcalloc(pool, sizeof(ngx_chain_t));
   if (NULL == chain) {
      return NULL;
   }

   chain->next = NULL;
   chain->buf = ngx_create_temp_buf(pool, size);
   if (NULL == chain->buf) {
      return NULL;
   }

   return chain;
}


#endif /* _SS_MEMORY_C_*/
