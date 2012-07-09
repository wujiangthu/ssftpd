
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SSFTP_C_
#define _SSFTP_C_
   

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ctype.h>
#include <assert.h>

#include "ssftp.h"
#include "ss_ftp_cmd.h"
#include "ss_ftp_cmd.c"
#include "ss_ftp_reply.h"

#define  SS_AGAIN                         NGX_AGAIN
#define  SS_ERROR                         NGX_ERROR
#define  SS_BUF_FULL                      -100 
#define  SS_FTP_PARSE_ONE_COMMAND_DONE    1
#define  SS_FTP_INVALID_COMMAND           2

#define SS_FTP_REQUEST_DEFAULT_POOL_SIZE  1024*10
#define SS_FTP_CMD_DEFAULT_BUF_LEN        1024

#define assert(expr)                          \
   ((expr)                              \
   ? __ASSERT_VOID_CAST (0)                    \
   : __assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION))

void ss_ftp_init_control_connection(ngx_connection_t *c);
static void ss_ftp_init_request(ngx_event_t *rev);
static void ss_ftp_process_cmds(ngx_event_t *rev);
static ngx_int_t ss_ftp_cmd_link_read(ngx_connection_t *c);
static ngx_int_t ss_ftp_data_link_read(ngx_connection_t *c);
static ngx_int_t ss_ftp_read_data(ngx_connection_t *c, ngx_buf_t *buf);
static ngx_int_t ss_ftp_parse_command(ss_ftp_request *r);
static ngx_int_t ss_ftp_process_command(ss_ftp_request *r); 
static void ss_to_lower(u_char *str, ngx_int_t len);
void ss_ftp_cmd_link_write(ngx_event_t *send);
ngx_int_t ss_ftp_data_link_write(ngx_connection_t *c);
void ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
ngx_int_t ss_ftp_data_link_add_chain(ngx_connection_t *c, ngx_chain_t *chain);
static ngx_int_t ss_ftp_write(ngx_connection_t *c, ngx_chain_t *current_chain, ngx_chain_t *in);
void ss_ftp_init_data_connection(ngx_connection_t *c);
static void ss_ftp_process_data_link_read(ngx_event_t *rev);
static void ss_ftp_process_data_link_write(ngx_event_t *rev);

void 
ss_ftp_init_control_connection(ngx_connection_t *c)
{
  dup2(stand_err, STDERR_FILENO);
  printf("%s\n", "init controll connnetion");  

  assert(c != NULL);
  assert(SS_FTP_CONTROL_CONN == c->ftp_conn_type);

  ngx_event_t *rev;
  ngx_event_t *send;

  rev = c->read;
  send = c->write;
  rev->handler = ss_ftp_init_request;
  send->handler = ss_ftp_cmd_link_write;

  ngx_handle_read_event(rev, 0);
  ngx_handle_write_event(send, 0);

  /* Welcome message  */
  char welcome[] = "220 (ssftp) beta\r\n";
  int rc = write(c->fd, welcome, strlen(welcome));
  rc++;
  //ss_ftp_reply(r, "hello, my friend. SSftp welcome you!");
  //ss_ftp_init_request(rev);
  ss_ftp_create_commands_hash_table(c->pool); 
}

