
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_CMD_C_
#define _SS_FTP_CMD_C_

#include "ss_ftp_core.h"
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <assert.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/types.h>

#include <security/pam_appl.h>
#include <security/pam_misc.h>





#define ss_merge(x,y)  x##y
#define SS_WRITE_FILE_AGAIN  -22

ngx_int_t ss_ftp_create_commands_hash_table(ngx_pool_t *pool); 
static ss_ftp_send_receive_cmd *ss_ftp_create_send_receive_cmd(ss_ftp_request *r);
static void ss_ftp_data_link_try_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
void ss_ftp_undefined_cmd(ss_ftp_request *r); 
static void ss_ftp_user(ss_ftp_request *r);
static void ss_ftp_pass(ss_ftp_request *r);
static void ss_ftp_cwd(ss_ftp_request *r);
static ngx_int_t ss_ftp_change_dir(ngx_pool_t *pool, ss_path_t *arg_dir, 
                                   ss_path_t *home_dir);
static void ss_ftp_cdup(ss_ftp_request *r);
static void ss_ftp_quit(ss_ftp_request *r);
static void ss_ftp_mode(ss_ftp_request *r);
static void ss_ftp_pasv(ss_ftp_request *r);
static void ss_ftp_port(ss_ftp_request *r);
static void ss_ftp_type(ss_ftp_request *r);
static void ss_ftp_rmd(ss_ftp_request *r);
static void ss_ftp_mkd(ss_ftp_request *r);
static void ss_ftp_pwd(ss_ftp_request *r);
static void ss_ftp_list(ss_ftp_request *r);
static ngx_int_t get_list_chain(ngx_pool_t *pool, ss_path_t *dir, 
                                ngx_chain_t **chain);
static void ss_ftp_list_clean_up(ngx_connection_t *c);
static void ss_ftp_retr(ss_ftp_request *r);
static void ss_ftp_retr_clean_up(ngx_connection_t *c);
static void ss_ftp_stor(ss_ftp_request *r);
static void ss_ftp_store(ss_ftp_request *r, ngx_int_t flags, ngx_int_t mode);
static int ss_ftp_get_full_filename(ss_ftp_request *r, 
                                    ss_path_t **full_filename);
static int ss_ftp_get_full_real_filename(ss_ftp_request *r, 
                                         ss_path_t **full_filename);
static ngx_int_t ss_ftp_stor_process(ngx_connection_t *c);
static void ss_ftp_stor_clean_up(ngx_connection_t *c);
static void ss_ftp_stou(ss_ftp_request *r);
static void ss_ftp_appe(ss_ftp_request *r);
static void ss_ftp_rnfr(ss_ftp_request *r);
static void ss_ftp_rnto(ss_ftp_request *r);
static void ss_ftp_dele(ss_ftp_request *r);
static void ss_ftp_syst(ss_ftp_request *r);
static int ss_ftp_get_absolute_realpath(ss_ftp_request *r, ss_path_t *arg_dir, 
                                  ss_path_t *home_dir, ss_path_t **reap_path);
static void ss_ftp_auth(ss_ftp_request *r);
void ss_ftp_ssl_control_conn_handler(ngx_connection_t *c);
static void ss_ftp_prot(ss_ftp_request *r);

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

     { ngx_string("RMD"),
       NGX_CONF_NOARGS,
       ss_ftp_rmd },

     { ngx_string("MKD"),
       NGX_CONF_NOARGS,
       ss_ftp_mkd },

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

     { ngx_string("STOU"),
       NGX_CONF_NOARGS,
       ss_ftp_stou },

     { ngx_string("APPE"),
       NGX_CONF_NOARGS,
       ss_ftp_appe},

     { ngx_string("RNFR"),
       NGX_CONF_NOARGS,
       ss_ftp_rnfr},

     { ngx_string("RNTO"),
       NGX_CONF_NOARGS,
       ss_ftp_rnto},

     { ngx_string("DELE"),
       NGX_CONF_NOARGS,
       ss_ftp_dele },

     { ngx_string("SYST"),
       NGX_CONF_NOARGS,
       ss_ftp_syst },

   /* 
    *  Authentication Commands 
    */

     { ngx_string("AUTH"),
       NGX_CONF_NOARGS,
       ss_ftp_auth },

     { ngx_string("PROT"),
       NGX_CONF_NOARGS,
       ss_ftp_prot},


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
   assert(NULL != r);

   ngx_str_t      *arg;
   char           *user_name;
   struct pam_conv conversation;
   int             rc;

   arg = r->cmd_args->elts;
   if ((user_name = ss_str_to_string(arg, r->pool)) == NULL) { 
      ss_ftp_process_out_of_memory(r);
      return;
   }

   ss_to_lower(arg->data, arg->len);
   if (strncmp((const char *) arg->data, "anonymous", sizeof("anonymous") - 1) == 0) {
       ss_ftp_reply(r, COMMAND_OK, COMMAND_OK_M);
       return;
   }

   r->username = user_name;

   conversation.conv = ss_ftp_conversation;
   conversation.appdata_ptr = r;

   rc = pam_start("vsftp", NULL, &conversation, (pam_handle_t **)&r->pamh);
   switch (rc) {
          
   case PAM_SUCCESS:
        break;

   case PAM_ABORT:
   case PAM_BUF_ERR:
   case PAM_SYSTEM_ERR:
        ngx_log_debug2(NGX_LOG_DEBUG_FTP, r->connection->log, 0, \
                       "pam_start failed when authenticate user %s, \
                        error code is : %d", user_name, rc);

        ss_ftp_reply(r, "451", "Request abort, server error in processing.");

        return;
   }

   ss_ftp_reply(r, USER_NAME_OK_NEED_PASSWORD, USER_NAME_OK_NEED_PASSWORD_M);
}

