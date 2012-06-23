
#include <util.h>
#include <common.h>
#include <errlog.h>
#include <rmd160.h>
#include <sha256.h>
#include <opcodes.h>

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

double usecs()
{
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_usec + 1000000*((uint64_t)t.tv_sec);
}

void showHex(
    const uint8_t *p,
    size_t        size,
    bool          rev
)
{
    for(size_t i=0; i<size; ++i)
        printf("%02x", p[rev ? (size-i-1): i]);
}

uint8_t fromHexDigit(
    uint8_t h
)
{
    if('0'<=h && h<='9') return      (h - '0');
    if('A'<=h && h<='F') return 10 + (h - 'A');
    if('a'<=h && h<='f') return 10 + (h - 'a');
    abort();
}

void fromHex(
          uint8_t *dst,
    const uint8_t *src,
    size_t        dstSize,
    bool          rev
)
{
    int incr = 2;
    uint8_t *end = dstSize + dst;
    if(rev) {
        src += 2*(dstSize-1);
        incr = -2;
    }

    while(dst<end) {
        uint8_t hi = fromHexDigit(src[0]);
        uint8_t lo = fromHexDigit(src[1]);
        *(dst++) = (hi<<4) + lo;
        src += incr;
    }
}

void showScript(
    const uint8_t   *p,
    size_t          scriptSize
)
{
    const uint8_t *e = scriptSize + p;
    while(p<e) {
        LOAD(uint8_t, c, p);
        bool isImmediate = (0<c && c<79) ;
        if(!isImmediate)
            printf("    0x%02X %s\n", c, getOpcodeName(c));
        else {
            uint64_t dataSize = 0;
                 if(likely(c<=75)) {                       dataSize = c; }
            else if(likely(76==c)) { LOAD( uint8_t, v, p); dataSize = v; }
            else if(likely(77==c)) { LOAD(uint16_t, v, p); dataSize = v; }
            else if(likely(78==c)) { LOAD(uint32_t, v, p); dataSize = v; }
            printf("         OP_PUSHDATA(%" PRIu64 ", 0x", dataSize);
            showHex(p, dataSize, false);
            printf(")\n");
            p += dataSize;
        }
    }
}

void decompressPublicKey(
          uint8_t *result,          // 65 bytes
    const uint8_t *compressedKey    // 33 bytes
)
{
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY *r = o2i_ECPublicKey(&key, &compressedKey, 33);
    if(!r)
        errFatal("o2i_ECPublicKey failed");

    EC_KEY_set_conv_form(key, POINT_CONVERSION_UNCOMPRESSED);
    size_t size = i2o_ECPublicKey(key, &result);
    if(65!=size)
        errFatal("i2o_ECPublicKey failed");

    EC_KEY_free(key);
}

int solveOutputScript(
          uint8_t *pubKeyHash,
    const uint8_t *script,
    uint64_t      scriptSize,
    uint8_t       *type
)
{
    type[0] = 0;

    // Output script commonly found in block reward TX, pays to explicit pubKey
    if(
          65==script[0]             &&  // OP_PUSHDATA(65)
        0xAC==script[scriptSize-1]  &&  // OP_CHECKSIG
          67==scriptSize
    ) {
        uint256_t sha;
        sha256(sha.v, 1+script, 65);
        rmd160(pubKeyHash, sha.v, kSHA256ByteSize);
        return 0;
    }

    // Unusual output script, pays to compressed pubKeys
    if(
          33==script[0]            &&  // OP_PUSHDATA(33)
        0xAC==script[scriptSize-1] &&  // OP_CHECKSIG
          35==scriptSize
    ) {
        uint8_t pubKey[65];
        decompressPublicKey(pubKey, 1+script);

        uint256_t sha;
        sha256(sha.v, pubKey, 65);
        rmd160(pubKeyHash, sha.v, kSHA256ByteSize);
        return 1;
    }

    // The most common output script type, pays to hash160(pubKey)
    if(
        0x76==script[0]              &&  // OP_DUP
        0xA9==script[1]              &&  // OP_HASH160
          20==script[2]              &&  // OP_PUSHDATA(20)
        0x88==script[scriptSize-2]   &&  // OP_EQUALVERIFY
        0xAC==script[scriptSize-1]   &&  // OP_CHECKSIG
          25==scriptSize
    ) {
        memcpy(pubKeyHash, 3+script, kRIPEMD160ByteSize);
        return 2;
    }

    // Recent output script type, pays to hash160(script)
    if(
        0xA9==script[0]             &&  // OP_HASH160
          20==script[1]             &&  // OP_PUSHDATA(20)
        0x87==script[scriptSize-1]  &&  // OP_EQUAL
          23==scriptSize
    ) {
        memcpy(pubKeyHash, 2+script, kRIPEMD160ByteSize);
        type[0] = 'S';
        type[1] = 0;
        return 3;
    }

    // Broken output scripts that were created by p2pool for a while -- very likely lost coins
    if(
        0x73==script[0] && // OP_IFDUP
        0x63==script[1] && // OP_IF
        0x72==script[2] && // OP_2SWAP
        0x69==script[3] && // OP_VERIFY
        0x70==script[4] && // OP_2OVER
        0x74==script[5]    // OP_DEPTH
    )
        return -2;

#if 0

    // TODO : some scripts are solved by satoshi's client and not by the above. track them

    // Unknown output script type -- very likely lost coins, but hit the satoshi script solver to make sure
    int result = extractAddress(pubKeyHash, script, scriptSize);
    if(result) return -1;
    return 5;
    printf("EXOTIC OUTPUT SCRIPT:\n");
    showScript(script, scriptSize);
#endif
    return -1;
}

