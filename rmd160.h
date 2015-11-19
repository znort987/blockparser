#ifndef __RMD160_H__
    #define __RMD160_H__

    #include <stddef.h>
    #include <inttypes.h>
    enum { kRIPEMD160ByteSize = 20 };
    void rmd160(
              uint8_t *result,
        const uint8_t *data,
               size_t len
    );

#endif // __RMD160_H__