static void
ss_ftp_pass(ss_ftp_request *r)
{
   assert(NULL != r);

   ngx_str_t  *arg;
   char       *pwd;
   int         rc;
   int         result;

   arg = r->cmd_args->elts;
   pwd = ss_str_to_string(arg, r->pool);
   if (NULL == pwd) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

   r->password = pwd;

   result = NGX_ERROR;
   rc = pam_authenticate(r->pamh, 0);
   switch (rc) {

   case PAM_AUTH_ERR:
        ss_ftp_reply(r,"430", "Invalid username or password.");
        break;

   case PAM_CRED_INSUFFICIENT:
   case PAM_MAXTRIES:
        ss_ftp_reply(r, "451", "Log in failed, because of \
                         insufficient credentials or maxtries.");
        break;

   case PAM_USER_UNKNOWN:
        ss_ftp_reply(r,"430", "User unknow.");
        break;

   case PAM_SUCCESS:
        result = NGX_OK;
        break;
 
   case PAM_ABORT: 
   case PAM_AUTHINFO_UNAVAIL:
   default:
        ss_ftp_reply(r, "451", "Request action aborted, \
                         server error in processing.");
        break;
   }

   if (NGX_ERROR == result) {
      pam_end(r->pamh, rc);
      r->username = NULL;
      r->password = NULL;
      return;
   }

   rc = pam_acct_mgmt(r->pamh, 0); 
   switch (rc) {

   case PAM_ACCT_EXPIRED:
        ss_ftp_reply(r,"430", "Account has expired.");
        break;

   case PAM_AUTH_ERR:
        ss_ftp_reply(r,"430", "Invalid username or password.");
        break;

   case PAM_NEW_AUTHTOK_REQD:
        ss_ftp_reply(r,"430", "Password has expired");
        break;

   case PAM_PERM_DENIED:
        ss_ftp_reply(r,"430", "Permission denied.");
        break;

   case PAM_SUCCESS:
        ss_ftp_reply(r, USER_LOGGED_IN, USER_LOGGED_IN_M);
        break;

   case PAM_USER_UNKNOWN:
        ss_ftp_reply(r,"430", "User unknow.");
        break;
   }

   pam_end(r->pamh, rc);
   r->username = NULL;
   r->password = NULL;
}

static void 
ss_ftp_cwd(ss_ftp_request *r)
{
   printf("%s\n", "enter cwd command******");

   assert(NULL != r);

   ngx_str_t    *arg;
   ngx_int_t     rc;
   ss_path_t    *home_dir;
   ss_path_t    *arg_dir;
  
   arg = r->cmd_args->elts;
   arg_dir = ss_ngx_str_to_path_alloc(r->pool, arg);
   if (NULL == arg_dir) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

   home_dir = &r->current_dir;
   rc = ss_ftp_change_dir(r->pool, arg_dir, home_dir);
   if (OUT_OF_MEMORY == rc) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

   if (SS_FTP_FILE_NOT_FOUND == rc) {
      ss_ftp_reply(r, "550", "Directory not exists.");
      return;
   }

   assert(SS_FTP_OK == rc);

   ss_ftp_reply(r, COMMAND_OK, COMMAND_OK_M);
}

static ngx_int_t 
ss_ftp_change_dir(ngx_pool_t *pool, ss_path_t *arg_dir, ss_path_t *home_dir)
{
   printf("%s\n", "enter change dir******");

   assert(NULL != pool);   
   assert(NULL != arg_dir);   
   assert(NULL != home_dir);   

   ss_path_t    *dir;
   ngx_int_t     dir_existence;

   dir = ss_get_absolute_path(pool, arg_dir, home_dir);
   if (NULL == dir) {
      return OUT_OF_MEMORY;
   }

   dir_existence = ss_check_dir_existence(dir); 
   if (false == dir_existence) {
      return SS_FTP_FILE_NOT_FOUND;
   } 

   ngx_memcpy(home_dir->path, dir->path, dir->psize); 
   home_dir->plen = dir->plen;
   home_dir->psize = dir->psize;

   if ('/' != home_dir->path[home_dir->plen - 1]) {
      home_dir->path[home_dir->plen] = '/';
      home_dir->plen++;
      home_dir->psize++;

      home_dir->path[home_dir->plen] = '\0';
   }

   printf("%s\n", "exit change dir******");

   return SS_FTP_OK;
}