const uint8_t *loadKeyHash(
    const uint8_t *hexHash
)
{
    static bool loaded = false;
    static uint8_t hash[kRIPEMD160ByteSize];
    const char *someHexHash = "0568015a9facccfd09d70d409b6fc1a5546cecc6"; // 1VayNert3x1KzbpzMGt2qdqrAThiRovi8 deepbit's very large address

    if(!loaded) {

        if(0==hexHash)
            hexHash = reinterpret_cast<const uint8_t *>(someHexHash);

        if((2*kRIPEMD160ByteSize)!=strlen((const char *)hexHash))
            errFatal("specified hash has wrong length");

        fromHex(hash, hexHash, sizeof(hash), false);
        loaded = true;

    }

    return hash;
}

void hash160ToAddr(
          uint8_t *addr,    // 32 bytes is safe
    const uint8_t *hash160,
          uint8_t type
)
{
    *(addr++) = type;

    uint8_t buf[4 + 1 + kRIPEMD160ByteSize + kSHA256ByteSize];
    const uint32_t size = 1 + kRIPEMD160ByteSize + 4;
    buf[ 0] = (size>>24) & 0xff;
    buf[ 1] = (size>>16) & 0xff;
    buf[ 2] = (size>> 8) & 0xff;
    buf[ 3] = (size>> 0) & 0xff;
    buf[ 4] = 0;
    memcpy(4 + 1 + buf, hash160, kRIPEMD160ByteSize);
    sha256Twice(
        4 + 1 + kRIPEMD160ByteSize + buf,
        4 + buf,
        1 + kRIPEMD160ByteSize
    );

    static BIGNUM *b58 = 0;
    static BIGNUM *num = 0;
    static BIGNUM *div = 0;
    static BIGNUM *rem = 0;
    static BN_CTX *ctx = 0;
    static const uint8_t b58Code[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    if(!ctx) {
        ctx = BN_CTX_new();
        BN_CTX_init(ctx);

        b58 = BN_new();
        num = BN_new();
        div = BN_new();
        rem = BN_new();
        BN_set_word(b58, 58);
    }

    BN_mpi2bn(buf, 4+size, num);

    uint8_t *p = addr;
    while(!BN_is_zero(num)) {
        int r = BN_div(div, rem, num, b58, ctx);
        if(!r) errFatal("BN_div failed");
        BN_copy(num, div);

        uint32_t digit = BN_get_word(rem);
        *(p++) = b58Code[digit];
    }

    const uint8_t *s = hash160;
    const uint8_t *e = kRIPEMD160ByteSize + hash160;
    while(!*(s++) && s<e) *(p++) = b58Code[0];
    *(p--) = 0;

    while(addr<p) {
        uint8_t a = *addr;
        uint8_t b = *p;
        *(addr++) = b;
        *(p--) = a;
    }
}

template<> uint8_t *PagedAllocator<Block>::pool = 0;
template<> uint8_t *PagedAllocator<Block>::poolEnd = 0;

template<> uint8_t *PagedAllocator<uint256_t>::pool = 0;
template<> uint8_t *PagedAllocator<uint256_t>::poolEnd = 0;

