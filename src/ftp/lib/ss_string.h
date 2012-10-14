/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_STRING_H_
#define _SS_STRING_H_


#include "ss_ftp_core.h"


void ss_to_lower(u_char *str, ngx_int_t len);
char *ss_str_to_string(ngx_str_t *str, ngx_pool_t *pool);


#endif /* _SS_STRING_H_ */


