
#include <sha256.h>
#include <crypto/sha256_btc.h>

void sha256(
    uint8_t       *result,
    const uint8_t *data,
    size_t        len
) {
    CSHA256 ctx;
    ctx.Write(data, len);
    ctx.Finalize(result);
}

