
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CMD_C_
#define _SS_FTP_CMD_C_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include "ssftp.h"
#include "ssftp.c"
#include "ss_ftp_cmd.h"
#include "ss_ftp_reply.h"
#include "ss_ftp_reply.c"
#include <assert.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/types.h>

#define ss_merge(x,y)  x##y


void ss_ftp_create_commands_hash_table(ngx_pool_t *pool); 
static ss_ftp_send_receive_cmd *ss_ftp_create_send_receive_cmd(ss_ftp_request *r);
static void ss_ftp_data_link_try_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
void ss_ftp_undefined_cmd(ss_ftp_request *r); 
static void ss_ftp_user(ss_ftp_request *r);
static void ss_ftp_pass(ss_ftp_request *r);
static void ss_ftp_cwd(ss_ftp_request *r);
static void ss_ftp_cdup(ss_ftp_request *r);
static void ss_ftp_quit(ss_ftp_request *r);
static void ss_ftp_mode(ss_ftp_request *r);
static void ss_ftp_pasv(ss_ftp_request *r);
static void ss_ftp_port(ss_ftp_request *r);
static void ss_ftp_type(ss_ftp_request *r);
static void ss_ftp_pwd(ss_ftp_request *r);
static void ss_ftp_list(ss_ftp_request *r);
static ngx_int_t list_dir_files(ngx_pool_t *pool, ngx_list_t *list, const char *dir);
ngx_chain_t * list_to_chain(ngx_pool_t *pool, ngx_list_t *list);
static void ss_ftp_list_clean_up(ngx_connection_t *c);
static void ss_ftp_retr(ss_ftp_request *r);
static void ss_ftp_retr_clean_up(ngx_connection_t *c);
static void ss_ftp_stor(ss_ftp_request *r);
static void ss_ftp_stor_process(ngx_connection_t *c);
static void ss_ftp_stor_clean_up(ngx_connection_t *c);

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
       ss_ftp_cwd},
       
     { ngx_string("CDUP"),
       NGX_CONF_NOARGS,
       ss_ftp_cdup},

     { ngx_string("QUIT"),
       NGX_CONF_NOARGS,
       ss_ftp_quit },


   /* 
    *  TRANSFER PARAMETER COMMANDS
    */

     { ngx_string("MODE"),
       NGX_CONF_TAKE1,
       ss_ftp_mode},

     { ngx_string("PASV"),
       NGX_CONF_TAKE1,
       ss_ftp_pasv },

     { ngx_string("PORT"),
       NGX_CONF_TAKE1,
       ss_ftp_port },

     { ngx_string("TYPE"),
       NGX_CONF_TAKE1,
       ss_ftp_type },


   /* 
    *  FTP SERVICE COMMANDS 
    */

     { ngx_string("PWD"),
       NGX_CONF_NOARGS,
       ss_ftp_pwd },

     { ngx_string("LIST"),
       NGX_CONF_NOARGS,
       ss_ftp_list },

     { ngx_string("RETR"),
       NGX_CONF_NOARGS,
       ss_ftp_retr },

     { ngx_string("STOR"),
       NGX_CONF_NOARGS,
       ss_ftp_stor },

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
   ngx_str_t *arg;
   char       *file_name;
   arg = r->cmd_args->elts;

   file_name = ngx_pcalloc(r->pool, arg->len + 1);
   strncpy(file_name, (const char *) arg->data, arg->len);
   file_name[arg->len] = '\0';

   ss_ftp_reply(r, USER_NAME_OK_NEED_PASSWORD, USER_NAME_OK_NEED_PASSWORD_M);
}

static void
ss_ftp_pass(ss_ftp_request *r)
{
   ngx_str_t *arg;
   char       *file_name;
   arg = r->cmd_args->elts;

   file_name = ngx_pcalloc(r->pool, arg->len + 1);
   strncpy(file_name, (const char *) arg->data, arg->len);
   file_name[arg->len] = '\0';

   ss_ftp_reply(r, USER_LOGGED_IN, USER_LOGGED_IN_M);
}

static void 
ss_ftp_cwd(ss_ftp_request *r)
{
   printf("%s\n", "enter cwd command******");

   ss_ftp_reply(r, COMMAND_OK, COMMAND_OK_M);
}

static void 
ss_ftp_cdup(ss_ftp_request *r)
{
   printf("%s\n", "enter cdup command******");
}

static void
ss_ftp_quit(ss_ftp_request *r)
{
   printf("%s\n", "enter quit command******");

   /*TODO : make sure all file operations on this control connection are completed*/ 
   ngx_connection_t *c;
   
   c = r->connection;
   assert(NULL != c);
   
   ss_ftp_reply(r, CLOSING_CONTROL_CONNECTION, CLOSING_CONTROL_CONNECTION_M);
   if (0 == c->num_data_conns) {
   printf("%s\n", "to be closed");
      ss_ftp_close_connection(c); 
      ngx_destroy_pool(r->pool);

   } else {
   printf("%s\n", "not now");
      c->to_be_closed = CONTROL_CONN_CAN_BE_CLOSED;   
   }
   printf("%s\n", "exit quit command******");
}

