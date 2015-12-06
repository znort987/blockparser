
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <timer.h>
#include <sys/time.h>

double Timer::usecs() {
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_usec + 1000000*((uint64_t)t.tv_sec);
}

uint64_t Timer::nanos() {
    #if defined(linux)
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
    #else
	return (uint64_t)(1000.0*usecs());
    #endif
}

