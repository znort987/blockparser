
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

struct Map {
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
static uint64_t gChainSize;
static uint64_t gMaxHeight;
static uint256_t gNullHash;

#define DO(x) x
    static inline void   startBlock(const uint8_t *p)                      { DO(gCallback->startBlock(p));    }
    static inline void     endBlock(const uint8_t *p)                      { DO(gCallback->endBlock(p));      }
    static inline void      startTX(const uint8_t *p, const uint8_t *hash) { DO(gCallback->startTX(p, hash)); }
    static inline void        endTX(const uint8_t *p)                      { DO(gCallback->endTX(p));         }
    static inline void  startInputs(const uint8_t *p)                      { DO(gCallback->startInputs(p));   }
    static inline void    endInputs(const uint8_t *p)                      { DO(gCallback->endInputs(p));     }
    static inline void   startInput(const uint8_t *p)                      { DO(gCallback->startInput(p));    }
    static inline void     endInput(const uint8_t *p)                      { DO(gCallback->endInput(p));      }
    static inline void startOutputs(const uint8_t *p)                      { DO(gCallback->startOutputs(p));  }
    static inline void   endOutputs(const uint8_t *p)                      { DO(gCallback->endOutputs(p));    }
    static inline void  startOutput(const uint8_t *p)                      { DO(gCallback->startOutput(p));   }
    static inline void        start(const Block *s, const Block *e)        { DO(gCallback->start(s, e));      }
#undef DO

static inline void     startMap(const uint8_t *p) { gCallback->startMap(p);               }
static inline void       endMap(const uint8_t *p) { gCallback->endMap(p);                 }
static inline void  startBlock(const Block *b)    { gCallback->startBlock(b, gChainSize); }
static inline void       endBlock(const Block *b) { gCallback->endBlock(b);               }

static inline void endOutput(
    const uint8_t *p,
    uint64_t      value,
    const uint8_t *txHash,
    uint64_t      outputIndex,
    const uint8_t *outputScript,
    uint64_t      outputScriptSize
) {
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
) {
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
) {
    if(!skip && !fullContext) {
        startOutput(p);
    }

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
) {
    if(!skip && !fullContext) {
        startOutputs(p);
    }

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
            if(found) {
                break;
            }
        }

    if(!skip && !fullContext) {
        endOutputs(p);
    }
}

template<
    bool skip
