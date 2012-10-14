/*
 * Copyright (C) Wu Jiang <wujiangthu@gmail.com>
 */


#include "ss_ftp_core.h"
#include "ss_ftp_pam.h"
#include <security/pam_appl.h>
#include <security/pam_misc.h>


#ifndef _SS_FTP_PAM_C_
#define _SS_FTP_PAM_C_


int ss_ftp_conversation(int n, const struct pam_message **msg,
                        struct pam_response **resp, void *data);


int
ss_ftp_conversation(int n, const struct pam_message **msg,
                    struct pam_response **resp, void *data)
{
   assert(NULL != data);

   struct pam_response *aresp;
   ss_ftp_request      *r;

   r = (ss_ftp_request *)data;

   if (n <= 0 || n > PAM_MAX_NUM_MSG) {
      return (PAM_CONV_ERR);
   }

   if ((aresp = calloc(n, sizeof(struct pam_response))) == NULL) {
      return (PAM_BUF_ERR);
   }

   int rc = NGX_OK;
   int i;
   for (i = 0; i < n && NGX_OK == rc; i++) {
       aresp[i].resp_retcode = 0;
       aresp[i].resp = NULL;

       switch (msg[i]->msg_style) {

       case PAM_PROMPT_ECHO_OFF:

            /* Set password */

            aresp[i].resp = strdup(r->password);
            if (NULL == aresp[i].resp){
               rc = NGX_ERROR;
            }

            break;

       case PAM_PROMPT_ECHO_ON:

            /* Set user name  */

            aresp[i].resp = strdup(r->username);
            if (aresp[i].resp == NULL) {
                rc = NGX_ERROR;
            }

            break;

       case PAM_ERROR_MSG:
       case PAM_TEXT_INFO:
            break;

       default:
            rc = NGX_ERROR;
            break;

       }
    }

    if (NGX_OK == rc) {
       *resp = aresp;
       return (PAM_SUCCESS);
    }

    for (i = 0; i < n; ++i) {
        if (aresp[i].resp != NULL) {
            memset(aresp[i].resp, 0, strlen(aresp[i].resp));
            free(aresp[i].resp);
        }
    }

    memset(aresp, 0, n * sizeof *aresp);
    *resp = NULL;

    return (PAM_CONV_ERR);
}


#endif /* _SS_FTP_PAM_C_ */

