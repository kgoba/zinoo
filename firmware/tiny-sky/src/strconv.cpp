#include "strconv.h"

bool strparse(uint32_t &value, const char *str, int width) {
    value = 0;
    int count = width;
    while (width == 0 || count > 0) {
        char ch = *str;
        if (ch == '\0') return true;
        if (ch < '0' || ch > '9') return false;
        int digit = (ch - '0');
        value *= 10;
        value += digit;
        str++;
        count--;
    }
    return true;    
}

bool strparse(int32_t &value, const char *str, int width) {
    uint32_t u32;
    if (*str == '-') {
        if (width > 0) {
            if (!strparse(u32, str + 1, width - 1)) return false;
        }
        else {
            if (!strparse(u32, str + 1, 0)) return false;
        }
        value = -u32;
    }
    else {
        if (!strparse(u32, str, width)) return false;
        value = u32;        
    }
    return true;    
}

bool strparse(float &value, const char *str, int width) {
    value = 0;
    bool negative = false;
    int count = width;

    if (*str == '-') {
        negative = true;
        str++;
        count--;
    }
    while (width == 0 || count > 0) {
        char ch = *str;
        if (ch == '\0') return true;
        if (ch == '.') break;
        if (ch < '0' || ch > '9') return false;
        int digit = (ch - '0');
        value *= 10;
        value += digit;
        str++;
        count--;
    }
    str++;  // SKIP DECIMAL POINT
    count--;
    float divider = 10;
    while (width == 0 || count > 0) {
        char ch = *str;
        if (ch == '\0') break;
        if (ch < '0' || ch > '9') return false;
        int digit = (ch - '0');
        value += digit / divider;
        divider *= 10;
        str++;
        count--;
    }
    if (negative) value = -value;
    return true;
}

const char * u32tostr(uint32_t value, int width, char *str, bool leading_zeros) {
    if (width < 1) return str;
    char *ptr = str;
    unsigned divisor = 1;
    for (int counter = 0; counter < width - 1; counter++) {
        divisor *= 10;
    }
    while (divisor > 0) {
        unsigned digit = value / divisor;
        if (digit > 9) break;
        char c;
        if (!leading_zeros) {
            if (digit == 0) {
                c = (width > 0) ? '\0' : ' ';
            }
            else {
                leading_zeros = true;
            }
        }
        if (leading_zeros || divisor == 1) {
            c = ('0' + digit);
        }
        if (c) {
            *ptr++ = c;
        }
        value = value % divisor;
        divisor /= 10;
    }
    *ptr = '\0';
    return str;
}

const char * i32tostr(int32_t value, int width, char *str, bool leading_zeros) {
    if (width < 1) return str;
    char *ptr = str;
    if (value < 0) {
        *ptr++ = '-';
        value = -value;
        width--;
    }
    u32tostr((unsigned)value, width, str, leading_zeros);
    return str;
}

const char * ftostr(float value, int width, int precision, char *str, bool leading_zeros) {
    if (width < 1) return str;
    char *ptr = str;
    if (value < 0) {
        *ptr++ = '-';
        value = -value;
        width--;
    }
    if (precision > 0) {
        int width_int = width - precision - 1;
        if (width_int < 1) {
            *ptr = '\0';
            return str;
        }
        i32tostr((int32_t)value, width_int, ptr, leading_zeros);
        ptr += width_int;
        *ptr++ = '.';
        float fraction = value - (int32_t)value;
        float multiplier = 1;
        for (int cnt = 0; cnt < precision; cnt++) {
            multiplier *= 10;
        }
        i32tostr((int32_t)(fraction * multiplier + 0.5f), precision, ptr, true);
    }
    else {
        i32tostr((int32_t)(value + 0.5f), width, ptr, leading_zeros);
    }
    return str;
}

const char * dtostrf(float value, int width, int precision, char *str) {
    return ftostr(value, width, precision, str);
}