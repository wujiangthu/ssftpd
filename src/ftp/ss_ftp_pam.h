/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#include <security/pam_appl.h>
#include <security/pam_misc.h>


#ifndef _SS_FTP_PAM_H_
#define _SS_FTP_PAM_H_


int ss_ftp_conversation(int n, const struct pam_message **msg,
                        struct pam_response **resp, void *data);


#endif /* _SS_FTP_PAM_H_ */


