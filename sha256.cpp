
#include <sha256.h>
#include "openssl/sha.h"

void sha256(
    uint8_t       *result,
    const uint8_t *data,
    size_t        len
)
{
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, len);
    SHA256_Final(result, &sha256);
}

