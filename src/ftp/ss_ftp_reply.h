
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_REPLY_H_
#define _SS_FTP_REPLY_H_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


void ss_ftp_reply(ss_ftp_request *r, const char *reply_code);


/* 
 *  1XX  
 *  
 *  Positive Preliminary Replies  
 */



/*
 *  2XX
 * 
 *  Positive Completion Replies  
 */

#define USER_LOGGED_IN             "230 Logged in.\r\n" 


/* 
 *  3XX
 *
 *  Positive Intermediate Replies  
 */

#define USER_NAME_OK_NEED_PASSWORD "331 Need password.\r\n" 


/*
 *  4XX
 * 
 *  Transient Negative Replies  
 */


/*
 *  5XX
 * 
 *  Permanent Negative Completion Replies  
 */



#endif /* _SS_FTP_REPLY_H_  */
