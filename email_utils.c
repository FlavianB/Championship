#include "email_utils.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

static char payload_text[5000];

struct upload_status {
    size_t bytes_read;
};

// Createas a payload for the email.
static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp) {
    struct upload_status *upload_ctx = (struct upload_status *)userp;
    const char *data;
    size_t room = size * nmemb;
    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
        return 0;
    }
    data = payload_text + upload_ctx->bytes_read;
    if (data) {
        size_t len = strlen(data);
        if (room < len)
            len = room;
        memcpy(ptr, data, len);
        upload_ctx->bytes_read += len;
        return len;
    }
    return 0;
}

int send_email(const char *to_email, const char *subject, const char *body) {
    memset(payload_text, 0, sizeof(payload_text));
    sprintf(payload_text,
            "Date: Mon, 5 Dec 2023 21:54:29 +2000\r\n"
            "To:  %s \r\n"
            "From: " FROM_MAIL "\r\n"
            "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@>\r\n"
            "Subject: %s \r\n"
            "\r\n"
            "%s \r\n"
            "\r\n",
            to_email, subject, body);

    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    struct upload_status upload_ctx = {0};
    curl = curl_easy_init();
    if (curl) {
        // Set the URL for the SMTP server.
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");

        // Email address of the sender.
        curl_easy_setopt(curl, CURLOPT_USERNAME, "admin@champ-manager.com");
        // Authorization code for the email address.
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "jtcb dmim xpyo fvlt ");

        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM_ADDR);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        recipients = curl_slist_append(recipients, to_email);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        curl_slist_free_all(recipients);

        curl_easy_cleanup(curl);
    }
    return (int)res;
}
