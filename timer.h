#ifndef __TIMER_H__
    #define __TIMER_H__

    #include <stdint.h>

    struct Timer {
        static uint64_t  nanos();
        static uint64_t micros() { return nanos()/1000;   }
        static   double  usecs() { return nanos()/1000.0; }
    };

#endif // __TIMER_H__
