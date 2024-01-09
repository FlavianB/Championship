#ifndef EMAIL_UTILS_H
#define EMAIL_UTILS_H

#define FROM_ADDR "admin@champ-manager.com"
#define FROM_MAIL "Championship Manager <" FROM_ADDR ">"

// Sends an email to the specified address.
int send_email(const char *to, const char *subject, const char *body);

#endif