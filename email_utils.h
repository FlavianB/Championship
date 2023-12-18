#ifndef EMAIL_UTILS_H
#define EMAIL_UTILS_H

#define FROM_ADDR "admin@champ-manager.com"
#define FROM_MAIL "Championship Manager <" FROM_ADDR ">"

int send_email(char *to, char *subject, char *body);

#endif