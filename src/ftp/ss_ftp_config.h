/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CONFIG_H_
#define _SS_FTP_CONFIG_H_


typedef struct {
    void        **main_conf;
    void        **srv_conf;
} ss_ftp_conf_ctx_t;


typedef struct {
    ngx_int_t   (*preconfiguration)(ngx_conf_t *cf);
    ngx_int_t   (*postconfiguration)(ngx_conf_t *cf);
 
    void       *(*create_main_conf)(ngx_conf_t *cf);
    char       *(*init_main_conf)(ngx_conf_t *cf, void *conf);
 
    void       *(*create_srv_conf)(ngx_conf_t *cf);
    char       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);

} ss_ftp_module_t;


#define SS_FTP_MODULE     0x50545449   /* 'FTP' */

#define SS_FTP_MAIN_CONF  0x02000000
#define SS_FTP_SRV_CONF   0x04000000

#define SS_FTP_MAIN_CONF_OFFSET  offsetof(ss_ftp_conf_ctx_t, main_conf)
#define SS_FTP_SRV_CONF_OFFSET   offsetof(ss_ftp_conf_ctx_t, srv_conf)

#endif /* _SS_FTP_CONFIG_H_ */

