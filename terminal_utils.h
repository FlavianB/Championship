#ifndef TERMINAL_UTILS_H
#define TERMINAL_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void terminal_alter();
void terminal_unalter();
void star_get(char *, size_t);
void welcome_prompt();

#endif