
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CMD_C_
#define _SS_FTP_CMD_C_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include "ss_ftp_cmd.h"
#include "ss_ftp_reply.h"
#include "ss_ftp_reply.c"

#define ss_merge(x,y)  x##y

void ss_ftp_create_commands_hash_table(ngx_pool_t *pool); 
static void ss_ftp_user(ss_ftp_request *r);
static void ss_ftp_pass(ss_ftp_request *r);
static void ss_ftp_quit(ss_ftp_request *r);


static ss_ftp_command ss_ftp_commands[] = {
       
   /*
    *  ACCESS CONTROL COMMANDS 
    */

     { ngx_string("USER"),
       NGX_CONF_TAKE1,
       ss_ftp_user },

     { ngx_string("PASS"),
       NGX_CONF_TAKE1,
       ss_ftp_pass },
       
     { ngx_string("CWD"),
       NGX_CONF_TAKE1,
       ss_ftp_user },
       
     { ngx_string("CDUP"),
       NGX_CONF_NOARGS,
       ss_ftp_user },

     { ngx_string("QUIT"),
       NGX_CONF_NOARGS,
       ss_ftp_quit },


   /* 
    *  TRANSFER PARAMETER COMMANDS
    */

     { ngx_string("MODE"),
       NGX_CONF_TAKE1,
       ss_ftp_quit },

     { ngx_string("PASV"),
       NGX_CONF_TAKE1,
       ss_ftp_quit },

     { ngx_string("PORT"),
       NGX_CONF_TAKE1,
       ss_ftp_quit },

     { ngx_string("TYPE"),
       NGX_CONF_TAKE1,
       ss_ftp_quit },


   /* 
    *  FTP SERVICE COMMANDS 
    */

       ss_null_command
};

static void
ss_ftp_user(ss_ftp_request *r)
{
   ss_ftp_reply(r, USER_NAME_OK_NEED_PASSWORD);
}

static void
ss_ftp_pass(ss_ftp_request *r)
{
   ss_ftp_reply(r, USER_LOGGED_IN);
}

static void
ss_ftp_quit(ss_ftp_request *r)
{
}

void
ss_ftp_create_commands_hash_table(ngx_pool_t *pool) {
     ngx_hash_init_t    *hash_init;
     ngx_array_t        *ss_ftp_cmds_array;

     hash_init = (ngx_hash_init_t *) ngx_pcalloc(pool,
                                                 sizeof(ngx_hash_init_t));
     ss_ftp_cmds_hash_table = (ngx_hash_t *) ngx_pcalloc(pool, 
                                                         sizeof(ngx_hash_t));
     hash_init->hash        = ss_ftp_cmds_hash_table;
     hash_init->key         = &ngx_hash_key_lc;
     hash_init->max_size    = 1024 * 10;
     hash_init->bucket_size = 64;
     hash_init->name        = "ss_ftp_command_hash";
     hash_init->pool        = pool;
     hash_init->temp_pool   = NULL;

     int arr_size = sizeof(ss_ftp_commands) / sizeof(ss_ftp_command) -1;
     int count;
     ngx_hash_key_t *arr_node;
     ss_ftp_cmds_array = ngx_array_create(pool, 
                                          arr_size, 
					  sizeof(ngx_hash_key_t)); 

     for(count = 0; count < arr_size; count++) {
         arr_node       = (ngx_hash_key_t *) ngx_array_push(ss_ftp_cmds_array); 
	 arr_node->key  = ss_ftp_commands[count].name;
	 arr_node->value  = (void *) &ss_ftp_commands[count];
	 arr_node->key_hash  = ngx_hash_key_lc(
	                       ss_ftp_commands[count].name.data, 
			       ss_ftp_commands[count].name.len);
     }

     ngx_hash_init(hash_init, 
                   (ngx_hash_key_t *) ss_ftp_cmds_array->elts,
		   ss_ftp_cmds_array->nelts);
}


#endif  /* _SS_FTP_CMD_C_  */