static void 
ss_ftp_mode(ss_ftp_request *r)
{
   printf("%s\n", "enter mode command******");
}

static void
ss_ftp_pasv(ss_ftp_request *r)
{
   printf("%s\n", "enter pasv command");

   ngx_connection_t  *data_listen_conn;
   ngx_connection_t  *control_conn;
   ngx_event_t       *rev;
   ngx_log_t         *log;
   ss_ftp_send_receive_cmd *srcmd;

   char address_port_str[100];

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

   data_listen_conn = ngx_get_connection(listenfd, NULL);
   /* set log, the second argument */
   /* error handling */

//   r->data_connection = data_conn;

   data_listen_conn->data = r;
   data_listen_conn->listening = ngx_pcalloc(r->pool, sizeof(ngx_listening_t));
   //TODO : error handling
   /* TODO : check whether need to alloc space  */
   data_listen_conn->listening->pool_size = 4096;
   data_listen_conn->listening->handler = ss_ftp_init_data_connection;
   assert(data_listen_conn->ftp_conn_type == SS_FTP_UNINIT_CONN);
   data_listen_conn->ftp_conn_type = SS_FTP_DATA_LISTEN_CONN;

   rev = data_listen_conn->read;
   //rev->handler = ss_ftp_data_accept; 
   rev->handler = ngx_event_accept; 

   log = ngx_pcalloc(r->pool, sizeof(ngx_log_t)); 
   if (NULL == log) printf("%s\n", "**log error, log null");
   //TODO : error handling
   *log = *(r->connection->log);
   data_listen_conn->log = log;
   data_listen_conn->listening->log = *log;
   rev->log = log; 

   srcmd = ss_ftp_create_send_receive_cmd(r);
   srcmd->data_listen_connection = data_listen_conn;

   control_conn = r->connection;
   assert(control_conn->num_data_conns >= 0);
   control_conn->num_data_conns++; 

   ngx_handle_read_event(rev, 0);
   /* error handling */ 

   addr_len = sizeof(addr);
   if (getsockname(listenfd, (struct sockaddr *) &addr, &addr_len) != 0) {

      /* error handling */ 

      ss_ftp_reply(r, SERVICE_NOT_AVAILABLE, SERVICE_NOT_AVAILABLE_M);
     
      return;
   } 
      
   //address = ntohl(addr.sin_addr);
   char address[20]; 
   char *temp;
   temp = inet_ntoa(addr.sin_addr);   
   strcpy(address, temp);

   port = ntohs(addr.sin_port);

   //printf("%s%d\n", "port: ", port);
   unsigned char p1 = port >> 8;
   unsigned char p2 = port - p1 * 256;

   snprintf(address_port_str, 
            sizeof(address_port_str), 
            "%s %c%s,%u,%u%c", 
           // "Entering passive mode", '(', address, p1, p2, ')');

           /* TODO : address 0.0.0.0 find why  */

            "Entering passive mode", '(', "10,199,46,164", p1, p2, ')');
  
   ss_ftp_reply(r, ENTERING_PASSIVE_MODE, address_port_str);
   
}

static void
ss_ftp_port(ss_ftp_request *r) 
{
   printf("%s\n", "enter port command"); 
}

static void
ss_ftp_type(ss_ftp_request *r)
{
   ss_ftp_reply(r, COMMAND_OK, COMMAND_OK_M);
}

static void
ss_ftp_pwd(ss_ftp_request *r)
{
   ss_ftp_reply(r, PATH_CREATED, ss_ftp_home_dir_l);
}

