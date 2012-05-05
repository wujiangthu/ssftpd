typedef struct ss_ftp_request {
  ngx_connection_t  *connection;
  ngx_pool_t        *pool;

  ngx_int_t         *state;
  u_char            *cmd_name_start; 
  u_char            *cmd_name_end; 
  ngx_array_s       *cmd_args; 
  ngx_buf_t         *cmd_buf;
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
ss_ftp_init_request (ngx_event_t *rev)
{
  ngx_connection_t  *c;
  ss_ftp_request    *r;

  c = rev->data;
  r = ngx_pcalloc(c->pool, sizeof(ss_ftp_request));
  c->data = r;

  r->state = 0;
  /* TODO : initialize r */

  /* TODO: allocate space for request->poll */

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
ss_ftp_read_data(r)
{
   /* TODO : read data from socket */
   /* TODO : allocate more space if needed */
   return 0;
}

ngx_int_t
ss_ftp_parse_command(r) 
{
   enum {
         state_start,
	 state_skip_telnet_chars,
	 state_name,
	 state_space_before_agr,
	 state_arg,
	 state_end,
	 state_done
   } state;

   state = r->state;

   u_char *p, ch;
   ngx_buf_t *buf = r->cmd_buf;
   for (p = buf->pos; p < buf->last; p++) {
       ch = *p;

      switch (state) {
        
      case state_start:
           state = state_skip_telnet_chars;
	   break;
      }

      case state_skip_telnet_chars:
           /* TODO : skip telnet chars */
           state = state_name;
	   break;

      case state_name:
           /* TODO : parse command name */
           state = state_space_before_arg;
	   break;
      
      case state_space_before_arg:
           /* TODO : skip SPACE before arg */
           state = state_arg;
	   break;

      case state_arg:
           /* TODO : parse arg */
           state = state_end;
	   break;

      case state_end:
           /* TODO : parse terminating end chars */
           state = state_done;
	   break;
  }

  r->state = state;
  if (state_done == state) {
      return SS_FTP_PARSE_ONE_COMMAND_DONE; 

  } else {
      return SS_AGAIN;  
  }

}

ngx_int_t
ss_ftp_process_command(r) 
{

} 
