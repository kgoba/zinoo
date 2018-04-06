#pragma once

#include <stdint.h>

bool strparse(float &value, const char *str, int width = 0);
bool strparse(uint32_t &value, const char *str, int width = 0);
bool strparse(int32_t &value, const char *str, int width = 0);

const char * u32tostr(uint32_t value, int width, char *str, bool leading_zeros = false);
const char * i32tostr(int32_t value, int width, char *str, bool leading_zeros = false);
const char * ftostr(float value, int width, int precision, char *str, bool leading_zeros = false);
const char * dtostrf(float value, int width, int precision, char *str);