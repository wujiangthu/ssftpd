
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

#define FILE_STATUS_OK          "150"
#define FILE_STATUS_OK_M        " " 


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
#define CLOSING_CONTROL_CONNECTION "221"
#define CLOSING_CONTROL_CONNECTION_M "Closing control connection." 
#define FILE_ACTION_OK              "226" 
#define FILE_ACTION_OK_M            "File action ok, closing data connection." 
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

#define SERVICE_NOT_AVAILABLE "421" 
#define SERVICE_NOT_AVAILABLE_M "Service not avalaible, closing control connection." 
#define INVALID_USERNAME_PASSWORD  "430 Invalid username or password." 
#define HOST_UNAVAILABLE           "434 Requested host unavailable." 
#define STORAGE_INSUFFICIENT "452" 
#define STORAGE_INSUFFICIENT_M "Requested action ot taken. Insufficient storage space in system." 


/*
 *  5XX
 * 
 *  Permanent Negative Completion Replies  
 */

#define COMMAND_SYNTAX_ERROR       "500 Syntax error,command unrecognized." 
#define ARGUMENT_SYNTAX_ERROR      "501 Arguments syntax error." 
#define FILE_UNAVAILABLE          "550" 
#define FILE_UNAVAILABLE_M        "FILE OPERATION ERROR" 


#endif /* _SS_FTP_REPLY_H_  */