static void
ss_ftp_list(ss_ftp_request *r)
{
   printf("%s\n", "enter list command");
   
   assert(r != NULL);
   assert(NULL != r->connection);


  // char list[] = "-rw-rw-rw- 1 1000 1000 6401 Jul 01 2000 hello.txt\r\n";
   //char list[] = "123.txt\r\na";
   //char list[] = "-rw-rw-r--    1 1000     1000            0 Apr 24 03:07 vimrc\r\n";

   ngx_chain_t  *chain_list;
   ngx_list_t   *list;
   ngx_int_t     rc;
   ss_ftp_send_receive_cmd *srcmd;

   ss_ftp_reply(r, FILE_STATUS_OK, "Here comes the directory listing");

   list = ngx_list_create(r->pool, 5, sizeof(ngx_str_t)); 
   if (NULL == list) {
      ss_ftp_process_insufficient_memory(r->connection);
      return;
   }

   rc = list_dir_files(r->pool, list, ss_ftp_home_dir);
   if (OUT_OF_MEMORY == rc) {
      ss_ftp_process_insufficient_memory(r->connection);
      return;
   }
   if (OPEN_DIR_ERROR == rc) {
      ss_ftp_reply(r, FILE_UNAVAILABLE, FILE_UNAVAILABLE_M);
      return;
   }
   assert(SS_FTP_OK == rc);
   // eroor handling
   chain_list = list_to_chain(r->pool, list);
   if (NULL == chain_list) {
      ss_ftp_process_insufficient_memory(r->connection);
      return;
   }
  // chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t));
  // chain->buf = ngx_create_temp_buf(r->pool, sizeof(list));

   //snprintf((char *) chain->buf->pos, sizeof(list),
         //   "%s", list);

   /* leave terminating '\0' */
   //chain->buf->last = chain->buf->last + sizeof(list) -1;
   srcmd = r->send_receive_cmd; 
   assert(srcmd->type == 0);
   srcmd->type = SS_FTP_SEND_CMD;
   srcmd->clean_up = ss_ftp_list_clean_up;

   ss_ftp_data_link_try_add_chain(r, chain_list);

  // ss_ftp_reply(r, FILE_ACTION_OK, "Directory send OK.");
  /* TODO : Should be done in ss_ftp_process_data_link function  */
}

// to decide return char * or chain *, and change ss_ftp_data_link_try_add_chain
// make it to be chain list, not only one chain
static ngx_int_t 
list_dir_files(ngx_pool_t *pool, ngx_list_t *list, const char *dir)
{
   char a[] =  "-rw-rw-r--    1 1000     1000           27 Apr 12 04:54 ";

   assert(NULL != pool);
   assert(NULL != list);
   assert(NULL != dir);

   DIR           *dp;
   struct dirent *ep;
   u_char          *dir_item_name;
   ngx_int_t      dir_item_len; 
   ngx_str_t     *list_node;

   dp = opendir(dir);
   if (NULL == dp) {
      return OPEN_DIR_ERROR; 
   }

  // ep = readdir(dp);
   for (ep = readdir(dp); ep; ep = readdir(dp)) {
     dir_item_len = strlen(ep->d_name);
     //dir_item_name = ngx_pcalloc(pool, dir_item_len + 2); 
     dir_item_name = ngx_pcalloc(pool, dir_item_len + strlen(a) + 2); 
     if (NULL == dir_item_name) {
         return OUT_OF_MEMORY;
     }

     if (!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, "..") || !strcmp(ep->d_name, "dir")) {
        continue; 
     }
   //test
     strncpy((char *) dir_item_name, a , strlen(a));
     strncpy((char *) dir_item_name + strlen(a), (const char *) (ep->d_name), dir_item_len);
     //dir_item_name[dir_item_len] = '\r';
     //dir_item_name[dir_item_len + 1] = '\n';
     dir_item_name[dir_item_len + +strlen(a)] = '\r';
     dir_item_name[dir_item_len + strlen(a)+1] = '\n';
   printf("%s\n", dir_item_name);
     list_node = (ngx_str_t *) ngx_list_push(list);
     if (NULL == list_node) {
        return OUT_OF_MEMORY;
     }

     list_node->data = dir_item_name;
     //list_node->len = dir_item_len + 2;
     list_node->len = dir_item_len +strlen(a)+ 2;

   //  ep = readdir(dp);
   }

   closedir(dp);
  
   return SS_FTP_OK;
}
   
ngx_chain_t *
list_to_chain(ngx_pool_t *pool, ngx_list_t *list)
{
  assert(NULL != pool);
  assert(NULL != list);

  struct ngx_list_part_s *lpart;
  ngx_str_t              *dir_items;
  unsigned int           count;
  ngx_chain_t            *head, *last, *current;
  ngx_buf_t              *buf;

  head = NULL;
  last = NULL;
  current = NULL;
  lpart = &list->part;
  dir_items = (ngx_str_t *) lpart->elts;
  for (count = 0 ;; count++) {
      if (count >= lpart->nelts) {
         if (lpart->next == NULL) {
            break;  
         }  
   
         lpart = lpart->next;
         dir_items = (ngx_str_t *) lpart->elts;
         count = 0;
      }

      current = ngx_pcalloc(pool, sizeof(ngx_chain_t)); 
      if (NULL == current) {
         return NULL;
      }

      current->buf = ngx_pcalloc(pool, sizeof(ngx_buf_t)); 
      if (NULL == current->buf) {
         return NULL;
      }

      buf = current->buf;
      buf->pos = buf->start = dir_items[count].data;
      buf->last = buf->end = dir_items[count].data + dir_items[count].len -1;
      buf->temporary = 1;

      if (NULL == head) {
         head = current;
         last = head;

      } else {
         last->next = current;   
         last = current;
      }
  }
 
  return head;
   //snprintf((char *) chain->buf->pos, sizeof(list),
         //   "%s", list);
   /* leave terminating '\0' */
   //chain->buf->last = chain->buf->last + sizeof(list) -1;
}

