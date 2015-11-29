
// base 58 tests

#include <test.h>
#include <util.h>
#include <common.h>
#include <rmd160.h>
#include <string.h>

#define TEST_ACTIVE true

#if defined(LITECOIN)

    #define EQUIV_LIST                                                                      \
        X("LiZZFKc7nqhw5EqFCZKcm9LYqidDo1w5z3", "ffffffffffffffffffffffffffffffffffffffff") \
        X("LW1FJTK6c52k1tNm9raCVvdS8gKG4QdHwa", "76423302eff5620a5b0896a67afe784966cc904d") \
        X("Ld4keFX9x7MVFUFGqX3FzFUFAtWYH8NyWL", "c3b4d20ec245a05fecbaf45133515d533ab6bc7f") \
        X("LhcNaoxfhPxqAZ9pMZAxeHgoDr2z4kN2Kr", "f5903862a48d1c5132007b7ac5e1e4ba5e520621") \
        X("LRmV6Maq5fF4FohAB8zJH24mDDNHDRXFw8", "47c76298d911a425d1c33dc1d04ac5f528000000") \
        X("LNUx1WLw9kNwfjhAB8zJH24mDDNHCLZimv", "23be67b5f0f2889aaf5053011000000000000000") \
        X("Lh6xSFb381vkeXPN8qvfEPPBs4YfUg4ufV", "f000000000000000000000000000000000000000") \
        X("LWtkkmTUSFUVfXGCgr9xX5hf2TzkXnAPYt", "8000000000000000000000000000000000000000") \
        X("LLgZ5HKukV2EgX93ErPFon28BsSqa4Ps44", "1000000000000000000000000000000000000000") \
        X("LKnk55Cht78FGo8chjVPUVxX942DGdBsY4", "06338c9ff8dea8794074e0b6b30304620dd198a4") \
        X("LKrfsrSwJs9FEKs61fosq1WLd7kn4zeLve", "06f1b6703d3f56427bfcfd372f952d50d04b64bd") \
        X("LKDxGEagrZVtC1FoQmhXrkzf9cwWsUcPxR", "00000014b57469c2ebbe6d5d7171cce78cc1512f") \
        X("LKDxGDJq5jdV1RtDXpkKJECfrH2vGmBmim", "00000000000069c2ebbe6d5d7171cce78cc1512f") \
        X("LKDxGDJq5fF4Fu2A6iscFv8kW9gmPHfdPH", "000000000000000000be6d5d7171cce78cc1512f") \
        X("LKDxGDJq5fF4Fp981DEUintPZTgPe7V23G", "0000000000000000001000000000000000000000") \
        X("LKDxGDJq5fF4FohABB8Tv1siHfiLb4iTsC", "0000000000000000000000007171cce78cc1512f") \
        X("LKDxGDJq5fF4FohAB8zJH51i1zVRjoFN7e", "000000000000000000000000000000e78cc1512f") \
        X("LKDxGDJq5fF4FohAB8zJH24mDE4huVHK8T", "000000000000000000000000000000000000512f") \
        X("LKDxGDJq5fF4FohAB8zJH24mDDNHDNtqsE", "0000000000000000000000000000000000000000") \
        X("LKDxGDJq5fF4FohAB8zJH24mDDNPgh3TkC", "000000000000000000000000000000000000003a") \
        X("LKDxGDJq5fF4FohAB8zJH24mDDNPex2urd", "0000000000000000000000000000000000000039") \
        X("LKDxGDJq5fF4FohAB8zJH24mDDNHTG8moc", "0000000000000000000000000000000000000002") \
        X("LKDxGDJq5fF4FohAB8zJH24mDDNHL33V4Q", "0000000000000000000000000000000000000001") \

