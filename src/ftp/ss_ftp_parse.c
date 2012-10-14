/*
 * Copyright(c) Wu Jiang <wujiangthu@gmail.com>
 */

#ifndef  _SS_FTP_PARSE_C_
#define  _SS_FTP_PARSE_C_


#include "ss_ftp_core.h"
#include "ss_ftp_parse.h"


#define SS_FTP_PARSE_PATH_OK                              0
#define SS_FTP_PARSE_PATH_NOT_BEGIN_WITH_SLASH_ERROR     -1
#define SS_FTP_PARSE_PATH_FILENAME_TOO_LONG_ERROR        -2
#define SS_FTP_PARSE_PATH_UNKNOWN_STATE_ERROR            -3
#define SS_FTP_PARSE_PATH_SYNTAX_ERROR                   -4



void ss_ftp_process_out_of_memory(ss_ftp_request *r);

ngx_int_t ss_ftp_parse_command(ss_ftp_request *r);
ngx_int_t ss_ftp_parse_path(ss_path_t *file_path);
void ss_set_type(u_char *c, ngx_int_t type);
void ss_set_nsp(u_char *c, ngx_int_t nsp);


ngx_int_t ss_ftp_parse_command(ss_ftp_request *r)
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

           r->cmd_args = ngx_array_create(r->pool, 10, sizeof(ngx_str_t));
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
           arg->len  = r->cmd_arg_end - r->cmd_arg_start + 1;
           arg->data = ngx_pcalloc(r->pool, arg->len);
           if (NULL == arg->data) {
              ss_ftp_process_out_of_memory(r);
           }

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

ngx_int_t 
ss_ftp_parse_path(ss_path_t *file_path)
{
   assert(NULL == file_path);

   enum {
         begin,
         begin_after_normal,
         during_normal, 
         begin_after_shrink,
         shrink_processed_one_dot,
         shrink_processed_two_dot,
         end
   } state;

   size_t    pos;
   u_char   *fpath;
   u_char    current;
   int       shrink_point = -1;
   size_t    file_name_len = 0;

   fpath = file_path->path;
   state = begin;  
   for (pos = 0; pos < file_path->plen; pos ++) {
       current = fpath[pos];

       switch (state) {

       case begin :
           
            /* roll back */

            if ('/' != current) {

               return SS_FTP_PARSE_PATH_NOT_BEGIN_WITH_SLASH_ERROR;
            }

            state = begin_after_normal;
     
            break;
      
       case begin_after_normal:
            if ('.' == current) {
               assert(-1  == shrink_point);

               shrink_point = pos - 1;              
               state = shrink_processed_one_dot; 

               break;
            }

            file_name_len++;            
            state = during_normal; 

            break;

       case during_normal:
            if ('/' == current) {
               state = begin_after_normal;

               break;
            }            
            
            file_name_len++;
            if (file_name_len > 255) {

               return SS_FTP_PARSE_PATH_FILENAME_TOO_LONG_ERROR;
            }

            break;

       case shrink_processed_one_dot:
            if ('.' == current) {
               state = shrink_processed_two_dot;

               break;
            }  

            if ('/' == current) {
               state = begin_after_shrink;

               /* TODO : process shrinking */               

               break;
            }

            return SS_FTP_PARSE_PATH_SYNTAX_ERROR;

       case shrink_processed_two_dot:
            if ('/' != current) {
               
               return SS_FTP_PARSE_PATH_SYNTAX_ERROR;
            }

            state = begin_after_shrink;

            break;              

       case begin_after_shrink:

            /* TODO  */ 

            break;              

       case end:
            
           /* TODO  */
           
            break;

       default:
            
            return SS_FTP_PARSE_PATH_UNKNOWN_STATE_ERROR;
       }
   }
   
   return SS_FTP_PARSE_PATH_OK;
}

void 
ss_set_type(u_char *c, ngx_int_t type)
{
   assert(NULL != c);
   assert(   SINGLE       == type 
          || DOUBLE       == type
          || MUTIL_SINGLE == type
          || MUTIL_DOUBLE == type
          || MIX          == type
       );

  *c &= 0x1F; 
  *c += ((u_char) type - 1) << 4;
}

void
ss_set_nsp(u_char *c, ngx_int_t nsp)
{
   assert(NULL != c);
   assert(nsp >= 0);
   assert(nsp < 2097152); /* 2097152 = 2M */

   *c &= 0xE0;
   *(c + 1) &= 0x00;
   *(c + 2) &= 0x00;

   *c += ((unsigned int) nsp) >> 16;     
   *(c + 1) += (((unsigned int) nsp) << 16) >> 24;
   *(c + 2) += (((unsigned int) nsp) << 24) >> 24;
}


#endif /* _SS_FTP_PARSE_C_ */