static void 
ss_ftp_cdup(ss_ftp_request *r)
{
   printf("%s\n", "enter cdup command******");

   assert(NULL != r);

   ngx_int_t     rc;
   ss_path_t    *home_dir;
   ss_path_t     arg_dir;
   char          parent_dir[] = "..";

   ss_chars_to_path(parent_dir, &arg_dir);
   home_dir = &r->current_dir;
   rc = ss_ftp_change_dir(r->pool, &arg_dir, home_dir);
   if (OUT_OF_MEMORY == rc) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

   /* Change to parent command will not throw file_not_found error */
   assert(SS_FTP_OK == rc);

   ss_ftp_reply(r, COMMAND_OK, COMMAND_OK_M);
   printf("%s\n", "exit cdup command******");
 
}

static void
ss_ftp_quit(ss_ftp_request *r)
{
   printf("%s\n", "enter quit command******");

   assert(NULL != r);

   ngx_connection_t *c;
   
   c = r->connection;
   assert(NULL != c);
   
   if (0 == c->num_data_conns) {
      ss_ftp_reply(r, CLOSING_CONTROL_CONNECTION, CLOSING_CONTROL_CONNECTION_M);
      ss_ftp_close_connection(c); 
      ngx_destroy_pool(r->pool);

   } else {
      assert(1 == c->num_data_conns);
      c->to_be_closed = CONTROL_CONN_CAN_BE_CLOSED;   
   }
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
   //ngx_connection_t  *control_conn;
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

   data_listen_conn = ngx_get_connection(listenfd, r->pool->log);
   /* set log, the second argument */
   /* error handling */

//   r->data_connection = data_conn;

   data_listen_conn->data = r;
   data_listen_conn->listening = ngx_pcalloc(r->pool, sizeof(ngx_listening_t));
   if (NULL == data_listen_conn) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

   /* TODO : check whether need to alloc space  */
   data_listen_conn->listening->pool_size = 4096;
   data_listen_conn->listening->handler = ss_ftp_init_data_connection;
   assert(data_listen_conn->ftp_conn_type == SS_FTP_UNINIT_CONN);
   data_listen_conn->ftp_conn_type = SS_FTP_DATA_LISTEN_CONN;

   rev = data_listen_conn->read;
   //rev->handler = ss_ftp_data_accept; 
   rev->handler = ngx_event_accept; 

   log = ngx_pcalloc(r->pool, sizeof(ngx_log_t)); 
   if (NULL == log) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

   *log = *(r->connection->log);
   data_listen_conn->log = log;
   data_listen_conn->listening->log = *log;
   rev->log = log; 

   srcmd = ss_ftp_create_send_receive_cmd(r);
   if (NULL == srcmd) {
      ss_ftp_process_out_of_memory(r);
      return;
   }
   srcmd->data_listen_connection = data_listen_conn;
   srcmd->data_link_send_type = SS_SEND_CHAIN;

   //control_conn = r->connection;
   //assert(control_conn->num_data_conns >= 0);
   //control_conn->num_data_conns++; 

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
   assert(NULL != r); 

  /* ngx_str_t      *arg;
   struct sockaddr_in server_addr;
   unsigned int addr;
   unsigned 

   arg = r->cmd_args->elts;
   
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = 
   server_addr.sin_addr = 
    */  
}
 
static void
ss_ftp_type(ss_ftp_request *r)
{
   assert(NULL != r);

   ss_ftp_reply(r, COMMAND_OK, COMMAND_OK_M);
}

static void
ss_ftp_rmd(ss_ftp_request *r)
{
   assert(NULL != r);

   ss_path_t   *dir;
   ngx_int_t    rc;

   if (ss_ftp_get_full_real_filename(r, &dir) != SS_FTP_OK) {
      return;
   }

   if (ss_check_dir_existence(dir) == false) {
      ss_ftp_reply(r, "550", "Directory not exists.");
      return;
   }

   rc = rmdir((const char *) dir->path);
   if (0 == rc) {
      ss_ftp_reply(r, "257", "Remove derectory successfully.");
      return;
   }
 
   assert(-1 == rc);

   ngx_log_debug1(NGX_LOG_DEBUG_FTP, r->connection->log, 0, "remove directory %s failed", dir->path);
   return;
}

