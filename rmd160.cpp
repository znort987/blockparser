
#include <rmd160.h>
#include <crypto/ripemd160_btc.h>

void rmd160(
          uint8_t *result,
    const uint8_t *data,
           size_t len
) {
    CRIPEMD160 ctx;
    ctx.Write(data, len);
    ctx.Finalize(result);
}

