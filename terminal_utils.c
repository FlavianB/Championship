#include "terminal_utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct termios term;

void terminal_alter() {
    struct termios new;
    if (tcgetattr(fileno(stdin), &term) != 0) {
        perror("tcgetattr failed");
        exit(-1);
    }

    new = term;
    new.c_lflag &= ~ICANON;
    new.c_lflag &= ~ECHO;
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0) {
        perror("tcsetattr failed");
        exit(-1);
    }
}

void terminal_unalter() {
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &term) != 0) {
        perror("tcsetattr failed");
        exit(-1);
    }
}

void star_get(char *buf, size_t size) {
    char c = '\0';
    int count = 0;
    while (--size > 0 && (c = getchar()) != '\n') {
        if (isalpha(c) || isdigit(c)) {
            *buf++ = c;
            putchar('*');
            count++;
        } else if (c == 127 || c == '\b') {
            if (count > 0) {
                buf--;
                *buf = '\0';
                printf("\b \b");
                count--;
            }
        }
    }
    *buf = '\0';
}

void welcome_prompt() {
    printf("Welcome to...\n\n");
    printf("\033[1;35m");
    printf("                                                                              \n"
           "   _____  _                               _                     _      _        \n"
           "  / ____|| |                             (_)                   | |    (_)       \n"
           " | |     | |__    __ _  _ __ ___   _ __   _   ___   _ __   ___ | |__   _  _ __  \n"
           " | |     | '_ \\  / _` || '_ ` _ \\ | '_ \\ | | / _ \\ | '_ \\ / __|| '_ \\ | || '_ \\ \n"
           " | |____ | | | || (_| || | | | | || |_) || || (_) || | | |\\__ \\| | | || || |_) |\n"
           "  \\_____||_| |_| \\__,_||_| |_| |_|| .__/ |_| \\___/ |_| |_||___/|_| |_||_|| .__/ \n"
           "                                  | |                                    | |    \n"
           "                                  |_|                                    |_|    \n"
           "                __  __                                                          \n"
           "               |  \\/  |                                                         \n"
           "               | \\  / |  __ _  _ __    __ _   __ _   ___  _ __                  \n"
           "               | |\\/| | / _` || '_ \\  / _` | / _` | / _ \\| '__|                 \n"
           "               | |  | || (_| || | | || (_| || (_| ||  __/| |                    \n"
           "               |_|  |_| \\__,_||_| |_| \\__,_| \\__, | \\___||_|                    \n"
           "                                              __/ |                             \n"
           "                                             |___/                              \n"
           "                                                                               \n\n");
    printf("\033[0m");
}