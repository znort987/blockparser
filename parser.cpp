
#include <util.h>
#include <common.h>
#include <errlog.h>
#include <callback.h>

#include <string>
#include <vector>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

struct Map
{
    int fd;
    uint64_t size;
    const uint8_t *p;
    std::string name;
};

typedef GoogMap<Hash256, const uint8_t*, Hash256Hasher, Hash256Equal>::Map TXMap;
typedef GoogMap<Hash256,         Block*, Hash256Hasher, Hash256Equal>::Map BlockMap;

static bool gNeedTXHash;
static Callback *gCallback;

static const Map *gCurMap;
static std::vector<Map> mapVec;

static TXMap gTXMap;
static BlockMap gBlockMap;
static uint8_t empty[kSHA256ByteSize] = { 0x42 };

static Block *gMaxBlock;
static Block *gNullBlock;
static uint64_t gMaxHeight;
static uint256_t gNullHash;

#define DO(x) x

    static inline void   startBlock(const uint8_t *p)                      { DO(gCallback->startBlock(p));   }
    static inline void     endBlock(const uint8_t *p)                      { DO(gCallback->endBlock(p));     }
    static inline void      startTX(const uint8_t *p, const uint8_t *hash) { DO(gCallback->startTX(p, hash));}
    static inline void        endTX(const uint8_t *p)                      { DO(gCallback->endTX(p));        }
    static inline void  startInputs(const uint8_t *p)                      { DO(gCallback->startInputs(p));  }
    static inline void    endInputs(const uint8_t *p)                      { DO(gCallback->endInputs(p));    }
    static inline void   startInput(const uint8_t *p)                      { DO(gCallback->startInput(p));   }
    static inline void     endInput(const uint8_t *p)                      { DO(gCallback->endInputs(p));    }
    static inline void startOutputs(const uint8_t *p)                      { DO(gCallback->startOutputs(p)); }
    static inline void   endOutputs(const uint8_t *p)                      { DO(gCallback->endOutputs(p));   }
    static inline void  startOutput(const uint8_t *p)                      { DO(gCallback->startOutput(p));  }
    static inline void        start(const Block *s, const Block *e)        { DO(gCallback->start(s, e));     }

#undef DO

static inline void     startMap(const uint8_t *p) { gCallback->startMap(p);     }
static inline void       endMap(const uint8_t *p) { gCallback->endMap(p);       }

static inline void  startBlock(const Block *b)    { gCallback->startBlock(b);   }
static inline void       endBlock(const Block *b) { gCallback->endBlock(b);     }

static inline void endOutput(
    const uint8_t *p,
    uint64_t      value,
    const uint8_t *txHash,
    uint64_t      outputIndex,
    const uint8_t *outputScript,
    uint64_t      outputScriptSize
)
{
    gCallback->endOutput(
        p,
        value,
        txHash,
        outputIndex,
        outputScript,
        outputScriptSize
    );
}

static inline void edge(
    uint64_t      value,
    const uint8_t *upTXHash,
    uint64_t      outputIndex,
    const uint8_t *outputScript,
    uint64_t      outputScriptSize,
    const uint8_t *downTXHash,
    uint64_t      inputIndex,
    const uint8_t *inputScript,
    uint64_t      inputScriptSize
)
{
    gCallback->edge(
        value,
        upTXHash,
        outputIndex,
        outputScript,
        outputScriptSize,
        downTXHash,
        inputIndex,
        inputScript,
        inputScriptSize
    );
}

template<
    bool skip,
    bool fullContext
>
static void parseOutput(
    const uint8_t *&p,
    const uint8_t *txHash,
    uint64_t      outputIndex,
    const uint8_t *downTXHash,
    uint64_t      downInputIndex,
    const uint8_t *downInputScript,
    uint64_t      downInputScriptSize,
    bool          found = false
)
{
    if(!skip && !fullContext) startOutput(p);

        LOAD(uint64_t, value, p);
        LOAD_VARINT(outputScriptSize, p);

        const uint8_t *outputScript = p;
        p += outputScriptSize;

        if(!skip && fullContext && found) {
            edge(
                value,
                txHash,
                outputIndex,
                outputScript,
                outputScriptSize,
                downTXHash,
                downInputIndex,
                downInputScript,
                downInputScriptSize
            );
        }

    if(!skip && !fullContext) {
        endOutput(
            p,
            value,
            txHash,
            outputIndex,
            outputScript,
            outputScriptSize
        );
    }
}

template<
    bool skip,
    bool fullContext