static void
ss_ftp_mkd(ss_ftp_request *r)
{
   assert(NULL != r);

   ss_path_t   *dir;
   int          err_no;

   if (ss_ftp_get_full_filename(r, &dir) != SS_FTP_OK) {
      return;
   }

   if (ss_check_dir_existence(dir) == true) {
      ss_ftp_reply(r, "550", "Directory exists.");
      return;
   }

   /* TODO : do research on mode flags */
   if (mkdir((const char *) dir->path, S_IRWXU | S_IRWXO) == 0) {
      ss_ftp_reply(r, "257", "Directory created successfully.");
      return;
   }

   err_no = errno;
   ss_ftp_reply_realpath_error(r, err_no); 
}

static void
ss_ftp_pwd(ss_ftp_request *r)
{
   assert(NULL != r);

   ss_path_t   *current_dir;
   char        *result_dir;

   current_dir = &r->current_dir;
   result_dir = ngx_pcalloc(r->pool, current_dir->plen + 3);
   if (NULL == result_dir) {
      ss_ftp_process_out_of_memory(r);
      return; 
   }
 
   ngx_memcpy(result_dir + 1, current_dir->path, current_dir->plen);
   result_dir[0] = '"';
   result_dir[current_dir->plen + 1] = '"';
   result_dir[current_dir->plen + 2] = '\0';

   ss_ftp_reply(r, PATH_CREATED, result_dir);
}

static void
ss_ftp_list(ss_ftp_request *r)
{
   assert(r != NULL);
   assert(NULL != r->connection);

   ngx_chain_t  *chain_list;
   ngx_int_t     rc;
   ss_ftp_send_receive_cmd *srcmd;

   ss_ftp_reply(r, FILE_STATUS_OK, "Here comes the directory listing");

   rc = get_list_chain(r->pool, &r->current_dir, &chain_list);
   if (OUT_OF_MEMORY == rc) {
      ss_ftp_process_out_of_memory(r);
      return;
   }
   if (OPEN_DIR_ERROR == rc) {
      ss_ftp_reply(r, "550", "Directory not exists.");
      return;
   }

   assert(SS_FTP_OK == rc);

   srcmd = r->send_receive_cmd; 
   assert(srcmd->type == 0);
   srcmd->type = SS_FTP_SEND_CMD;
   srcmd->clean_up = ss_ftp_list_clean_up;

   ss_ftp_data_link_try_add_chain(r, chain_list);
}

