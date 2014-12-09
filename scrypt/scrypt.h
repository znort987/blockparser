#ifndef SCRYPT_MINE_H
#define SCRYPT_MINE_H

#include <stdint.h>
#include <stdlib.h>
#include <h9/uint256.h>
#define SCRYPT_BUFFER_SIZE (131072 + 63)

uint256 scrypt_salted_multiround_hash(const void* input, size_t inputlen, const void* salt, size_t saltlen, const unsigned int nRounds);
uint256 scrypt_salted_hash(const void* input, size_t inputlen, const void* salt, size_t saltlen);
uint256 scrypt_nosalt(const void* input, size_t inputlen, void *scratchpad);
uint256 scrypt_hash(const void* input, size_t inputlen);
uint256 scrypt_blockhash(const void* input);

#endif // SCRYPT_MINE_H
