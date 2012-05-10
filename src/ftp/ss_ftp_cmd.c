
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CMD_C_
#define _SS_FTP_CMD_C_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
//#include <sys/socket.h>
//#include <sys/types.h>
#include "ssftp.h"
#include "ssftp.c"
#include "ss_ftp_cmd.h"
#include "ss_ftp_reply.h"
#include "ss_ftp_reply.c"

#define ss_merge(x,y)  x##y


void ss_ftp_create_commands_hash_table(ngx_pool_t *pool); 
void ss_ftp_undefined_cmd(ss_ftp_request *r); 
static void ss_ftp_user(ss_ftp_request *r);
static void ss_ftp_pass(ss_ftp_request *r);
static void ss_ftp_quit(ss_ftp_request *r);
static void ss_ftp_pasv(ss_ftp_request *r);
static void ss_ftp_type(ss_ftp_request *r);
static void ss_ftp_pwd(ss_ftp_request *r);


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
       ss_ftp_pasv },

     { ngx_string("PORT"),
       NGX_CONF_TAKE1,
       ss_ftp_quit },

     { ngx_string("TYPE"),
       NGX_CONF_TAKE1,
       ss_ftp_type },


   /* 
    *  FTP SERVICE COMMANDS 
    */

     { ngx_string("PWD"),
       NGX_CONF_NOARGS,
       ss_ftp_pwd },

       ss_null_command
};

void 
ss_ftp_undefined_cmd(ss_ftp_request *r)
{
   ss_ftp_reply(r, COMMAND_NOT_IMPLEMENTED, COMMAND_NOT_IMPLEMENTED_M);
}

static void
ss_ftp_user(ss_ftp_request *r)
{
   ss_ftp_reply(r, USER_NAME_OK_NEED_PASSWORD, USER_NAME_OK_NEED_PASSWORD_M);
}

static void
ss_ftp_pass(ss_ftp_request *r)
{
   ss_ftp_reply(r, USER_LOGGED_IN, USER_LOGGED_IN_M);
}

static void
ss_ftp_quit(ss_ftp_request *r)
{
}

static void
ss_ftp_pasv(ss_ftp_request *r)
{
   ngx_connection_t  *data_conn;
   ngx_event_t       *rev;

   char port_str[10];

   struct sockaddr_in addr;
   socklen_t addr_len;
   unsigned short port;

   ngx_int_t listenfd = socket(AF_INET, SOCK_STREAM, 0); 
   struct sockaddr_in serveraddr;
   serveraddr.sin_family = AF_INET;
   serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
   serveraddr.sin_port = 0;
   bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
   listen(listenfd, 1024);
   /* TODO : error handling  */

   data_conn = ngx_get_connection(listenfd, NULL);
   /* set log, the second argument */
   /* error handling */

   r->data_connection = data_conn;

   data_conn->listening = ngx_pcalloc(r->pool, sizeof(ngx_listening_t));
   /* TODO : check whether need to alloc space  */
   data_conn->listening->handler = ss_ftp_init_data_connection;

   rev = data_conn->read;
   rev->handler = ngx_event_accept; 
 
   ngx_handle_read_event(rev, 0);
   /* error handling */ 

   addr_len = sizeof(addr);
   getsockname(listenfd, (struct sockaddr *) &addr, &addr_len); 
   /* error handling */ 
      
   port = ntohs(addr.sin_port);
   snprintf(port_str, sizeof(port_str), "%i", port);
  
   ss_ftp_reply(r, ENTERING_PASSIVE_MODE, port_str);
   
}

static void
ss_ftp_type(ss_ftp_request *r)
{
   ss_ftp_reply(r, COMMAND_OK, COMMAND_OK_M);
}
static void
ss_ftp_pwd(ss_ftp_request *r)
{
   ss_ftp_reply(r, PATH_CREATED, ss_ftp_home_dir);
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
