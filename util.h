#ifndef __UTIL_H__
    #define __UTIL_H__

    #include <common.h>
    #include <sha256.h>

    typedef const uint8_t *Hash256;
    struct uint256_t { uint8_t v[kSHA256ByteSize]; };

    struct Block
    {
        const uint8_t *data;
        uint64_t      height;
        Block         *prev;
        Block         *next;
    };

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
                    set_empty_key(empty);
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

    void showHex(
        const uint8_t *p,
        size_t        size = kSHA256ByteSize,
        bool          rev = true
    );

    uint8_t fromHexDigit(
        uint8_t h
    );

    void fromHex(
              uint8_t *dst,
        const uint8_t *src,
        size_t        dstSize,
        bool          rev = true
    );

    void showScript(
        const uint8_t *p,
        size_t        scriptSize
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

    const uint8_t *loadKeyHash(
        const uint8_t *hexHash = 0
    );

    template<
        typename T,
        size_t   kPageSize = 16384
    >
    struct PagedAllocator
    {
        static uint8_t *pool;
        static uint8_t *poolEnd;
        enum { kPageByteSize = sizeof(T)*kPageSize };

        static inline uint8_t *alloc()
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

#endif // __UTIL_H__

