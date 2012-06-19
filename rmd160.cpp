
#include <rmd160.h>
#include "openssl/ripemd.h"

void rmd160(
          uint8_t *result,
    const uint8_t *data,
           size_t len
)
{
    RIPEMD160_CTX ctx;
    RIPEMD160_Init(&ctx);
    RIPEMD160_Update(&ctx, data, len);
    RIPEMD160_Final(result, &ctx);
}


