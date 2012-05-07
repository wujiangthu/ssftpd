
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

#define COMMAND_OK                 "200 Command ok.\r\n" 
#define COMMAND_NOT_IMPLEMENTED    "202 Command not implemented.\r\n" 
#define SERVICE_READY              "220 Service is ready.\r\n" 
#define USER_LOGGED_IN             "230 Logged in.\r\n" 
#define USER_LOGGED_OUT            "231 Logged out.\r\n" 
#define USER_DELAYED_LOGGED_OUT    "232 Logged out command received,   \ 
                                        will complete when transfer done.\r\n" 
#define PATH_CREATED               "257 Path created.\r\n"    

/* 
 *  3XX
 *
 *  Positive Intermediate Replies  
 */

#define USER_NAME_OK_NEED_PASSWORD "331 Need password.\r\n" 
#define NEED_ACCOUNT               "332 Need account.\r\n" 


/*
 *  4XX
 * 
 *  Transient Negative Replies  
 */

#define INVALID_USERNAME_PASSWORD  "430 Invalid username or password.\r\n" 
#define HOST_UNAVAILABLE           "434 Requested host unavailable.\r\n" 


/*
 *  5XX
 * 
 *  Permanent Negative Completion Replies  
 */

#define COMMAND_SYNTAX_ERROR       "500 Syntax error,command unrecognized.\r\n" 
#define ARGUMENT_SYNTAX_ERROR      "501 Arguments syntax error.\r\n" 


#endif /* _SS_FTP_REPLY_H_  */
