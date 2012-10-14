/*
 * Copyright(c) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FILE_C_
#define _SS_FILE_C_


#include "ss_ftp_core.h"


ngx_int_t ss_check_dir_existence(ss_path_t *dir_name);
ngx_int_t ss_check_file_existence(ss_path_t *file_name);
ss_path_t *ss_get_absolute_path(ngx_pool_t *pool, ss_path_t *file, ss_path_t *prefix);
ngx_int_t ss_get_realpath(ngx_pool_t *pool, ss_path_t *file_path, ss_path_t **realpath);
ss_path_t *s_create_path_buf(ngx_pool_t *pool, size_t size);
ss_path_t *ss_chars_to_path(char *file_chars, ss_path_t *path);
ss_path_t *ss_chars_to_path_alloc(ngx_pool_t *pool, char *file);


ngx_int_t
ss_check_dir_existence(ss_path_t *dir_name)
{
  assert(NULL != dir_name);
 
  struct stat dir_info;
  int         rc;

  rc = stat((const char *) dir_name->path, &dir_info);
  if (-1 == rc) {
     return false;
  }

  if (S_ISDIR(dir_info.st_mode) != 0) {
     return true;

  } else {
     return false;
  }
}

ngx_int_t 
ss_check_file_existence(ss_path_t *file_name)
{
   assert(NULL != file_name);

   struct stat file_info;
   int         rc;

   rc = stat((const char *) file_name->path, &file_info);
   if (-1 == rc) {
      return false;
   }

   return true;   
}

ss_path_t *
ss_get_absolute_path(ngx_pool_t *pool, ss_path_t *file, ss_path_t *prefix)
{
   assert(NULL != pool);
   assert(NULL != file);
   assert(NULL != prefix);

   ss_path_t      *abs_path;
   size_t          path_len;

   if (ss_check_absolute_path(file) == true) { 
      return file;       
   } 
 
   path_len = file->plen + prefix->plen;
   abs_path = ss_create_path_buf(pool, path_len + 1);
   if (NULL == abs_path) {
      return NULL;
   } 

   ngx_memcpy(abs_path->path, prefix->path, prefix->plen);
   ngx_memcpy(abs_path->path + prefix->plen, file->path, file->psize);

   abs_path->plen = path_len;
   abs_path->psize = path_len + 1;

   return abs_path;
}

ngx_int_t
ss_get_realpath(ngx_pool_t *pool, ss_path_t *file_path, ss_path_t **real_path)
{
   assert(NULL != pool);
   assert(NULL != file_path);
   assert(NULL != real_path);

   size_t       path_len;

   *real_path = ss_create_path_buf(pool, file_path->psize);
   /* TODO : more error should be handled */
   if (NULL == *real_path) {
      return OUT_OF_MEMORY;
   }

   if (realpath((const char *) file_path->path, (char *) (*real_path)->path) == NULL) {
      return errno;
   }

   path_len = strlen((char *) (*real_path)->path);
   (*real_path)->plen = path_len;
   (*real_path)->psize = path_len + 1;
   
   return SS_FTP_OK;;
}

ss_path_t *
ss_create_path_buf(ngx_pool_t *pool, size_t size)
{
   assert(NULL != pool);

   ss_path_t   *path_buf;

   path_buf = ngx_pcalloc(pool, sizeof(ss_path_t));
   if (NULL == path_buf) {
      return NULL;
   }

   path_buf->path = ngx_pcalloc(pool, size);
   if (NULL == path_buf->path) {
      return NULL;
   }

   path_buf->plen = size - 1;
   path_buf->psize = size;

   return path_buf;
}

ss_path_t *
ss_chars_to_path(char *file_chars, ss_path_t *path)
{
   assert(NULL != file_chars);
   assert(NULL != path);

   size_t  file_path_len;

   file_path_len = strlen(file_chars);
   path->path = (u_char *) file_chars;
   path->plen = file_path_len; 
   path->psize = file_path_len + 1;

   return path; 
}

ss_path_t *
ss_chars_to_path_alloc(ngx_pool_t *pool, char *file_chars)
{
   assert(NULL != pool);
   assert(NULL != file_chars);

   size_t      file_path_len;
   ss_path_t  *file_path;
 
   file_path_len = strlen(file_chars);
   file_path = ss_create_path_buf(pool, file_path_len + 1); 
   if (NULL == file_path) {
      return NULL;
   }
 
   file_path->plen = file_path_len;
   file_path->psize = file_path_len + 1;
   ngx_memcpy(file_path->path, file_chars, file_path->psize);

   return file_path;
}

ss_path_t *
ss_ngx_str_to_path_alloc(ngx_pool_t *pool, ngx_str_t *str)
{
   assert(NULL != pool);
   assert(NULL != str);

   ss_path_t  *path;
  
   path = ss_create_path_buf(pool, str->len + 1); 
   if (NULL == path) {
      return NULL;
   }
 
   path->plen = str->len;
   path->psize = str->len + 1;
   ngx_memcpy(path->path, str->data, path->plen);
   path->path[path->plen] = '\0';

   return path;
}


#endif /* _SS_FILE_C_  */
