
#include "ParserUtils.h"

#include <stdlib.h> // scanf



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
