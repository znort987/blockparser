#ifndef __COMMON_H__
    #define __COMMON_H__

    #if !defined(__STDC_FORMAT_MACROS)
        #define __STDC_FORMAT_MACROS 1
    #endif

    #include <stddef.h>
    #include <stdlib.h>
    #include <inttypes.h>

    #if defined(__GNUC__)
        #define likely(x)   __builtin_expect((x), 1)
        #define unlikely(x) __builtin_expect((x), 0)
        #define ALWAYS_INLINE __attribute__ ((always_inline))
    #else
        #define likely(x)   (x)
        #define unlikely(x) (x)
        #define ALWAYS_INLINE
    #endif

    #if defined(__CYGWIN__)
        #define lseek64 lseek
        #define NO_GOOGLE_MAP 1
    #endif

    #if defined(_WIN64)
        #include <time.h>
        #define NO_GOOGLE_MAP 1
        #define strncasecmp _strnicmp
        #define gmtime_r(x, y) gmtime_s(y, x)
        #define asctime_r(x, y) asctime_s(y, 1024, x)
    #endif

#endif // __COMMON_H__