#else

    #define EQUIV_LIST                                                                      \
        X("1QLbz7JHiBTspS962RLKV8GndWFwi5j6Qr", "ffffffffffffffffffffffffffffffffffffffff") \
        X("1JqoP3DKsT7RzfZ7fP3xiEQUxg9GDaGtyu", "c3b4d20ec245a05fecbaf45133515d533ab6bc7f") \
        X("1PPRKbeqcjimukTfBRBfNGd31dfhwWJvGE", "f5903862a48d1c5132007b7ac5e1e4ba5e520621") \
        X("17YXq9H1111111111111111111115U38h8", "47c76298d911a425d1c33dc1d04ac5f528000000") \
        X("14FzkJ37568tQw111111111111112FgUDy", "23be67b5f0f2889aaf5053011000000000000000") \
        X("1Nt1B3HD3MghPihCxhwMxNKRerBPJgyfUh", "f000000000000000000000000000000000000000") \
        X("1CfoVZ9eMbESQia3WiAfF4dtpFdUQ8uBUz", "8000000000000000000000000000000000000000") \
        X("12Tbp525fpnBRiSt4iPxXkxMyf5ZWzA5TC", "1000000000000000000000000000000000000000") \
        X( "1ZnortsoStC1zSTXbW6CUtkvqew8czMMG", "06338c9ff8dea8794074e0b6b30304620dd198a4") \
        X( "1dice97ECuByXAvqXpaYzSaQuPVvrtmz6", "06f1b6703d3f56427bfcfd372f952d50d04b64bd") \
        X( "11112GrmuFpwCZeEdiEajvtwQaEmpCQjC", "00000014b57469c2ebbe6d5d7171cce78cc1512f") \
        X(  "11111115PRkdC4Mgm22D8ue4fe8ryYNs", "00000000000069c2ebbe6d5d7171cce78cc1512f") \
        X(   "11111111116KzvatJyu4zHwKVDbDdW1", "000000000000000000be6d5d7171cce78cc1512f") \
        X(    "1111111111Sxq5FBSmpdMFK7bPvQb1", "0000000000000000001000000000000000000000") \
        X(    "111111111111139Adzox5TM4UJM5Xu", "0000000000000000000000007171cce78cc1512f") \
        X(     "11111111111111113wwon89cQX6eV", "000000000000000000000000000000e78cc1512f") \
        X(       "1111111111111111111hRsgmyHu", "000000000000000000000000000000000000512f") \
        X(       "1111111111111111111114oLvT2", "0000000000000000000000000000000000000000") \
        X(       "111111111111111111117e8jdRT", "000000000000000000000000000000000000003a") \
        X(       "111111111111111111117TDMMQQ", "0000000000000000000000000000000000000039") \
        X(        "11111111111111111111HeBAGj", "0000000000000000000000000000000000000002") \
        X(        "11111111111111111111BZbvjr", "0000000000000000000000000000000000000001") \

    #if !defined(BITCOIN)
        #undef TEST_ACTIVE
        #define TEST_ACTIVE false
    #endif

#endif

static int digit() {
    bool ok = true;
    for(auto x=0; x<256; ++x) {
        auto digit = (uint8_t)x;
        auto decDigit = fromB58Digit(digit, false);

        uint8_t c;
        const uint8_t *p = b58Digits;
        while(1) {
            c = *(p++);
            if(0==c || digit==c) {
                break;
            }
        }

        auto offset = (p-b58Digits) - 1;
        auto matches =
            (    0==c &&     0xFF==decDigit) ||
            (digit==c && decDigit==offset);

        TEST_CHECK(
            ok,
            matches,
            "not a match, c=%d(%c) digit=%d decDigit=%d offset=%d\n",
            (int)c,
            (int)c,
            (int)digit,
            (int)decDigit,
            (int)offset
        );
    }
    return ok ? 0 : 1;
}

static bool testHash2Addr(
    const char *addr,
    const char *hexHash
) {
    auto ok = true;
    uint8_t buf[64];
    uint8_t hash[kRIPEMD160ByteSize];
    fromHex(hash, (const uint8_t*)hexHash, sizeof(hash), false);
    hash160ToAddr(buf, (uint8_t*)hash);

    TEST_CHECK(
        ok,
        0==strcmp((char*)buf, addr),
        "encode fail\n"
        "    for hash: %s\n"
        "    expected: %s\n"
        "    got     : %s\n"
        "\n",
        hexHash,
        addr,
        (char*)(buf)
    );
    return ok;
}

static int h2addr() {
    auto count = 0;
    auto ok = true;
    #define X(addr, hash) { ok = ok && testHash2Addr(addr, hash); ++count; }
        EQUIV_LIST
    #undef X
    Test::pushMsg("checked %d vectors", count);
    return ok ? 0 : 1;
}

static bool testAddr2Hash(
    const uint8_t *addr,
    const uint8_t *hexHash
) {

    uint8_t result[kRIPEMD160ByteSize];
    auto ok = addrToHash160(result, addr, true);
    TEST_CHECK(ok, ok, "failed to decode address %s", addr);

    uint8_t hash[kRIPEMD160ByteSize];
    fromHex(hash, (const uint8_t*)hexHash, kRIPEMD160ByteSize, false);

    uint8_t hexResult[1 + 2*kRIPEMD160ByteSize];
    toHex(hexResult, result, kRIPEMD160ByteSize, false);

    TEST_CHECK(
        ok,
        0==memcmp(result, hash, kRIPEMD160ByteSize),
        "decode fail\n"
        "    for addr: %s\n"
        "    expected: %s\n"
        "         got: %s\n"
        "\n",
        addr,
        hexHash,
        hexResult
    );
    return ok;
}

static int addr2hash() {
    auto count = 0;
    auto ok = true;
    #define X(addr, hash) { ok = ok && testAddr2Hash((const uint8_t*)addr, (const uint8_t*)hash); ++count; }
        EQUIV_LIST
    #undef X
    Test::pushMsg("checked %d vectors", count);
    return ok ? 0 : 1;
}

static SimpleTest t0(digit, "base58 digit");
static SimpleTest t1(h2addr, "hash160 -> addr", TEST_ACTIVE);
static SimpleTest t2(addr2hash, "addr -> hash160", TEST_ACTIVE);