static ngx_int_t 
get_list_chain(ngx_pool_t *pool, ss_path_t *dir, ngx_chain_t **chain)
{
   assert(NULL != pool); 
   assert(NULL != dir); 

   DIR            *dp;
   struct dirent  *ep;
   struct stat     file_info;
   char           *dir_item_name;
   ss_path_t       dir_item_path;
   size_t          dir_item_name_len;
   size_t          dir_item_len;
   /* TODO : choose a better value  */
   char            dir_item_buf[1024];
   ss_path_t      *full_file_name;
   ngx_chain_t    *head_chain = NULL;
   ngx_chain_t    *rear_chain;
   ngx_chain_t    *temp_chain;
   struct tm       time_temp;

   dp = opendir((const char *) dir->path);
   if (NULL == dp) {
      return OPEN_DIR_ERROR;
   }

   for (ep = readdir(dp); ep; ep = readdir(dp)) {
       dir_item_name = ep->d_name; 
       dir_item_name_len = strlen(dir_item_name); 

       if (   !strcmp(dir_item_name, ".") 
           || !strcmp(dir_item_name, "..") ){
           continue; 
       }

       dir_item_path.path = (u_char *) dir_item_name;
       dir_item_path.plen = dir_item_name_len;
       dir_item_path.psize = dir_item_name_len + 1;

       full_file_name = ss_get_absolute_path(pool, &dir_item_path, dir);
       if (NULL == full_file_name) {
          return OUT_OF_MEMORY;
       }

       if (stat((const char *) full_file_name->path, &file_info) != 0) {
          return STAT_ERROR;   
       }

       localtime_r(&file_info.st_mtime, &time_temp); 

       /* -rw-rw-r-- 1 usher usher  42135 2012-02-05 15:02 ngx_http_script.c */
       sprintf(dir_item_buf, 
               "%crwxrwxr-x %d %d %d %lu %d-%d-%d %d:%d %s",
               GET_FILE_TYPE(file_info),
               file_info.st_nlink,
               file_info.st_uid,
               file_info.st_gid,
               (unsigned long) file_info.st_size,
               time_temp.tm_year + 1900,
               time_temp.tm_mon + 1,
               time_temp.tm_mday,
               time_temp.tm_hour,
               time_temp.tm_min,
               dir_item_name 
              ); 
      dir_item_len = strlen(dir_item_buf); 
      temp_chain = ss_create_temp_chain(pool, dir_item_len + 2);
      if (NULL == temp_chain) {
         return OUT_OF_MEMORY;
      }
  
      ngx_memcpy(temp_chain->buf->pos, dir_item_buf, dir_item_len);
      ngx_memcpy(temp_chain->buf->pos + dir_item_len, "\r\n", sizeof("\r\n") -1);
      temp_chain->buf->last += dir_item_len + 2; 

      if (NULL == head_chain) {
         head_chain = temp_chain; 
         rear_chain = head_chain;

      } else {
         rear_chain->next = temp_chain;
         rear_chain = temp_chain;
      }
   }

   *chain = head_chain;

   return SS_FTP_OK;
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
   assert(NULL != r);   

   char                     *file_name;
   ngx_fd_t                  fd;
   ngx_chain_t              *chain;
   ngx_buf_t                *buf;
   ngx_int_t                 file_flags;
   ss_path_t                *full_filename;
   ss_ftp_send_receive_cmd  *srcmd;


   if (ss_ftp_get_full_real_filename(r, &full_filename) != SS_FTP_OK) {
       return;
   }

   file_name = (char *) full_filename->path;

   /* TODO : choose text mode or binary mode according to type command sent by client  */

   fd = open(file_name, O_RDONLY);
   if (-1 == fd) {
      ngx_log_debug2(NGX_LOG_DEBUG_FTP, c->log, 0, "open file %s failed, errno is %d", file_name, errno);
      ss_ftp_reply(r, "451", "Storing file aborted, server error in processing");
      return;
   }

   if ((file_flags = fcntl(fd, F_GETFL, 0)) < 0) {
      ngx_log_debug1(NGX_LOG_DEBUG_FTP, c->log, 0, "get file %s's attributes failed", file_name);
      ss_ftp_reply(r, "451", "Storing file aborted, server error in processing");
      return;
   }

   if (fcntl(fd, F_SETFL, file_flags | O_NONBLOCK) < 0) {
      ngx_log_debug1(NGX_LOG_DEBUG_FTP, c->log, 0, "set file %s's attributes failed", file_name);
      ss_ftp_reply(r, "451", "Storing file aborted, server error in processing");
      return;
   }

   srcmd = r->send_receive_cmd;
   assert(NULL != srcmd);
   srcmd->fd_retr = fd;
   /* error handling  */

   /* TODO : to check i/o operating modes for potential performance improvements */

   chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t)); 
   if (NULL == chain) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

   chain->next = NULL;

   if (SS_FTP_CLEAR == r->protection_level) {
      chain->buf  = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
      if (NULL == chain->buf) {
         ss_ftp_process_out_of_memory(r);
         return;
      }

      buf = chain->buf;
      buf->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
      if (NULL == buf->file) {
         ss_ftp_process_out_of_memory(r);
         return;
      }

      buf->file->fd = fd;
      buf->in_file = 1;
      buf->file_pos = 1; /* File beginning  */ 
      /* TODO : check whether file position starts from 0 */
      buf->file_last = lseek(fd, 0, SEEK_END); 

   } else {
      srcmd->data_link_send_type = SS_SEND_FILE;
   }

   assert(srcmd->type == 0);
   srcmd->type = SS_FTP_SEND_CMD;
   srcmd->clean_up = ss_ftp_retr_clean_up;

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

   //ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->log, 0, "ftp:about to close data connection");
   ss_ftp_close_connection(c);
}