static void
ss_ftp_init_request(ngx_event_t *rev)
{
  printf("%s\n", "init request");  

  ngx_connection_t  *c;
  ss_ftp_request    *r;

  c = rev->data;
  assert(c != NULL && c->pool != NULL);
  r = ngx_pcalloc(c->pool, sizeof(ss_ftp_request));
  c->data = r;

  r->pool = ngx_create_pool(SS_FTP_REQUEST_DEFAULT_POOL_SIZE, c->log);
  /* TODO : error handling  */
  
  r->connection = c;
  r->state = 0;
  r->cmd_buf = ngx_create_temp_buf(r->pool, SS_FTP_CMD_DEFAULT_BUF_LEN); 
  r->cmd_link_write = ngx_pcalloc(r->pool, sizeof(ngx_chain_t)); 
  r->cmd_link_write->buf = ngx_create_temp_buf(r->pool, SS_FTP_CMD_DEFAULT_BUF_LEN); 
  r->cmd_link_write->next = NULL;

  r->current_dir = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
  r->current_dir->data  = ngx_pcalloc(r->pool, sizeof(ss_ftp_home_dir));
  r->current_dir->len   = sizeof(ss_ftp_home_dir);
  ngx_memcpy(r->current_dir->data, ss_ftp_home_dir, sizeof(ss_ftp_home_dir));

  /* TODO :  change 10 to better value  */
  r->cmd_args = ngx_array_create(r->pool, 10, sizeof(ngx_str_t));
  /* TODO : initialize r */
  

  rev->handler = ss_ftp_process_cmds;

  ss_ftp_process_cmds(rev);
}

static void 
ss_ftp_process_cmds(ngx_event_t *rev)
{
  ngx_int_t         rc;
  ngx_connection_t  *c;
  ss_ftp_request    *r;

  c = (ngx_connection_t *) rev->data;
  assert(c != NULL && c->data != NULL);
  r = c->data; 

  for ( ;; ) {

      rc = ss_ftp_cmd_link_read(c);
      
      if (SS_AGAIN == rc || SS_ERROR == rc) {
          return; 
      }

      rc =  ss_ftp_parse_command(r);      

      
      if (SS_AGAIN == rc) {
          continue; 
      }

     /* TODO :error handling */

      if (SS_FTP_INVALID_COMMAND == rc) {

          /*  TODO : hanlde invalid command */ 

         return;
      }

      rc = ss_ftp_process_command(r);
      /* TODO : decide a better place to put it*/
      r->cmd_args = ngx_array_create(r->pool, 10, sizeof(ngx_str_t));
  }
}

static ngx_int_t
ss_ftp_cmd_link_read(ngx_connection_t *c)
{
   assert(c != NULL);
   ngx_int_t         rc;
   ss_ftp_request   *r;

   r = (ss_ftp_request *) c->data;
   assert(r != NULL && r->cmd_buf != NULL);
   rc = ss_ftp_read_data(c, r->cmd_buf);
   assert(rc > 0 || rc == SS_AGAIN || rc == SS_ERROR || rc == SS_BUF_FULL);

   /* All buffered data have been processed, and buffer becomes full */
   if (SS_BUF_FULL == rc) { 
      /* TODO : save command to another buffer */       
   }
   
   /* TODO  */
   /* just for passing compilation */
   return rc;
}

static ngx_int_t
ss_ftp_data_link_read(ngx_connection_t *c)
{
   assert(c != NULL);
   ngx_int_t         rc;
   ss_ftp_send_receive_cmd *srcmd;
   ngx_buf_t        *buf;

   srcmd = c->send_receive_cmd; 
   buf = srcmd->read_buf;
   assert(srcmd != NULL);
   assert(buf != NULL);
   rc = ss_ftp_read_data(c, buf);
   assert(rc > 0 || rc == SS_AGAIN || rc == SS_ERROR || rc == SS_BUF_FULL);

   /* All buffered data have been processed, and buffer becomes full */
   if (SS_BUF_FULL == rc) {
      buf->pos = buf->start;
      buf->last = buf->start; 

      rc = ss_ftp_read_data(c, buf);
      assert(rc > 0 || rc == SS_AGAIN || rc == SS_ERROR);
   }

   if (rc > 0 || rc == SS_AGAIN) {
      return rc; 
   }

   /* Error */
   /* TODO : consider putting error handling here or throw to caller */ 
    return rc;
}

