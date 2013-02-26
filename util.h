#ifndef __UTIL_H__
    #define __UTIL_H__

    #include <string>
    #include <vector>
    #include <common.h>
    #include <rmd160.h>
    #include <sha256.h>

    typedef const uint8_t *Hash160;
    typedef const uint8_t *Hash256;
    struct uint160_t { uint8_t v[kRIPEMD160ByteSize]; };
    struct uint256_t { uint8_t v[   kSHA256ByteSize]; };
    typedef signed int int128_t __attribute__((mode(TI)));
    typedef unsigned int uint128_t __attribute__((mode(TI)));
    struct Hash160Hasher { uint64_t operator()( const Hash160 &hash160) const { uintptr_t i = reinterpret_cast<uintptr_t>(hash160); const uint64_t *p = reinterpret_cast<const uint64_t*>(i); return p[0]; } };
    struct Hash256Hasher { uint64_t operator()( const Hash256 &hash256) const { uintptr_t i = reinterpret_cast<uintptr_t>(hash256); const uint64_t *p = reinterpret_cast<const uint64_t*>(i); return p[0]; } };

    struct Hash160Equal
    {
        bool operator()(
            const Hash160 &ha,
            const Hash160 &hb
        ) const
        {
            uintptr_t ia = reinterpret_cast<uintptr_t>(ha);
            uintptr_t ib = reinterpret_cast<uintptr_t>(hb);

            const uint64_t *a0 = reinterpret_cast<const uint64_t *>(ia);
            const uint64_t *b0 = reinterpret_cast<const uint64_t *>(ib);
            if(unlikely(a0[0]!=b0[0])) return false;
            if(unlikely(a0[1]!=b0[1])) return false;

            const uint32_t *a1 = reinterpret_cast<const uint32_t *>(ia);
            const uint32_t *b1 = reinterpret_cast<const uint32_t *>(ib);
            if(unlikely(a1[4]!=b1[4])) return false;

            return true;
        }
    };

    struct Hash256Equal
    {
        bool operator()(
            const Hash256 &ha,
            const Hash256 &hb
        ) const
        {
            uintptr_t ia = reinterpret_cast<uintptr_t>(ha);
            uintptr_t ib = reinterpret_cast<uintptr_t>(hb);

            const uint64_t *a = reinterpret_cast<const uint64_t *>(ia);
            const uint64_t *b = reinterpret_cast<const uint64_t *>(ib);

            if(unlikely(a[0]!=b[0])) return false;
            if(unlikely(a[1]!=b[1])) return false;
            if(unlikely(a[2]!=b[2])) return false;
            if(unlikely(a[3]!=b[3])) return false;
            return true;
        }
    };

    struct Block
    {
        const uint8_t *data;
        int64_t       height;
        Block         *prev;
        Block         *next;
    };

    template<
        typename T,
        size_t   kPageSize = 16384
    >
    struct PagedAllocator
    {
        static uint8_t *pool;
        static uint8_t *poolEnd;
        enum { kPageByteSize = sizeof(T)*kPageSize };

        static uint8_t *alloc()
        {
            if(unlikely(poolEnd<=pool)) {
                pool = (uint8_t*)malloc(kPageByteSize);
                poolEnd = kPageByteSize + pool;
            }

            uint8_t *result = pool;
            pool += sizeof(T);
            return result;
        }
    };

    static inline Block   *allocBlock()   { return (Block*)PagedAllocator<    Block>::alloc(); }
    static inline uint8_t *allocHash256() { return         PagedAllocator<uint256_t>::alloc(); }
    static inline uint8_t *allocHash160() { return         PagedAllocator<uint160_t>::alloc(); }

    #define WANT_DENSE
    #if defined(WANT_DENSE)

        // Faster, uses more RAM

        #include <google/dense_hash_map>

        template<
            typename Key,
            typename Value,
            typename Hasher,
            typename Equal
        >
        struct GoogMap
        {
            typedef google::dense_hash_map<
                Key,
                Value,
                Hasher,
                Equal
            > MapBase;

            struct Map:public MapBase
            {
                void setEmptyKey(
                    const Key &empty
                )
                {
                    this->set_empty_key(empty);
                }
            };
        };

    #else

        // Slower, uses less RAM

        #include <google/sparse_hash_map>

        template<
            typename Key,
            typename Value,
            typename Hasher,
            typename Equal
        >
        struct GoogMap
        {
            typedef google::sparse_hash_map<
                Key,
                Value,
                Hasher,
                Equal
            > MapBase;

            struct Map:public MapBase
            {
                void setEmptyKey(
                    const Key &empty
                )
                {
                }
            };
        };

    #endif

    #define SKIP(type, var, p)       \
        p += sizeof(type)            \

    #define LOAD(type, var, p)       \
        type var = *(type*)p;        \
        p += sizeof(type)            \

    #define LOAD_VARINT(var, p)      \
        uint64_t var = loadVarInt(p) \

    static inline uint64_t loadVarInt(
        const uint8_t *&p
    )
    {
        uint64_t r = *(p++);
        if(likely(r<0xFD))  {                       return r; }
        if(likely(0xFD==r)) { LOAD(uint16_t, v, p); return v; }
        if(likely(0xFE==r)) { LOAD(uint32_t, v, p); return v; }
                              LOAD(uint64_t, v, p); return v;
    }

    double usecs();

    void toHex(
              uint8_t *dst,
        const uint8_t *src,
        size_t        size = kSHA256ByteSize,
        bool          rev = true
    );

    void showHex(
        const uint8_t *src,
        size_t        size = kSHA256ByteSize,
        bool          rev = true
    );

    uint8_t fromHexDigit(
        uint8_t h,
        bool abortOnErr = true
    );

    bool fromHex(
              uint8_t *dst,
        const uint8_t *src,
        size_t        dstSize = kSHA256ByteSize,
        bool          rev = true,
        bool          abortOnErr = true
    );

    void showScript(
        const uint8_t *p,
        size_t        scriptSize,
        const char    *header = 0,
        const char    *indent = 0
    );

    bool compressPublicKey(
              uint8_t *result,
        const uint8_t *decompressedKey
    );

    bool decompressPublicKey(
              uint8_t *result,
        const uint8_t *compressedKey
    );

    int solveOutputScript(
              uint8_t *pubKeyHash,
        const uint8_t *script,
        uint64_t      scriptSize,
        uint8_t       *type
    );

    static inline void sha256Twice(
              uint8_t *sha,
        const uint8_t *buf,
        uint64_t      size
    )
    {
        sha256(sha, buf, size);
        sha256(sha, sha, kSHA256ByteSize);
    }

    extern const uint8_t hexDigits[];
    extern const uint8_t b58Digits[];

    uint8_t fromB58Digit(
        uint8_t digit,
        bool abortOnErr = true
    );

    void hash160ToAddr(
              uint8_t *addr,
        const uint8_t *hash160,
        #if defined(LITECOIN)
              uint8_t type = 48
        #else
              uint8_t type = 0
        #endif
    );

    bool addrToHash160(
              uint8_t *hash160,
        const uint8_t *addr,
                 bool checkHash = false,
                 bool verbose = true
    );

    bool guessHash160(
              uint8_t *hash160,
        const uint8_t *addr
    );

    const uint8_t *loadKeyHash(
        const uint8_t *hexHash = 0,
        bool verbose = false
    );

    void loadKeyList(
        std::vector<uint160_t> &result,
        const char *str,
        bool verbose = false
    );

    void loadHash256List(
        std::vector<uint256_t> &result,
        const char *str,
        bool verbose = false
    );

    std::string pr128(
        const uint128_t &y
    );

    void showFullAddr(
        const Hash160 &addr,
        bool both = false
    );

    uint64_t getBaseReward(
        uint64_t h
    );

#endif // __UTIL_H__

