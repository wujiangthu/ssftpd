
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SSFTP_C_
#define _SSFTP_C_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ctype.h>

#include "ssftp.h"
#include "ss_ftp_cmd.h"
#include "ss_ftp_cmd.c"
#include "ss_ftp_reply.h"

#define  SS_AGAIN                         NGX_AGAIN
#define  SS_ERROR                         NGX_ERROR
#define  SS_FTP_PARSE_ONE_COMMAND_DONE    1
#define  SS_FTP_INVALID_COMMAND           2

#define SS_FTP_REQUEST_DEFAULT_POOL_SIZE  1024*10
#define SS_FTP_CMD_DEFAULT_BUF_LEN        1024


void ss_ftp_init_control_connection(ngx_connection_t *c);
static void ss_ftp_init_request(ngx_event_t *rev);
static void ss_ftp_process_cmds(ngx_event_t *rev);
static ngx_int_t ss_ftp_read_data(ss_ftp_request *r);
static ngx_int_t ss_ftp_parse_command(ss_ftp_request *r);
static ngx_int_t ss_ftp_process_command(ss_ftp_request *r); 
static void ss_to_lower(u_char *str, ngx_int_t len);
void ss_ftp_cmd_link_write(ngx_event_t *send);
void ss_ftp_data_link_write(ngx_event_t *send);
void ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
void ss_ftp_data_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain);
static void ss_ftp_write(ss_ftp_request *r, ngx_chain_t *current_chain, ngx_chain_t *in);
void ss_ftp_init_data_connection(ngx_connection_t *c);
static void ss_ftp_process_data_link(ngx_event_t *rev);

void 
ss_ftp_init_control_connection(ngx_connection_t *c)
{
  ngx_event_t *rev;
  ngx_event_t *send;

  rev = c->read;
  send = c->write;
  rev->handler = ss_ftp_init_request;
  send->handler = ss_ftp_cmd_link_write;

  ngx_handle_read_event(rev, 0);
  ngx_handle_write_event(send, 0);

  /* Welcome message  */
  char welcome[] = "220 hello, my friend. SSftp welcome you!\r\n";
  int rc = write(c->fd, welcome, strlen(welcome) -1);
  rc++;
  //ss_ftp_reply(r, "hello, my friend. SSftp welcome you!");
  //ss_ftp_init_request(rev);
  ss_ftp_create_commands_hash_table(c->pool); 
}

static void
ss_ftp_init_request(ngx_event_t *rev)
{
  ngx_connection_t  *c;
  ss_ftp_request    *r;

  c = rev->data;
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

  c = rev->data;
  r = c->data; 

  for ( ;; ) {

      rc = ss_ftp_read_data(r);
      
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
  }
}

static ngx_int_t
ss_ftp_read_data(ss_ftp_request *r)
{

   /* TODO : allocate more space if needed */

   ngx_int_t           n;
   ngx_event_t        *rev;
   ngx_connection_t   *c;

   c = r->connection;
   rev = c->read;

   n = r->cmd_buf->last - r->cmd_buf->pos;

   if (n > 0) {
      return n; 
   }

   if (rev->ready) {
      n = c->recv(c, 
                  r->cmd_buf->last, 
		  r->cmd_buf->end - r->cmd_buf->last); 
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

   r->cmd_buf->last += n;

   return n;
}

static ngx_int_t
ss_ftp_parse_command(ss_ftp_request *r) 
{
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
	   arg->data = r->cmd_arg_start;
	   arg->len  = r->cmd_arg_end - r->cmd_arg_start + 1;

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
   ss_ftp_command *sfcmd;
   ngx_uint_t     key;
   u_char         *cmd_name;
   ngx_int_t      cmd_len;

   cmd_name = r->cmd_name_start;
   cmd_len  = r->cmd_name_end - r->cmd_name_start +1;

   key = ngx_hash_key_lc(cmd_name, cmd_len);
   ss_to_lower(cmd_name, cmd_len);
   sfcmd = (ss_ftp_command *) ngx_hash_find(ss_ftp_cmds_hash_table,
                                            key,
			                    cmd_name,
			                    cmd_len);

   if (NULL != sfcmd) {
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
   ngx_int_t count = 0;

   if (NULL == str) {
      return; 

   } else {
      for (count = 0; count < len; count++) {
          str[count] = tolower(str[count]); 
      }
   }
}

void 
ss_ftp_cmd_link_write(ngx_event_t *send)
{
   ngx_connection_t  *c;
   ss_ftp_request    *r;

   c = (ngx_connection_t *) send->data;
   r = (ss_ftp_request *) c->data;

   ss_ftp_write(r, r->cmd_link_write, NULL); 
}

void 
ss_ftp_data_link_write(ngx_event_t *send)
{
   ngx_connection_t  *c;
   ss_ftp_request    *r;

   c = (ngx_connection_t *) send->data;
   r = (ss_ftp_request *) c->data;

   ss_ftp_write(r, r->data_link_write, NULL); 
}

void
ss_ftp_cmd_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain)
{
   ss_ftp_write(r, r->cmd_link_write, chain);
}

void
ss_ftp_data_link_add_chain(ss_ftp_request *r, ngx_chain_t *chain)
{
   ss_ftp_write(r, r->data_link_write, chain);
}


static void
ss_ftp_write(ss_ftp_request *r, ngx_chain_t *current_chain, ngx_chain_t *in)
{
   ngx_chain_t       **ll;
   ngx_chain_t       **fl;
   ngx_chain_t       *temp;
   ngx_chain_t       *temp2;
   ngx_chain_t       *chain;
   ngx_connection_t  *c;
   ngx_event_t       *send;

   if (current_chain == r->cmd_link_write) {
      ll = &r->cmd_link_write; 

   } else if (current_chain == r->data_link_write) {
      ll = &r->data_link_write; 

   } else {

      /* just for passing compilation  */

      ll = &r->data_link_write; 

      /* TODO : error handling  */ 
   }

   fl = ll;

   for (temp = current_chain; temp; temp = temp->next) {
       ll = &temp->next; 
   }

   *ll = in;

   c = (ngx_connection_t *) r->connection;
   chain = c->send_chain(c, *fl, 0);

   for (temp = *fl; temp && temp != chain; /* void */){
       temp2 = temp->next;
       ngx_free_chain(r->pool, temp);
       temp = temp2;
   }

   *fl = temp;
   
   send = c->write;
   ngx_handle_write_event(send, 0);

   /* check whether it is suitable to put it here  */
   /* error handling  */

   return;
}

void 
ss_ftp_init_data_connection(ngx_connection_t *c) {
   ngx_event_t *rev;
   ngx_event_t *send;

   rev = c->read;
   rev->handler = ss_ftp_process_data_link;

   send = c->write;
   send->handler = ss_ftp_data_link_write;

   ngx_handle_read_event(rev, 0);
   ngx_handle_write_event(send, 0);
}

static void 
ss_ftp_process_data_link(ngx_event_t *rev) {


}


#endif /* _SSFTP_C  */
