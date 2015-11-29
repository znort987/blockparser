#ifndef __TIMER_H__
    #define __TIMER_H__

    #include <stdint.h>

    struct Timer {
        static   double  usecs();
        static uint64_t  nanos();
    };

#endif // __TIMER_H__