>
static void parseOutputs(
    const uint8_t *&p,
    const uint8_t *txHash,
    uint64_t      stopAtIndex = -1,
    const uint8_t *downTXHash = 0,
    uint64_t      downInputIndex = 0,
    const uint8_t *downInputScript = 0,
    uint64_t      downInputScriptSize = 0
)
{
    if(!skip && !fullContext) startOutputs(p);

        LOAD_VARINT(nbOutputs, p);
        for(uint64_t outputIndex=0; outputIndex<nbOutputs; ++outputIndex) {
            bool found = fullContext && !skip && (stopAtIndex==outputIndex);
            parseOutput<skip, fullContext>(
                p,
                txHash,
                outputIndex,
                downTXHash,
                downInputIndex,
                downInputScript,
                downInputScriptSize,
                found
            );
            if(found) break;
        }

    if(!skip && !fullContext) endOutputs(p);
}

template<
    bool skip
>
static void parseInput(
    const uint8_t *&p,
    const uint8_t *txHash,
    uint64_t      inputIndex
)
{
    if(!skip) startInput(p);

        const uint8_t *upTXHash = p;
        const uint8_t *upTXOutputs = 0;

        if(gNeedTXHash && !skip) {
            bool isGenTX = (0==memcmp(gNullHash.v, upTXHash, sizeof(gNullHash)));
            if(likely(false==isGenTX)) {
                auto i = gTXMap.find(upTXHash);
                if(unlikely(gTXMap.end()==i))
                    errFatal("failed to locate upstream TX");
                upTXOutputs = i->second;
            }
        }

        SKIP(uint256_t, dummyUpTXhash, p);
        LOAD(uint32_t, upOutputIndex, p);
        LOAD_VARINT(inputScriptSize, p);

        if(!skip && 0!=upTXOutputs) {
            const uint8_t *inputScript = p;
            parseOutputs<false, true>(
                upTXOutputs,
                upTXHash,
                upOutputIndex,
                txHash,
                inputIndex,
                inputScript,
                inputScriptSize
            );
        }

        p += inputScriptSize;
        SKIP(uint32_t, sequence, p);

    if(!skip) endInput(p);
}

template<
    bool skip
>
static void parseInputs(
    const uint8_t *&p,
    const uint8_t *txHash
)
{
    if(!skip) startInputs(p);

        LOAD_VARINT(nbInputs, p);
        for(uint64_t inputIndex=0; inputIndex<nbInputs; ++inputIndex)
            parseInput<skip>(p, txHash, inputIndex);

    if(!skip) endInputs(p);
}

template<
    bool skip
>
static void parseTX(
    const uint8_t *&p
)
{
    uint8_t *txHash = 0;
    const uint8_t *txStart = p;

    if(gNeedTXHash && !skip) {
        const uint8_t *txEnd = p;
        parseTX<true>(txEnd);
        txHash = allocHash256();
        sha256Twice(txHash, txStart, txEnd - txStart);
    }

    if(!skip) startTX(p, txHash);

        SKIP(uint32_t, version, p);

        parseInputs<skip>(p, txHash);

        if(gNeedTXHash && !skip)
            gTXMap[txHash] = p;

        parseOutputs<skip, false>(p, txHash);

        SKIP(uint32_t, lockTime, p);

    if(!skip) endTX(p);
}

static void parseBlock(
    const Block *block
)
{
    startBlock(block);

        const uint8_t *p = block->data;
        const uint8_t *header = p;
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        SKIP(uint32_t, blkTime, p);
        SKIP(uint32_t, blkBits, p);
        SKIP(uint32_t, blkNonce, p);

        LOAD_VARINT(nbTX, p);
        for(uint64_t txIndex=0; likely(txIndex<nbTX); ++txIndex)
            parseTX<false>(p);

    endBlock(block);
}

static void parseLongestChain()
{
    Block *blk = gNullBlock->next;

    start(blk, gMaxBlock);
    while(likely(0!=blk)) {
        parseBlock(blk);
        blk = blk->next;
    }
}

static void findLongestChain()
{
    Block *block = gMaxBlock;
    while(1) {
        Block *prev = block->prev;
        if(unlikely(0==prev)) break;
        prev->next = block;
        block = prev;
    }
}

static void parseBlock(
    const uint8_t *&p
)
{
    startBlock(p);

        LOAD(uint32_t, magic, p);
        LOAD(uint32_t, size, p);
        if(unlikely(0xd9b4bef9!=magic))
            errFatal("at offset %" SCNuPTR " should have found block magic.", p - gCurMap->p);

        auto i = gBlockMap.find(p+4);
        if(unlikely(gBlockMap.end()==i))
            errFatal("failed to locate parent block");

        Block *prev = i->second;
        Block *block = allocBlock();
        block->height = 1 + prev->height;
        block->prev = prev;
        block->data = p;
        block->next = 0;

        if(likely(gMaxHeight<block->height)) {
            gMaxHeight = block->height;
            gMaxBlock = block;
        }

        uint8_t *hash = allocHash256();
        sha256Twice(hash, p, 80);
        gBlockMap[hash] = block;
        p += size;

    endBlock(p);
}

