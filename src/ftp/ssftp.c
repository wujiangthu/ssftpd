
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


typedef struct ss_ftp_request {
  ngx_connection_t  *connection;
  ngx_pool_t        *pool;

  ngx_int_t         *state;
  u_char            *cmd_name_start; 
  u_char            *cmd_name_end; 
  u_char            *cmd_arg_start; 
  u_char            *cmd_arg_end; 
  ngx_array_s       *cmd_args; 
  ngx_buf_t         *cmd_buf

  ngx_chain_t       *cmd_link_write;
  ngx_chain_t       *data_link_write
} ss_ftp_request;

void 
ss_ftp_init_connection (ngx_connection_t *c)
{
  ngx_event_t *rev;
  ngx_event_t *send;

  rev = c->read;
  send = c->write;
  rev->handler = ss_ftp_init_request;
  send->handler = ss_ftp_cmd_link_write;

  ngx_handle_read_event(rev, 0);
  ngx_handle_write_event(send, 0);

  //ss_ftp_init_request(rev);
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
  
  r->state = 0;
  r->cmd_buf = ngx_pcalloc(r->pool, SS_FTP_CMD_DEFAULT_BUF_LEN); 
  r->cmd_link_write = ngx_pcalloc(r->pool, sizeof(ngx_chain_t)); 
  r->cmd_link_write->buf = ngx_pcalloc(r->pool, SS_FTP_CMD_DEFAULT_BUF_LEN); 
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

      if (SS_FTP_PARSE_INVALID_COMMAND == rc) {

          /*  TODO : hanlde invalid command */ 

         return;
      }

      rc = ss_ftp_process_command(r);
  }
}

ngx_int_t
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

   }

   r->cmd_buf->last += n;

   return n;
}

ngx_int_t
ss_ftp_parse_command(ss_ftp_request *r) 
{
   enum {
         state_start,
	 state_skip_telnet_chars,
	 state_name,
	 state_space_before_agr,
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

       if (state_done == state) {
          break; 
       }

       ch = *p;

      switch (state) {
        
      case state_start:
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
	   arg = (ngx_str_t *) ngx_array_push(cmd_args);
	   arg->data = cmd_arg_start;
	   arg->len  = cmd_arg_end - cmd_arg_start + 1;

	   break;

      case state_almost_done:

           /* received '\r', waiting for '\n' */
           
	   if ('\n' == ch) {
               state = state_done;
	       break;
	   }

	   return SS_FTP_INVALID_COMMAND;
    }
  }

  r->state = state;
  r->skipped_tel_chars = stcs;

  if (state_done == state) {
      return SS_FTP_PARSE_ONE_COMMAND_DONE; 

  } else {
      return SS_AGAIN;  
  }

}

ngx_int_t
ss_ftp_process_command(ss_ftp_request *r) 
{
   ss_ftp_command *sfcmd;
   ngx_uint_t     key;
   u_char         *cmd_name;
   ngx_int_t      cmd_len;

   cmd_name = r->cmd_name_start;
   cmd_len  = r->cmd_name_end - r->cmd_name_start +1;

   key = ngx_hash_key_lc(cmd_name, cmd_len);
   sfcmd = (ss_ftp_command *) ngx_hash_find(ss_ftp_cmds_hash_table,
                                            key,
			                    cmd_name,
			                    cmd_len);
   sfcmd->execute(r);
} 

void 
ss_ftp_cmd_link_write(ss_ftp_request *r)
{
  ss_ftp_write(r, r->cmd_link_write, NULL); 
}

void 
ss_ftp_data_link_write(ss_ftp_request *r)
{
  ss_ftp_write(r, r->data_link_write, NULL); 
}

void
ss_ftp_write(ss_ftp_request *r, ngx_chain_t *current_chain, ngx_chain_t *in)
{
   ngx_chain_t   *ll;
   ngx_chain_t   *fl;
   ngx_chain_t   *temp;
   ngx_chain_t   *temp2;
   ngx_chain_t   *chain;

   if (current_chain == r->cmd_link_write) {
      ll = &r->cmd_link_write; 

   } else if (current_chain == r->data_link_write) {
      ll = &r->data_link_write; 

   } else {

      /* TODO : error handling  */ 
   }

   fl = ll;

   for (temp = current_chain; temp; temp = temp->next) {
       ll = &temp->next; 
   }

   *ll = in;

   chain = c->send_chain(c, *fl, 0);

   for (temp = *fl; temp && temp != chain; /* void */){
       temp2 = temp->next;
       ngx_free_chain(r->pool, temp);
       temp = temp2;
   }

   *fl = temp;
   
   return;
}
