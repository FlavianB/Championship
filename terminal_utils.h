#ifndef TERMINAL_UTILS_H
#define TERMINAL_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Set terminal to non-canonical mode
void terminal_alter();
// Set terminal to canonical mode
void terminal_unalter();
// Print * instead of characters for password input
void star_put(char *, size_t);
// Print welcome message
void welcome_prompt();

#endif