static void
ss_ftp_stor(ss_ftp_request *r)
{
   assert(NULL != r);
  
   ss_ftp_store(r, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
}

static void
ss_ftp_store(ss_ftp_request *r, ngx_int_t flags, ngx_int_t mode)
{
   assert(NULL != r);

   char                    *file_name;
   ss_path_t               *full_filename;
   ngx_fd_t                 fd;
   ngx_chain_t            **chain;
   ss_ftp_send_receive_cmd *srcmd;
   int                      file_flags;
   ngx_connection_t        *c;

   c = r->connection;
   srcmd = r->send_receive_cmd;
   assert(NULL != srcmd);

   /* Get file descriptor for storing file ready */

   if (ss_ftp_get_full_filename(r, &full_filename) != SS_FTP_OK) {
      ngx_log_debug(NGX_LOG_DEBUG_FTP, c->log, 0, "get file %s's full filename failed when executing store commmand");
      ss_ftp_reply(r, "451", "Storing file aborted, server error in processing");
      return; 
      }
 
   file_name = (char *) full_filename->path;

   /* TODO : choose text mode or binary mode according to type command sent by client  */

   fd = open(file_name, flags, mode);
   if (-1 == fd) {
      ngx_log_debug2(NGX_LOG_DEBUG_FTP, c->log, 0, "open file %s failed, errno is %d", file_name, errno);
      ss_ftp_reply(r, "451", "Storing file aborted, server error in processing");
      return;
   }

   if ((file_flags = fcntl(fd, F_GETFL, 0)) < 0) {
      ngx_log_debug1(NGX_LOG_DEBUG_FTP, c->log, 0, "get file %s's attributes failed", file_name);
      ss_ftp_reply(r, "451", "Storing file aborted, server error in processing");
      return;
   } 

   if (fcntl(fd, F_SETFL, file_flags | O_NONBLOCK) < 0) {
      ngx_log_debug1(NGX_LOG_DEBUG_FTP, c->log, 0, "set file %s's attributes failed", file_name);
      ss_ftp_reply(r, "451", "Storing file aborted, server error in processing");
      return;
   }

   r->send_receive_cmd->fd_stor = fd;

   /* Get data link read buffed ready */

   chain = &srcmd->chain;
   assert(NULL == *chain);
   *chain = ss_create_temp_chain(r->pool, 1024);
   if (NULL == *chain) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

  /* Register the handlers */

   assert(r->send_receive_cmd->type == 0);
   r->send_receive_cmd->type = SS_FTP_RECEIVE_CMD;
   srcmd->process = ss_ftp_stor_process;
   srcmd->clean_up = ss_ftp_stor_clean_up;

   ss_ftp_reply(r, FILE_STATUS_OK, "open data connection for file transfering");
   ss_ftp_data_link_try_add_chain(r, *chain);
}

static int
ss_ftp_get_full_filename(ss_ftp_request *r, ss_path_t **full_filename)
{
   assert(NULL != r);

   ss_path_t   *file_path;
   ss_path_t   *abs_path;
   ngx_str_t   *arg;

   arg = r->cmd_args->elts;
   file_path = ss_ngx_str_to_path_alloc(r->pool, arg);
   if (NULL == file_path) {
      return OUT_OF_MEMORY;
   }

   abs_path = ss_get_absolute_path(r->pool, file_path, &r->current_dir);
   if (NULL == abs_path) {
      return OUT_OF_MEMORY; 
   }

   *full_filename = abs_path;
   return SS_FTP_OK;
}

static int 
ss_ftp_get_full_real_filename(ss_ftp_request *r, ss_path_t **full_filename)
{
   assert(NULL != r);

   ss_path_t   *file_path;
   ngx_str_t   *arg;
   int          rc;

   arg = r->cmd_args->elts;
   file_path = ss_ngx_str_to_path_alloc(r->pool, arg);
   if (NULL == file_path) {
      return OUT_OF_MEMORY;
   }
 
   rc = ss_ftp_get_absolute_realpath(r, file_path, &r->current_dir, full_filename);
   if (SS_FTP_OK != rc) {
      return rc;
   }
   
   return SS_FTP_OK;
}

static ngx_int_t 
ss_ftp_stor_process(ngx_connection_t *c)
{
  assert(NULL != c);

  ss_ftp_send_receive_cmd  *srcmd;
  ngx_buf_t                *buf;
  ngx_int_t                 n;
  ngx_int_t                 error;

  srcmd = (ss_ftp_send_receive_cmd *) c->send_receive_cmd;
  assert(NULL != srcmd);
  assert(NULL != srcmd->chain);
  buf = srcmd->chain->buf;
  assert(buf->last - buf->pos > 0);

  for ( ;; ) {

    if (buf->last - buf->pos < 0) {
       return NGX_ERROR;
    }
 
    if (buf->last - buf->pos == 0) {
       return NGX_OK;
    }

    n = write(srcmd->fd_stor, buf->pos, buf->last - buf->pos);
    if (-1 == n) {
       error = errno;  

       if (EAGAIN == error) { 
          return SS_WRITE_FILE_AGAIN;

       } else {
         ngx_log_debug1(NGX_LOG_DEBUG_FTP, c->log, 0, "write file %d failed", srcmd->fd_stor);
         return NGX_ERROR;
       }
    }

    buf->pos += n; 
  }
}

static void 
ss_ftp_stor_clean_up(ngx_connection_t *c)
{
   //printf("%s\n", "store command clean up");

   assert(NULL != c);

   ss_ftp_request           *r;

   r = (ss_ftp_request *) c->data;
   ss_ftp_reply(r, FILE_ACTION_OK, FILE_ACTION_OK_M);

   ngx_log_debug0(NGX_LOG_DEBUG_FTP, c->log, 0, "ftp:about to close data connection for storing file");
    
   ss_ftp_close_connection(c);
}

static void
ss_ftp_stou(ss_ftp_request *r)
{
   char                    *file_name;
   char                    *template;
   ss_path_t               *full_file_name;
   ss_path_t                file_name_path;
   ngx_int_t                file_name_len;
   ngx_fd_t                 fd;
   ngx_chain_t             **chain;
   //ngx_connection_t        *c;
   ss_ftp_send_receive_cmd *srcmd;
   ngx_str_t               *arg;

   //c = r->connection;
   srcmd = r->send_receive_cmd;
   assert(NULL != srcmd);

   arg = r->cmd_args->elts;
   /* 1 6 1 : . XXXXXX \0 */
   template = ngx_pcalloc(r->pool, arg->len + 1 + 6 + 1);
   if (NULL == template) {
      ss_ftp_process_out_of_memory(r);
      return;
   }
   
   ngx_memcpy(template, arg->data, arg->len);
   ngx_memcpy(template + arg->len, ".", 1);
   ngx_memcpy(template + arg->len + 1, "XXXXXX", 6);
   ngx_memcpy(template + arg->len + 1 + 6 , "\0", 1);

   file_name = mktemp(template);
   if (NULL == file_name) {
      ngx_log_debug1(NGX_LOG_DEBUG_FTP, c->log, 0, "make temporary file for %s failed", file_name);

      ss_ftp_reply(r, "550", "Generating random string for file name failed.");
      return;
   }

   /* TODO : choose text mode or binary mode according to type command sent by client  */
   file_name_len = strlen(file_name);
   file_name_path.path = (u_char *) file_name;
   file_name_path.plen = file_name_len;
   file_name_path.psize= file_name_len + 1;

   if (ss_ftp_get_absolute_realpath(r, &file_name_path, &r->current_dir, &full_file_name) != SS_FTP_OK) {
      return;
   }

   fd = open((const char *) full_file_name->path, O_WRONLY | O_CREAT, S_IRWXU);
   if (-1 == fd) {
      ngx_log_debug2(NGX_LOG_DEBUG_FTP, c->log, 0, "open file %s failed, errno is %d", full_file_name->path, errno);

      ss_ftp_reply(r, "550", "Open file failed.");
      return;
   }

   r->send_receive_cmd->fd_stor = fd;

   /* Get data link read buffed ready */

   chain = &srcmd->chain;
   assert(NULL == *chain);
   *chain = ss_create_temp_chain(r->pool, 1024);
   if (NULL == *chain) {
      ss_ftp_process_out_of_memory(r);
      return;
   }

  /* Register the handlers */

   assert(r->send_receive_cmd->type == 0);
   r->send_receive_cmd->type = SS_FTP_RECEIVE_CMD;
   srcmd->process = ss_ftp_stor_process;
   srcmd->clean_up = ss_ftp_stor_clean_up;

   ss_ftp_reply(r, "150 FILE: ", file_name);
   /* TODO : check how server should response this  */
   ss_ftp_data_link_try_add_chain(r, *chain);
}

static void
ss_ftp_rnfr(ss_ftp_request *r)
{
   assert(NULL != r);

   ss_path_t   *real_path;

   if (ss_ftp_get_full_real_filename(r, &real_path) != SS_FTP_OK) {
      return;
   }

   if (ss_check_file_existence(real_path) == false) {
       ss_ftp_reply(r, "550 ", "File not exists.");
       return;
   }

   r->rename_from_filename = real_path;
   ss_ftp_reply(r, "250", "File name received.");
}

static void
ss_ftp_rnto(ss_ftp_request *r)
{
   assert(NULL != r);

   ss_path_t   *rnfr_path; /* Rename_from name */
   ss_path_t   *rnto_path; /* Rename_to name */

   rnfr_path = r->rename_from_filename;
   if (NULL == rnfr_path) {
      ss_ftp_reply(r, "503 ", "Need rename_from command frist.");
      return;
   }
   
   if (ss_ftp_get_full_filename(r, &rnto_path) != SS_FTP_OK) {
      return;
   }

   if (ss_check_file_existence(rnto_path) == true) {
       ss_ftp_reply(r, "550 ", "File exists.");
       return;
   }

   if (rename((const char *) rnfr_path->path, (const char *) rnto_path->path) == 0) {
       ss_ftp_reply(r, "250 ", "Rename file successfully.");
       return;
   }     
  
   /* TODO : more situation to be handled */
   ss_ftp_reply(r, "550", "Rename file failed.");
}

static void
ss_ftp_appe(ss_ftp_request *r)
{
   assert(NULL != r);

   ss_ftp_store(r, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
}

static void
ss_ftp_dele(ss_ftp_request *r)
{
   printf("%s\n", "enter delete command");

   assert(NULL != r);

   char       *file_name;
   ss_path_t  *full_filename;
   ngx_int_t   rc;

   if (ss_ftp_get_full_real_filename(r, &full_filename) != SS_FTP_OK) {
      return;
   }  

   file_name = (char *) full_filename->path;
   rc = unlink(file_name);
   if (-1 == rc) {
      /* TODO : error handling */
      printf("delete file %s failed\n", file_name);
      ngx_log_debug2(NGX_LOG_DEBUG_FTP, r->connection->log, 0, "delete file %s failed, errno is %d", file_name, errno);
      exit(1);
   }

   ss_ftp_reply(r, "250", "Delete file successfully");
}

static void 
ss_ftp_syst(ss_ftp_request *r)
{
   printf("%s\n", "enter syst command");

   assert(NULL != r);

   /* TODO  */
   ss_ftp_reply(r, "215", "UNIX TYPE: L8");
}


void ss_ftp_process_cmds(ngx_event_t *rev);

static void 
ss_ftp_auth(ss_ftp_request *r)
{
   assert(NULL != r);

   ngx_ssl_t   ssl;

   if (ss_ftp_ssl_create(r, &ssl) != NGX_OK) {
      return;
   } 

   if (ss_ftp_ssl_certificate(r, &ssl) != NGX_OK) {
      return;
   } 

   if (ss_ftp_ssl_create_connection(r->connection, &ssl) != NGX_OK) {
      return;
   } 

   r->connection->ssl->handler = ss_ftp_ssl_control_conn_handler;
   r->connection->ssl->buffer = 0;

   ss_ftp_reply(r, "234", "Enter ssl handshake..");

   /* Error logging performed in ngx_ssl_handshake(), no need another error logging */
   ngx_ssl_handshake(r->connection);
}

void 
ss_ftp_ssl_control_conn_handler(ngx_connection_t *c) 
{
   assert(NULL != c);

   c->read->handler = ss_ftp_process_cmds;
   c->write->handler = ss_ftp_cmd_link_write;
   ss_ftp_process_cmds(c->data);
}

void
ss_ftp_prot(ss_ftp_request *r) 
{
   assert(NULL != r);

   ngx_str_t  *arg; 
   char        level;
  
   arg = r->cmd_args->elts;
   if (1 != arg->len) {
      ss_ftp_reply(r, "500", "Syntax error in prot command's argument");
      return;
   } 

   level =  ((char *) (arg->data))[0];
   switch (level) {

   case 'C':
   case 'c':
        r->protection_level = SS_FTP_CLEAR;          
        break;

   case 'S':
   case 's':
        r->protection_level = SS_FTP_SAFE;
        break;

   case 'E':
   case 'e':
        r->protection_level = SS_FTP_CONFIDENTIAL;
        break;

   case 'P':
   case 'p':
        r->protection_level = SS_FTP_PRIVATE;
        break;
  
   default:
        ss_ftp_reply(r, "500", "Syntax error in prot command's argument");
        return;
   }

   ss_ftp_reply(r, "200", "Specified protection level is accepted.");
}

ngx_int_t
ss_ftp_create_commands_hash_table(ngx_pool_t *pool) 
{
     assert(pool != NULL);

     ngx_hash_init_t    *hash_init;
     ngx_array_t        *ss_ftp_cmds_array;

     hash_init = (ngx_hash_init_t *) ngx_pcalloc(pool,
                                                 sizeof(ngx_hash_init_t));
     if (NULL == hash_init) {
        return OUT_OF_MEMORY; 
     }

     ss_ftp_cmds_hash_table = (ngx_hash_t *) ngx_pcalloc(pool, 
                                                         sizeof(ngx_hash_t));
     if (NULL == ss_ftp_cmds_hash_table) {
        return OUT_OF_MEMORY; 
     }

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
     if (NULL == ss_ftp_cmds_array)  {
        return OUT_OF_MEMORY; 
     }


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

     return NGX_OK;
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
//   assert(chain != NULL);
	
   ngx_listening_t         *ls;
   ngx_connection_t        *dc;
   ss_ftp_send_receive_cmd *srcmd;
   srcmd = r->send_receive_cmd;
  
   srcmd->chain = chain; 
   srcmd->cmd_arrived = true;

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

static int 
ss_ftp_get_absolute_realpath(ss_ftp_request *r, ss_path_t *arg_dir, ss_path_t *home_dir, ss_path_t **real_path)
{

   ss_path_t  *file_path;
   int         rc;

   file_path = ss_get_absolute_path(r->pool, arg_dir, home_dir);
   if (NULL == file_path) {
      ss_ftp_process_out_of_memory(r);
      return OUT_OF_MEMORY;
   }

   rc = ss_get_realpath(r->pool, file_path, real_path);
   if (OUT_OF_MEMORY == rc) {
      ss_ftp_process_out_of_memory(r);
      return OUT_OF_MEMORY;
   }

   /* Error occured */
   if (SS_FTP_OK != rc) {
      ss_ftp_reply_realpath_error(r, rc);
      return rc;
   }
   
   return SS_FTP_OK;
}
#endif  /* _SS_FTP_CMD_C_  */
