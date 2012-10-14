/*
 * Copyright(c) Wu Jiang <wujiangthu@gmail.com>
 */


#ifndef _SS_TIME_C_
#define _SS_TIME_C_


/* file - struct stat  */
#define GET_FILE_TYPE(file)  S_ISDIR(file.st_mode) != 0 ? 'd' : \
                             (S_ISREG(file.st_mode) != 0 ? '-' : 'l') 


#endif /* _SS_TIME_C  */
