
#include "ParserUtils.h"

//#include <stdlib.h> // scanf
#include <stdio.h> // sscanf
#include <time.h> // mktime, tm



// FIXME: error handling
inline float parseFloat(const char* str) {
    return atof(str);
}
// FIXME: error handling
inline double parseDouble(const char* str) {
    return atof(str);
}

// FIXME: error handling
inline int parseInt(const char* str) {
    return atoi(str);
}

inline long long parseLong(const char* str) {
    return atoll(str);
}

eomi_time parseDateTime(const char * dateStr) {
    int y,M,d,h,m;
    float s = 0;
    int tzh = 0, tzm = 0;
    int ret = sscanf(dateStr, "%d-%d-%dT%d:%d:%f%d:%dZ", &y, &M, &d, &h, &m, &s, &tzh, &tzm);
    if (ret < 5) return 0;
    if (6 < ret) {
        if (tzh < 0) {
           tzm = -tzm;    // Fix the sign on minutes.
        }
    }

    struct tm datetime = { 0 };
    datetime.tm_year = y - 1900; // Year since 1900
    datetime.tm_mon = M - 1;     // 0-11
    datetime.tm_mday = d;        // 1-31
    datetime.tm_hour = h - tzh;  // 0-23
    datetime.tm_min = m - tzm;   // 0-59
    datetime.tm_sec = (int)s;    // 0-61 (0-60 in C++11)
    // FIXME: also does localtime (timezone) conversion, which needst to be corrected
    return mktime(&datetime);
}
