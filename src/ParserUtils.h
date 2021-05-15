#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

#include "utils.h" // types

#ifdef __cplusplus
extern "C" {
#endif

float parseFloat(const char* str);
double parseDouble(const char* str);
int parseInt(const char* str);
long long parseLong(const char* str);
eomi_time parseDateTime(const char * dateStr);
eomi_bool parseBoolean(strhash hash);


#ifdef __cplusplus
}
#endif
#endif
