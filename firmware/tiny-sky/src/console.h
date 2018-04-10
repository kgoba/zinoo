#pragma once

#include <stdint.h>

void print(char c);
void print(const char *format, ...);

void print_report();

void console_parse(const char *line);
