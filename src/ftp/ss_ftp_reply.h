
/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_FTP_REPLY_H_
#define _SS_FTP_REPLY_H_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


void ss_ftp_reply(ss_ftp_request *r, const char *reply_code, char *reply_message);


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

#define COMMAND_OK                 "200" 
#define COMMAND_OK_M                 "Command ok." 
#define COMMAND_NOT_IMPLEMENTED    "202" 
#define COMMAND_NOT_IMPLEMENTED_M    "Command not implemented." 
#define SERVICE_READY              "220" 
#define SERVICE_READY_M              "Service is ready." 
#define ENTERING_PASSIVE_MODE      "227" 
#define ENTERING_PASSIVE_MODE_M     "Entering Passive Mode." 
#define USER_LOGGED_IN             "230" 
#define USER_LOGGED_IN             "230" 
#define USER_LOGGED_IN_M             "Logged in." 
#define USER_LOGGED_OUT            "231" 
#define USER_LOGGED_OUT_M            "Logged out." 
#define USER_DELAYED_LOGGED_OUT    "232"
#define USER_DELAYED_LOGGED_OUT_M    "Logged out command received, \
                                        will complete when transfer done." 
#define PATH_CREATED               "257"    
#define PATH_CREATED_M               "Path created."    

/* 
 *  3XX
 *
 *  Positive Intermediate Replies  
 */

#define USER_NAME_OK_NEED_PASSWORD "331" 
#define USER_NAME_OK_NEED_PASSWORD_M "Need password." 
#define NEED_ACCOUNT               "332" 
#define NEED_ACCOUNT_M               "Need account." 


/*
 *  4XX
 * 
 *  Transient Negative Replies  
 */

#define INVALID_USERNAME_PASSWORD  "430 Invalid username or password." 
#define HOST_UNAVAILABLE           "434 Requested host unavailable." 


/*
 *  5XX
 * 
 *  Permanent Negative Completion Replies  
 */

#define COMMAND_SYNTAX_ERROR       "500 Syntax error,command unrecognized." 
#define ARGUMENT_SYNTAX_ERROR      "501 Arguments syntax error." 


#endif /* _SS_FTP_REPLY_H_  */