static ngx_int_t
ss_ftp_read_data(ngx_connection_t *c, ngx_buf_t *buf)
{
   /* TODO : modify to make it suitable for both cmd and data link reading  */
   /* TODO : allocate more space if needed */

   assert(c != NULL);
   assert(buf != NULL);
  
   ngx_int_t           n, n_free;
   ngx_event_t        *rev;

   rev = c->read;

   n = buf->last - buf->pos;
   assert(n >= 0);
   if (n > 0) {
      return n; 
   }

   n_free = buf->end - buf->last;
   assert(n_free >= 0);
   if (0 == n_free) {
      return SS_BUF_FULL; 
   }

   if (rev->ready) {
      n = c->recv(c, 
                  buf->last, 
		  buf->end - buf->last); 
   } else {
      n = SS_AGAIN; 
   }

   if (SS_AGAIN == n) {
      ngx_handle_read_event(rev, 0); 

      /* TODO : error handling  */

      return SS_AGAIN;
   }

   if (0 == n || SS_ERROR == n) {

      /* TODO : error handling  */

      return SS_ERROR; 
   }

   buf->last += n;

   return n;
}

static ngx_int_t
ss_ftp_parse_command(ss_ftp_request *r) 
{
   assert(r != NULL);

   enum {
         state_start,
	 state_skip_telnet_chars,
	 state_name,
	 state_space_before_arg,
	 state_arg,
	 state_almost_done,
	 state_done
   } state;

   state = r->state;
   ngx_int_t stcs = r->skipped_tel_chars;

   u_char *p, ch;
   ngx_str_t *arg;
   ngx_buf_t *buf = r->cmd_buf;
   for (p = buf->pos; p < buf->last; p++) {
       ch = *p;

      switch (state) {
        
      case state_start:
          /* roll back */
           p--; 
           state = state_skip_telnet_chars;
	   break;

      case state_skip_telnet_chars:

           /*  skip telnet control characters */

	   if (0 == stcs) {

	      if (255 == ch) {
	         stcs =2;

	      } else {
		 r->cmd_name_start = p;
	         state = state_name;
	      }

	   } else {
	      stcs--; 
	   }

	   break;

      case state_name:

           /* parse command name */

	   if (' ' == ch) {
	      r->cmd_name_end = p-1; 
              state = state_space_before_arg;

	   } else if ('\r' == ch) {
	      r->cmd_name_end = p-1; 
	      state = state_almost_done; 

	   } else {

	      /* comand name character, continue */ 
	   }

	   break;
      
      case state_space_before_arg:

           /* skip SPACEs before argument */

           if ('\r' == ch) {
	      state =state_almost_done;    

	   } else if (' ' != ch) {
	      r->cmd_arg_start = p; 
	      state = state_arg;

	   } else {

	      /* skip spaces */ 
	   }

	   break;

      case state_arg:

           /* parse arguments */

	   if (' ' == ch) {
	      state = state_space_before_arg; 

	   } else if ('\r' == ch) {
	      state = state_almost_done;

	   } else {

	     /*  argument character  */ 

	     break;
	   }

	   r->cmd_arg_end = p-1;
	   arg = (ngx_str_t *) ngx_array_push(r->cmd_args);
	   //arg->data = r->cmd_arg_start;
	   arg->len  = r->cmd_arg_end - r->cmd_arg_start + 1;
           arg->data = ngx_pcalloc(r->pool, arg->len);
           strncpy((char *) arg->data, (const char *) r->cmd_arg_start, arg->len);

	   break;

      case state_almost_done:

           /* received '\r', waiting for '\n' */
           
	   if ('\n' == ch) {
               state = state_done;

               /* update command buffer pointer  */
               r->cmd_buf->pos = p + 1;

	       break;
	   }

	   return SS_FTP_INVALID_COMMAND;

      case state_done:
           goto done;           

      default :

           /* error handling  */

           break; 
    }
  }


done:

  r->state = state;
  r->skipped_tel_chars = stcs;

  if (state_done == state) {
      r->state = state_start;
      return SS_FTP_PARSE_ONE_COMMAND_DONE; 

  } else {
      return SS_AGAIN;  
  }

}

