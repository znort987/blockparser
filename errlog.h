#ifndef __ERRLOG_H__
    #define __ERRLOG_H__

    #include <errno.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdarg.h>
    #include <stdlib.h>

    static inline void vError(
        int level,
        bool system,
        const char *format,
        va_list vaList
    )
    {
        bool fatal = (level==0);
        bool warning = (level==2);

        fprintf(
            stdout,
            "%s: %s%s",
            fatal ? "fatal" : warning ? "warning" : "error",
            system ? strerror(errno) : "",
            system ? ": " : ""
        );

        vfprintf(
            stdout,
            format,
            vaList
        );

        fputc('\n', stdout);
        if(fatal) abort();
    }

    static inline void sysErr(
        const char *format,
        ...
    )
    {
        va_list vaList;
        va_start(vaList, format);
            vError(1, true, format, vaList);
        va_end(vaList);
    }

    static inline void errFatal(
        const char *format,
        ...
    )
    {
        va_list vaList;
        va_start(vaList, format);
            vError(0, false, format, vaList);
        va_end(vaList);
    }

    static inline void sysErrFatal(
        const char *format,
        ...
    )
    {
        va_list vaList;
        va_start(vaList, format);
            vError(0, true, format, vaList);
        va_end(vaList);
    }

    static inline void warning(
        const char *format,
        ...
    )
    {
        va_list vaList;
        va_start(vaList, format);
            vError(2, false, format, vaList);
        va_end(vaList);
    }

#endif // __ERRLOG_H__

