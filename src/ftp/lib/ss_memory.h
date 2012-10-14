/*
 * Copyright (c) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_MEMORY_H_
#define _SS_MEMORY_H_


#include "ss_ftp_core.h"


ngx_chain_t * ss_create_temp_chain(ngx_pool_t *pool, ngx_int_t size);


#endif /* _SS_MEMORY_H_*/