static ngx_int_t
ss_ftp_process_command(ss_ftp_request *r) 
{
   assert(r != NULL);

   ss_ftp_command *sfcmd;
   ngx_uint_t     key;
   u_char         *cmd_name;
   ngx_int_t      cmd_len;

   cmd_name = r->cmd_name_start;
   cmd_len  = r->cmd_name_end - r->cmd_name_start +1;
   assert(cmd_name != NULL && cmd_len > 0);

   key = ngx_hash_key_lc(cmd_name, cmd_len);
   ss_to_lower(cmd_name, cmd_len);
   sfcmd = (ss_ftp_command *) ngx_hash_find(ss_ftp_cmds_hash_table,
                                            key,
			                    cmd_name,
			                    cmd_len);

   if (NULL != sfcmd) {
      /* A PASV or PORT command arrived before */
      if (NULL != r->send_receive_cmd) {
          r->send_receive_cmd->cmd = sfcmd; 
      }
      sfcmd->execute(r);

   } else {
      ss_ftp_undefined_cmd(r);  
   }
  
   return NGX_OK;

   /* handler return value  */
} 

static void 
ss_to_lower(u_char *str, ngx_int_t len)
{
   assert(str != NULL && len > 0);

   ngx_int_t count = 0;
   for (count = 0; count < len; count++) {
       str[count] = tolower(str[count]); 
   }
}

void 
ss_ftp_cmd_link_write(ngx_event_t *send)
{
   ngx_connection_t  *c;
   ss_ftp_request    *r;
   ngx_int_t         rc;

   c = (ngx_connection_t *) send->data;
   assert(c != NULL);
   r = (ss_ftp_request *) c->data;
   assert(r != NULL);

   rc = ss_ftp_write(c, r->cmd_link_write, NULL); 
   assert(rc == NGX_AGAIN || rc == NGX_OK);
   if (NGX_AGAIN == rc) {
      ngx_handle_write_event(send, 0);

   /* Data transmited ok. */
   } else {
      /* TODO  */	   
     // ngx_del_event();    
   }
}

ngx_int_t 
ss_ftp_data_link_write(ngx_connection_t *data_conn)
{
   printf("%s\n", "data link write");
   assert(data_conn != NULL);

   //ss_ftp_request    *r;
   assert(data_conn->data != NULL);
   //r = (ss_ftp_request *) data_conn->data;
   assert(data_conn->send_receive_cmd != NULL);

   ngx_chain_t *chain = ((ss_ftp_send_receive_cmd *) (data_conn->send_receive_cmd))->chain;
   assert(chain != NULL);

   return ss_ftp_write(data_conn, chain, NULL); 
}

void
ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain)
{
   assert(r != NULL);

   ngx_event_t  *send;
   ngx_connection_t *conn;
   ngx_int_t     rc;

   conn = (ngx_connection_t *) r->connection;
   rc = ss_ftp_write(conn, r->cmd_link_write, chain);
   assert(rc == NGX_AGAIN || rc == NGX_OK);
   if (NGX_AGAIN == rc) {
      send = conn->write;
      ngx_handle_write_event(send, 0);

   /* Data transmited ok. */
   } else {
      /* TODO  */	   
      //ngx_del_event();    
   }
}

ngx_int_t
ss_ftp_data_link_add_chain(ngx_connection_t *data_conn, ngx_chain_t *chain)
{
   assert(data_conn != NULL);

   ss_ftp_send_receive_cmd *srcmd;
   srcmd = data_conn->send_receive_cmd;
   assert(srcmd != NULL);

   return ss_ftp_write(srcmd->data_connection, srcmd->chain, chain);
}


