/*
 * Copyright(c) Wu Jiang <wujiangthu@gmail.com>
 */

#ifndef  _SS_FTP_PARSE_H_
#define  _SS_FTP_PARSE_H_


/*

PROTOCOL:

|00|01|02|03|04|05|06|07|           08|09|10|11|12|13|14|15|
|16|17|18|19|20|21|22|23|

00-03  Shrinking Type (ST)
04-23  Next Shrinking Point (NSP)

*Type*
// TODO : change to regular expressions
     000 : /abc/./abd 
     001 : /abc/../abc 
     010 : /abc/././+/abc
     011 : /abc/./.././+/abc
     100 :
"+" means repeating previous part 0 or more times.

*/

#define  SINGLE                           1
#define  DOUBLE                           2 
#define  MUTIL_SINGLE                     3 
#define  MUTIL_DOUBLE                     4 
#define  MIX                              5 
#define  UNKNOW                           6

#define  ss_check_type(x)  x >> 5 == 0x00 ? SINGLE : \
                           x >> 5 == 0x01 ? DOUBLE : \
                           x >> 5 == 0x02 ? MUTIL_SINGLE : \
                           x >> 5 == 0x03 ? MUTILE_DOUBLE : \
                           x >> 5 == 0x04 ? MIX : UNKNOW 
#define  ss_get_nsp(c)     ((unsigned int) (*c << 3)) << (16 - 3) \
                         + ((unsigned int) *(c + 1)) << 8       \
                         + ((unsigned int) *(c + 2))

 
#define  SS_AGAIN                         NGX_AGAIN
#define  SS_FTP_INVALID_COMMAND           2
#define  SS_FTP_PARSE_ONE_COMMAND_DONE    1


ngx_int_t ss_ftp_parse_command(ss_ftp_request *r);
ngx_int_t ss_ftp_parse_path(ss_path_t *file_path);
void ss_set_type(u_char *c, ngx_int_t type);
void ss_set_nsp(u_char *c, ngx_int_t nsp);


#endif /* _SS_FTP_PARSE_H_ */
