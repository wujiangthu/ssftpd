/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_STRING_C_
#define _SS_STRING_C_


#include "ss_ftp_core.h"
#include "ss_string.h"
#include "ngx_core.h"
#include "ngx_config.h"


void ss_to_lower(u_char *str, ngx_int_t len);
char *ss_str_to_string(ngx_str_t *str, ngx_pool_t *pool);


void
ss_to_lower(u_char *str, ngx_int_t len)
{
   assert(str != NULL && len > 0);

   ngx_int_t count = 0;
   for (count = 0; count < len; count++) {
       str[count] = tolower(str[count]);
   }
}

char * 
ss_str_to_string(ngx_str_t *str, ngx_pool_t *pool)
{
   assert(NULL != str);
   assert(NULL != pool);

   char *string;

   string = ngx_pcalloc(pool, str->len + 1);
   if (NULL == string) {
      return NULL;
   }

   strncpy(string, (const char *) str->data, str->len);
   string[str->len] = '\0';

   return string;
}


#endif /* _SS_STRING_C_*/