>
static void parseInput(
    const uint8_t *&p,
    const uint8_t *txHash,
    uint64_t      inputIndex
) {
    if(!skip) {
        startInput(p);
    }

        const uint8_t *upTXHash = p;
        const uint8_t *upTXOutputs = 0;

        if(gNeedTXHash && !skip) {
            bool isGenTX = (0==memcmp(gNullHash.v, upTXHash, sizeof(gNullHash)));
            if(likely(false==isGenTX)) {
                auto i = gTXMap.find(upTXHash);
                if(unlikely(gTXMap.end()==i)) {
                    errFatal("failed to locate upstream TX");
                }
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

    if(!skip) {
        endInput(p);
    }
}

template<
    bool skip
>
static void parseInputs(
    const uint8_t *&p,
    const uint8_t *txHash
) {
    if(!skip) {
        startInputs(p);
    }

        LOAD_VARINT(nbInputs, p);
        for(uint64_t inputIndex=0; inputIndex<nbInputs; ++inputIndex) {
            parseInput<skip>(p, txHash, inputIndex);
        }

    if(!skip) {
        endInputs(p);
    }
}

template<
    bool skip
>
static void parseTX(
    const uint8_t *&p
) {
    uint8_t *txHash = 0;
    const uint8_t *txStart = p;

    if(gNeedTXHash && !skip) {
        const uint8_t *txEnd = p;
        parseTX<true>(txEnd);
        txHash = allocHash256();
        sha256Twice(txHash, txStart, txEnd - txStart);
    }

    if(!skip) {
        startTX(p, txHash);
    }

        SKIP(uint32_t, version, p);

        parseInputs<skip>(p, txHash);

        if(gNeedTXHash && !skip) {
            gTXMap[txHash] = p;
        }

        parseOutputs<skip, false>(p, txHash);

        SKIP(uint32_t, lockTime, p);

    if(!skip) {
        endTX(p);
    }
}

static void parseBlock(
    const Block *block
) {
    startBlock(block);

        const uint8_t *p = block->data;
        const uint8_t *header = p;

        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        SKIP(uint32_t, blkTime, p);
        SKIP(uint32_t, blkBits, p);
        SKIP(uint32_t, blkNonce, p);

        #if defined PROTOSHARES
            SKIP(uint32_t, nBirthdayA, p);
            SKIP(uint32_t, nBirthdayB, p);
        #endif

        #if defined DARKCOIN
        #endif

        #if defined LITECOIN
        #endif

        #if defined BITCOIN
        #endif
        
        #if defined FEDORACOIN
        #endif
        
        LOAD_VARINT(nbTX, p);
        for(uint64_t txIndex=0; likely(txIndex<nbTX); ++txIndex) {
            parseTX<false>(p);
        }

    endBlock(block);
}

static void parseLongestChain() {

    Block *blk = gNullBlock->next;
    start(blk, gMaxBlock);
    while(likely(0!=blk)) {
        parseBlock(blk);
        blk = blk->next;
    }
}

static void wireLongestChain() {

    info("pass 3 -- wire longest chain ...");

    Block *block = gMaxBlock;
    while(1) {
        Block *prev = block->prev;
        if(unlikely(0==prev)) {
            break;
        }
        prev->next = block;
        block = prev;
    }
}

static void initCallback(
    int  argc,
    char *argv[]
) {
    const char *methodName = 0;
    if(0<argc) {
        methodName = argv[1];
    }
    if(0==methodName) {
        methodName = "";
    }
    if(0==methodName[0]) {
        methodName = "help";
    }
    gCallback = Callback::find(methodName);
    fprintf(stderr, "\n");

    info("starting command \"%s\"", gCallback->name());

    if(argv[1]) {
        int i = 0;
        while('-'==argv[1][i]) {
            argv[1][i++] = 'x';
        }
    }

    int ir = gCallback->init(argc, (const char **)argv);
    if(ir<0) {
        errFatal("callback init failed");
    }
    gNeedTXHash = gCallback->needTXHash();
}

static void mapBlockChainFiles() {

    std::string coinDirName(

        #if defined DARKCOIN
            "/.darkcoin/"
        #endif

        #if defined PROTOSHARES
            "/.protoshares/"
        #endif

        #if defined LITECOIN
            "/.litecoin/"
        #endif

        #if defined BITCOIN
            "/.bitcoin/"
        #endif
        
        #if defined FEDORACOIN
            "/.fedoracoin/"
        #endif

    );

    const char *home = getenv("HOME");
    if(0==home) {
        warning("could not getenv(\"HOME\"), using \".\" instead.");
        home = ".";
    }

    std::string homeDir(home);
    std::string blockDir = homeDir + coinDirName + std::string("blocks");

    struct stat statBuf;
    int r = stat(blockDir.c_str(), &statBuf);
    bool oldStyle = (r<0 || !S_ISDIR(statBuf.st_mode));

    int blkDatId = oldStyle ? 1 : 0;
    const char *fmt = oldStyle ? "blk%04d.dat" : "blocks/blk%05d.dat";
    while(1) {

        char buf[64];
        sprintf(buf, fmt, blkDatId++);

        std::string blockMapFileName =
            homeDir                             +
            coinDirName                         +
            std::string(buf)
        ;

        int blockMapFD = open(blockMapFileName.c_str(), O_RDONLY);
        if(blockMapFD<0) {
            if(1<blkDatId) {
                break;
            }
            sysErrFatal(
                "failed to open block chain file %s",
                blockMapFileName.c_str()
            );
        }

        struct stat statBuf;
        int st0 = fstat(blockMapFD, &statBuf);
        if(st0<0) {
            sysErrFatal(
                "failed to fstat block chain file %s",
                blockMapFileName.c_str()
            );
        }

        size_t mapSize = statBuf.st_size;
        int st1 = posix_fadvise(blockMapFD, 0, mapSize, POSIX_FADV_NOREUSE);
        if(st1<0) {
            warning(
                "failed to posix_fadvise on block chain file %s",
                blockMapFileName.c_str()
            );
        }

        void *pMap = mmap(0, mapSize, PROT_READ, MAP_PRIVATE, blockMapFD, 0);
        if(((void*)-1)==pMap) {
            sysErrFatal(
                "failed to mmap block chain file %s",
                blockMapFileName.c_str()
            );
        }

        #if 0 // This slows things down in the first pass
            int st2 = madvise(pMap, mapSize, MADV_SEQUENTIAL);
            if(st2<0) {
                warning(
                    "failed to madvises mmap'd block chain file %s",
                    blockMapFileName.c_str()
                );
            }
        #endif

        Map map;
        map.size = mapSize;
        map.fd = blockMapFD;
        map.name = blockMapFileName;
        map.p = (const uint8_t*)pMap;
        mapVec.push_back(map);
    }
}

static void initHashtables() {

    info("initializing hash tables");
    gTXMap.setEmptyKey(empty);
    gBlockMap.setEmptyKey(empty);

    gChainSize = 0;
    for(const auto &map : mapVec) {
        gChainSize += map.size;
    }

    double txPerBytes = (3976774.0 / 1713189944.0);
    size_t nbTxEstimate = (1.5 * txPerBytes * gChainSize);
    gTXMap.resize(nbTxEstimate);

    double blocksPerBytes = (184284.0 / 1713189944.0);
    size_t nbBlockEstimate = (1.5 * blocksPerBytes * gChainSize);
    gBlockMap.resize(nbBlockEstimate);
}

static void linkBlock(
    Block *block
) {

    // Root block
    if(unlikely(0==block->data)) {
        block->height = 0;
        block->prev = 0;
        block->next = 0;
        return;
    }

    // Walk up the chain until we hit a block whose depth is known
    Block *b = block;
    while(b->height<0) {

        // In case we haven't linked yet, try to do that now that we have all block headers
        if(unlikely(0==b->prev)) {
            auto i = gBlockMap.find(4 + b->data);
            if(unlikely(gBlockMap.end()==i)) {
                uint8_t buf[2*kSHA256ByteSize + 1];
                toHex(buf, 4 + b->data);
                warning(
                    "failed to locate parent block %s",
                    buf
                );
                return;
            }
            b->prev = i->second;
        }

        // Link down and move up
        b->prev->next = b;
        b = b->prev;
    }

    // Walk back down and label blocks with their correct height
    uint64_t height = b->height;
    while(block!=b) {

        if(likely(gMaxHeight<height)) {
            gMaxHeight = height;
            gMaxBlock = b;
        }

        Block *next = b->next;
        b->height = height++;
        b->next = 0;
        b = next;
    }
}

static void linkAllBlocks() {

    info("pass 2 -- link all blocks ...");
    for(const auto &pair:gBlockMap) {
        linkBlock(pair.second);
    }
}

static bool buildBlock(
    const uint8_t *&p,
    const uint8_t *e
) {

    static const uint32_t expected =

    #if defined FEDORACOIN
        0xdead1337
    #endif
    
    #if defined PROTOSHARES
        0xd9b5bdf9
    #endif

    #if defined DARKCOIN
        0xbd6b0cbf
    #endif

    #if defined LITECOIN
        0xdbb6c0fb
    #endif

    #if defined BITCOIN
        0xd9b4bef9
    #endif
    ;

    if(unlikely(e<=(8+p))) {
        return true;
    }

    LOAD(uint32_t, magic, p);
    if(unlikely(expected!=magic)) {
        return true;
    }

    LOAD(uint32_t, size, p);
    if(unlikely(e<(p+size))) {
        return true;
    }

    Block *block = allocBlock();
    block->height = -1;
    block->data = p;
    block->prev = 0;
    block->next = 0;

    // While we have our hands on the prev block hash, see if we can already link
    auto i = gBlockMap.find(p + 4);
    if(likely(gBlockMap.end()!=i)) {
        block->prev = i->second;
    }

    uint8_t *hash = allocHash256();

    #if defined(PROTOSHARES)
        size_t headerSize = 88;
    #else
        size_t headerSize = 80;
    #endif

    #if defined(DARKCOIN)
        h9(hash, p, headerSize);
    #else
        sha256Twice(hash, p, headerSize);
    #endif

    gBlockMap[hash] = block;
    p += size;
    return false;
}

static void buildAllBlocks() {

    info("pass 1 -- walk all blocks and build headers ...");

    size_t nbBlocks = 0;
    size_t baseOffset = 0;
    size_t lastReportOffset = 0;
    const auto startTime = usecs();
    const auto oneMeg = 1024 * 1024;
    const auto reportStep = 100 * oneMeg;

    for(const auto &map : mapVec) {

        auto p = map.p;
        auto s = map.p;
        auto e = map.size + map.p;

        startMap(p);

            while(likely(p<e)) {

                auto fullOffset = (p - s) + baseOffset;
                auto delta = fullOffset - lastReportOffset;
                if(unlikely(reportStep<delta)) {

                    auto now = usecs();
                    auto elapsed = now - startTime;
                    auto bytesPerSec = fullOffset / (elapsed*1e-6);
                    auto bytesLeft = gChainSize - fullOffset;
                    auto secsLeft = bytesLeft / bytesPerSec;
                    printf(
                        "%.2f%% (%.2f/%.2f Gigs) -- %6d blocks -- %.2f Megs/sec -- ETA %.0f secs            \r",
                        (100.0*fullOffset)/gChainSize,
                        fullOffset/(1000.0*oneMeg),
                        gChainSize/(1000.0*oneMeg),
                        (int)nbBlocks,
                        bytesPerSec*1e-6,
                        secsLeft
                    );
                    lastReportOffset = fullOffset;
                    fflush(stdout);
                }
                
                if(buildBlock(p, e)) {
                    break;
                }
                ++nbBlocks;
            }

        endMap(p);
        baseOffset += map.size;
    }

    auto elapsed = 1e-6*(usecs() - startTime);
    info(
        "pass 1 -- took %.0f secs, %6d blocks, %.2f Gigs, %.2f Megs/secs                                               ",
        elapsed,
        (int)nbBlocks,
        (gChainSize * 1e-9),
        (gChainSize * 1e-6)/elapsed
    );
}

static void buildNullBlock() {
    gBlockMap[gNullHash.v] = gNullBlock = allocBlock();
    gNullBlock->data = 0;
}

static void firstPart() {
    buildNullBlock();
    buildAllBlocks();
    linkAllBlocks();
}

static void secondPart() {

    wireLongestChain();

    info("pass 4 -- full blockchain analysis ...");
    gCallback->startLC();
    parseLongestChain();
    gCallback->wrapup();
    info("pass 4 -- done.");
}

static void cleanMaps() {

    for(const auto &map : mapVec) {

        int r0 = munmap((void*)map.p, map.size);
        if(r0<0) {
            sysErr(
                "failed to unmap block chain file %s",
                map.name.c_str()
            );
        }

        int r1 = close(map.fd);
        if(r1<0) {
            sysErr(
                "failed to unmap block chain file %s",
                map.name.c_str()
            );
        }
    }
}

int main(
    int  argc,
    char *argv[]
) {
    double start = usecs();

        initCallback(argc, argv);
        mapBlockChainFiles();
        initHashtables();
        firstPart();
        secondPart();
        cleanMaps();

    double elapsed = (usecs()-start)*1e-6;
    info("all done in %.2f seconds\n", elapsed);
    return 0;
}