static void 
ss_ftp_list_clean_up(ngx_connection_t *c)
{ 
   assert(NULL != c);

   ss_ftp_request *r;
   
   r = (ss_ftp_request *) c->data;
   ss_ftp_reply(r, FILE_ACTION_OK, FILE_ACTION_OK_M); 

   ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->log, 0, "ftp:about to close data connection");
   ss_ftp_close_connection(c);
}

static void
ss_ftp_retr(ss_ftp_request *r)
{
   printf("%s\n", " enter retrive command");

   assert(NULL != r);   

   ngx_str_t  *arg;
   char       *file_name;
   ngx_fd_t    fd;
   ngx_chain_t *chain;
   ngx_buf_t   *buf;

   arg = r->cmd_args->elts;

   file_name = ngx_pcalloc(r->pool, sizeof(ss_ftp_home_dir) -1 + arg->len + 1);
   strncpy(file_name, ss_ftp_home_dir, sizeof(ss_ftp_home_dir) -1);
   strncpy(file_name + sizeof(ss_ftp_home_dir) -1, (const char *) arg->data, arg->len);
   file_name[sizeof(ss_ftp_home_dir) -1 + arg->len] = '\0';

   /* TODO : choose text mode or binary mode according to type command sent by client  */

   fd = open(file_name, O_RDONLY);
   printf("%s %d\n", "fd", fd); 
   assert(NULL != r->send_receive_cmd);
   r->send_receive_cmd->fd_retr = fd;
   /* error handling  */

   /* TODO : to check i/o operating modes for potential performance improvements */

   chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t)); 
   chain->next = NULL;
   chain->buf  = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

   buf = chain->buf;

   buf->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
   buf->file->fd = fd;
   buf->in_file = 1;
   buf->file_pos = 1; /* File beginning  */ 
   /* TODO : check whether file position starts from 0 */
   buf->file_last = lseek(fd, 0, SEEK_END); 

   assert(r->send_receive_cmd->type == 0);
   r->send_receive_cmd->type = SS_FTP_SEND_CMD;
   r->send_receive_cmd->clean_up = ss_ftp_retr_clean_up;

   ss_ftp_reply(r, FILE_STATUS_OK, "open data connection for file transfering");
   ss_ftp_data_link_try_add_chain(r, chain);

   //TODO : ss_ftp_reply(r, , );
}

static void 
ss_ftp_retr_clean_up(ngx_connection_t *c)
{ 
   assert(NULL != c);

   ss_ftp_request *r;
   
   r = (ss_ftp_request *) c->data;
   ss_ftp_reply(r, FILE_ACTION_OK, FILE_ACTION_OK_M); 

   ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->log, 0, "ftp:about to close data connection");
   ss_ftp_close_connection(c);
}

static void
ss_ftp_stor(ss_ftp_request *r)
{
//   ss_ftp_reply(r, , );
}

static void 
ss_ftp_stor_process(ngx_connection_t *c)
{

}

static void 
ss_ftp_stor_clean_up(ngx_connection_t *c)
{

}

void
ss_ftp_create_commands_hash_table(ngx_pool_t *pool) 
{
     assert(pool != NULL);

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

ss_ftp_send_receive_cmd * 
ss_ftp_create_send_receive_cmd(ss_ftp_request *r)
{
   assert(r != NULL);

   ss_ftp_send_receive_cmd **srcmd;

   srcmd = &r->send_receive_cmd;
   assert(*srcmd == NULL);
   *srcmd = ngx_pcalloc(r->pool, sizeof(ss_ftp_send_receive_cmd)); 

   (*srcmd)->request = r;
   
   return *srcmd;
}

static void 
ss_ftp_data_link_try_add_chain(ss_ftp_request *r, ngx_chain_t *chain)
{
   printf("%s\n", "try add chain");
   assert(r != NULL);
   assert(chain != NULL);
	
   ngx_listening_t         *ls;
   ngx_connection_t        *dc;
   ss_ftp_send_receive_cmd *srcmd;
   srcmd = r->send_receive_cmd;
  
   srcmd->chain = chain; 

   /* Command arrives after data connection has established */
   dc = srcmd->data_connection;
   if (NULL != dc) {
      printf("%s\n", "Command arrives after data connection has established"); 

      dc->send_receive_cmd = srcmd;
      //r->send_receive_cmd = NULL;
      assert(dc->listening != NULL && dc->listening->handler != NULL);
      ls = dc->listening;
      ls->handler(dc); 
   }
}


#endif  /* _SS_FTP_CMD_C_  */