static ngx_int_t 
ss_ftp_write(ngx_connection_t *c, ngx_chain_t *current_chain, ngx_chain_t *in)
{
   assert(c != NULL);

   ngx_chain_t       **ll;
   ngx_chain_t       **fl;
   ngx_chain_t       *temp;
   ngx_chain_t       *temp2;
   ngx_chain_t       *chain;
   ngx_int_t          ctype;
   //ngx_event_t       *send;
   ss_ftp_request    *r;

   r = (ss_ftp_request *) c->data;
   ctype = c->ftp_conn_type;
   assert(SS_FTP_CONTROL_CONN == ctype || SS_FTP_DATA_CONN == ctype);

   if (SS_FTP_CONTROL_CONN == ctype) {
      ll = &r->cmd_link_write; 

   } else {
      ll = &((ss_ftp_send_receive_cmd *) (c->send_receive_cmd))->chain; 
   } 

   fl = ll;

   for (temp = current_chain; temp; temp = temp->next) {
       ll = &temp->next; 
   }

   *ll = in;

   //c = (ngx_connection_t *) r->connection;
   chain = c->send_chain(c, *fl, 0);

   for (temp = *fl; temp && temp != chain; /* void */){
       temp2 = temp->next;
       ngx_free_chain(r->pool, temp);
       temp = temp2;
   }

   *fl = temp;
   /* All chains have been sent */
   if (NULL == *fl) {
      return NGX_OK; 
   }   
   //send = c->write;
  // ngx_handle_write_event(send, 0);

   /* check whether it is suitable to put it here  */
   /* error handling  */

   return NGX_AGAIN;
}

void 
ss_ftp_init_data_connection(ngx_connection_t *c) {
   printf("%s\n", "init data connection");

   assert(c != NULL);
   assert(SS_FTP_DATA_CONN == c->ftp_conn_type);

   ngx_event_t *rev;
   ngx_event_t *send;
   ss_ftp_request *r;
   ss_ftp_send_receive_cmd **r_srcmd;
   ss_ftp_send_receive_cmd *dc_srcmd;

   r = (ss_ftp_request *) c->data;
   assert(r != NULL);
   // for test, for keep work, not comment it
  // assert(r->send_receive_cmd->data_connection == NULL);
   r->send_receive_cmd->data_connection = c;
   r_srcmd = &r->send_receive_cmd;
   assert(*r_srcmd != NULL);
   /* Corresponding command have't come yet */
   if (NULL == (*r_srcmd)->chain) {
   printf("%s\n", "command not come");
      return;
   }

   c->send_receive_cmd = *r_srcmd; 
   *r_srcmd = NULL; 

   dc_srcmd = c->send_receive_cmd;
   assert( dc_srcmd->type == SS_FTP_RECEIVE_CMD 
        || dc_srcmd->type == SS_FTP_SEND_CMD);

   if (SS_FTP_RECEIVE_CMD == dc_srcmd->type) {
      printf("%s\n", "read type command");  

      rev = c->read;
      rev->handler = ss_ftp_process_data_link_read;
     // ngx_handle_read_event(rev, 0);
      ss_ftp_process_data_link_read(c->read);

   } else {
     printf("%s\n", "write type command");  

     send = c->write;
     send->handler = ss_ftp_process_data_link_write;
     //ngx_handle_write_event(send, 0);
     ss_ftp_process_data_link_write(c->write);
   }
}

static void 
ss_ftp_process_data_link_read(ngx_event_t *rev) {
   printf("%s\n", "process data link read");

   ngx_int_t          rc;
   ngx_connection_t  *data_conn;

   data_conn = (ngx_connection_t *) rev->data;

   rc = ss_ftp_data_link_read(data_conn);
   assert(rc == NGX_AGAIN || rc == NGX_OK);
   if (NGX_AGAIN == rc) {
      ngx_handle_read_event(rev, 0); 
      return;
   }

   /* Transmittion complete. */
   /* TODO : reply to client, close data connection */


}

static void 
ss_ftp_process_data_link_write(ngx_event_t *send) {
   printf("%s\n", "process data link write");

   ngx_int_t          rc;
   ngx_connection_t  *data_conn;

   data_conn = (ngx_connection_t *) send->data;

   rc = ss_ftp_data_link_write(data_conn);
   assert(rc == NGX_AGAIN || rc == NGX_OK);
   if (NGX_AGAIN == rc) {
      ngx_handle_write_event(send, 0); 
      return;
   }

   printf("%s\n", "about to close data connection");
   close(data_conn->fd);
   ss_ftp_reply(data_conn->data, FILE_ACTION_OK, FILE_ACTION_OK_M);
}


#endif /* _SSFTP_C */