static void initCallback(
    int     argc,
    char    *argv[]
)
{
    argv += (0<argc);
    argc -= (0<argc);

    const char *methodName = 0;
    if(0<argc) {
        methodName = argv[0];
        ++argv;
        --argc;
    }

    if(0==methodName) methodName = "";
    gCallback = Callback::find(methodName);
    if(0==gCallback) errFatal("unknown callback : %s\n", methodName);

    fprintf(stderr, "\n");
    info(
        "Starting command \"%s\"\n",
        gCallback->name()
    );

    int ir = gCallback->init(argc, argv);
    if(ir<0) errFatal("callback init failed");
    gNeedTXHash = gCallback->needTXHash();
}

static void mapBlockChainFiles()
{
    const char *homeDir = getenv("HOME");
    if(0==homeDir) {
        warning("could not getenv(\"HOME\"), using \".\" instead.");
        homeDir = ".";
    }

    int blkDatId = 0;
    while(1) {

        char buf[64];
        sprintf(buf, "%04d", ++blkDatId);

        std::string blockMapFileName =
            homeDir                      +
            std::string("/.bitcoin/blk") +
            std::string(buf)             +
            std::string(".dat");

        int blockMapFD = open(blockMapFileName.c_str(), O_DIRECT | O_RDONLY);
        if(blockMapFD<0) {
            if(1<blkDatId) break;
            sysErrFatal(
                "failed to open block chain file %s",
                blockMapFileName.c_str()
            );
        }

        struct stat statBuf;
        int r = fstat(blockMapFD, &statBuf);
        if(r<0) sysErrFatal( "failed to fstat block chain file %s", blockMapFileName.c_str());

        size_t mapSize = statBuf.st_size;
        void *pMap = mmap(0, mapSize, PROT_READ, MAP_PRIVATE, blockMapFD, 0);
        if(((void*)-1)==pMap) sysErrFatal( "failed to mmap block chain file %s", blockMapFileName.c_str());

        Map map;
        map.size = mapSize;
        map.fd = blockMapFD;
        map.name = blockMapFileName;
        map.p = (const uint8_t*)pMap;
        mapVec.push_back(map);
    }
}

static void initHashtables()
{
    gTXMap.setEmptyKey(empty);
    gBlockMap.setEmptyKey(empty);

    auto e = mapVec.end();
    uint64_t totalSize = 0;
    auto i = mapVec.begin();
    while(i!=e) totalSize += (i++)->size;

    double txPerBytes = (3976774.0 / 1713189944.0);
    size_t nbTxEstimate = (1.5 * txPerBytes * totalSize);
    gTXMap.resize(nbTxEstimate);

    double blocksPerBytes = (184284.0 / 1713189944.0);
    size_t nbBlockEstimate = (1.5 * blocksPerBytes * totalSize);
    gBlockMap.resize(nbBlockEstimate);
}

static void firstPass()
{
    gBlockMap[gNullHash.v] = gNullBlock = allocBlock();
    gNullBlock->height = -1;
    gNullBlock->prev = 0;
    gNullBlock->next = 0;
    gNullBlock->data = 0;

    auto e = mapVec.end();
    auto i = mapVec.begin();
    while(i!=e) {

        const Map *map = gCurMap = &(*(i++));
        const uint8_t *end = map->size + map->p;
        const uint8_t *p = map->p;
        startMap(p);

            while(unlikely(p<end)) parseBlock(p);

        endMap(p);
    }
}

static void secondPass()
{
    findLongestChain();
    parseLongestChain();
    gCallback->wrapup();
}

static void cleanMaps()
{
    auto e = mapVec.end();
    auto i = mapVec.begin();
    while(i!=e) {

        const Map &map = *(i++);

        int r = munmap((void*)map.p, map.size);
        if(r<0) sysErr("failed to unmap block chain file %s", map.name.c_str());

        r = close(map.fd);
        if(r<0) sysErr("failed to unmap block chain file %s", map.name.c_str());

    }
}

int main(
    int     argc,
    char    *argv[]
)
{
    double start = usecs();

        initCallback(argc, argv);
        mapBlockChainFiles();
        initHashtables();
        firstPass();
        secondPass();
        cleanMaps();

    double elapsed = (usecs()-start)*1e-6;
    info("all done in %.3f seconds\n", elapsed);
    return 0;
}

