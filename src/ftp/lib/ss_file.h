/*
 * Copyright(c) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FILE_H_
#define _SS_FILE_H_


#include "ss_ftp_core.h"


#define  true   1
#define  false  0 

typedef struct {
    u_char      *path;
    size_t       plen;  /* Path length, not including terminating \0 */
    size_t       psize; /* plen + 1, including terminating \0 */
} ss_path_t;

#define ss_path(str)  { (u_char *) str, strlen(str) - 1, strlen(str) }
#define ss_check_absolute_path(file_path) file_path->path[0] == '/' \
                                          ? true : false


ngx_int_t ss_check_dir_existence(ss_path_t *dir_name);
ngx_int_t ss_check_file_existence(ss_path_t *file_name);
ss_path_t *ss_get_absolute_path(ngx_pool_t *pool, ss_path_t *file, ss_path_t *prefix);
ngx_int_t ss_get_realpath(ngx_pool_t *pool, ss_path_t *file_path, ss_path_t **realpath);
ss_path_t *ss_create_path_buf(ngx_pool_t *pool, size_t size);
ss_path_t *ss_chars_to_path(char *file_chars, ss_path_t *path);
ss_path_t *ss_chars_to_path_alloc(ngx_pool_t *pool, char *file);
ss_path_t *ss_ngx_str_to_path_alloc(ngx_pool_t *pool, ngx_str_t *str);


#endif /* _SS_FILE_H_  */
