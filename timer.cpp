
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <timer.h>

uint64_t Timer::nanos() {
    struct timespec t;
    int r = clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    if(r<0) {
        perror("clock_gettime");
        return -1;
    }

    uint64_t c = t.tv_sec;
    c *= (1000 * 1000 * 1000);
    c += t.tv_nsec;
    return c;
}

