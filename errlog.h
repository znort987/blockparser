#ifndef __ERRLOG_H__
    #define __ERRLOG_H__

    #include <errno.h>
    #include <stdio.h>
    #include <stdarg.h>
    #include <stdlib.h>
    #include <string.h>

    static inline void vError(
        int level,
        bool system,
        const char *format,
        va_list vaList
    ) {
        fflush(stdout);
        fflush(stderr);

        bool info = (level==3);
        bool fatal = (level==0);
        bool warning = (level==2);
        const char *msgType =
            info    ? "info"    :
            fatal   ? "fatal"   :
            warning ? "warning" :
                      "error"
        ;

        fprintf(
            stderr,
            "%s: %s%s",
            msgType,
            system ? strerror(errno) : "",
            system ? ": " : ""
        );

        vfprintf(
            stderr,
            format,
            vaList
        );

        fputc('\n', stderr);
        fflush(stdout);
        fflush(stderr);

        if(fatal) {
            abort();
        }
    }

    static inline void sysErr(
        const char *format,
        ...
    ) {
        va_list vaList;
        va_start(vaList, format);
            vError(1, true, format, vaList);
        va_end(vaList);
    }

    static inline void errFatal(
        const char *format,
        ...
    ) {
        va_list vaList;
        va_start(vaList, format);
            vError(0, false, format, vaList);
        va_end(vaList);
    }

    static inline void sysErrFatal(
        const char *format,
        ...
    ) {
        va_list vaList;
        va_start(vaList, format);
            vError(0, true, format, vaList);
        va_end(vaList);
    }

    static inline void warning(
        const char *format,
        ...
    ) {
        va_list vaList;
        va_start(vaList, format);
            vError(2, false, format, vaList);
        va_end(vaList);
    }

    static inline void info(
        const char *format,
        ...
    ) {
        va_list vaList;
        va_start(vaList, format);
            vError(3, false, format, vaList);
        va_end(vaList);
    }

#endif // __ERRLOG_H__

