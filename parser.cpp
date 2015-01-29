
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

typedef GoogMap<Hash256, Chunk*, Hash256Hasher, Hash256Equal>::Map TXOMap;
typedef GoogMap<Hash256, Block*, Hash256Hasher, Hash256Equal>::Map BlockMap;

static bool gNeedTXHash;
static Callback *gCallback;

static const Map *gCurMap;
static std::vector<Map> mapVec;

static TXOMap gTXOMap;
static BlockMap gBlockMap;
static uint8_t empty[kSHA256ByteSize] = { 0x42 };

static Block *gMaxBlock;
static Block *gNullBlock;
static int64_t gMaxHeight;
static uint64_t gChainSize;
static uint256_t gNullHash;

#if defined BITCOIN
    static const size_t gHeaderSize = 80;
    static auto gCoinDirName = "/.bitcoin/";
    static const uint32_t gExpectedMagic = 0xd9b4bef9;
#endif

#if defined LITECOIN
    static const size_t gHeaderSize = 80;
    static auto gCoinDirName = "/.litecoin/";
    static const uint32_t gExpectedMagic = 0xdbb6c0fb;
#endif

#if defined DARKCOIN
    static const size_t gHeaderSize = 80;
    static auto gCoinDirName = "/.darkcoin/";
    static const uint32_t gExpectedMagic = 0xbd6b0cbf;
#endif

#if defined PROTOSHARES
    static const size_t gHeaderSize = 88;
    static auto gCoinDirName = "/.protoshares/";
    static const uint32_t gExpectedMagic = 0xd9b5bdf9;
#endif

#if defined FEDORACOIN
    static const size_t gHeaderSize = 80;
    static auto gCoinDirName = "/.fedoracoin/";
    static const uint32_t gExpectedMagic = 0xdead1337;
#endif

#if defined PEERCOIN
    static const size_t gHeaderSize = 80;
    static auto gCoinDirName = "/.ppcoin/";
    static const uint32_t gExpectedMagic = 0xe5e9e8e6;
#endif

#if defined CLAM
    static const size_t gHeaderSize = 80;
    static auto gCoinDirName = "/.clam/";
    static const uint32_t gExpectedMagic = 0x15352203;
#endif

#if defined JUMBUCKS
    static const size_t gHeaderSize = 80;
    static auto gCoinDirName = "/.coinmarketscoin/";
    static const uint32_t gExpectedMagic = 0xb6f1f4fc;
#endif

#define DO(x) x
    static inline void   startBlock(const uint8_t *p)                      { DO(gCallback->startBlock(p));    }
    static inline void     endBlock(const uint8_t *p)                      { DO(gCallback->endBlock(p));      }
    static inline void     startTXs(const uint8_t *p)                      { DO(gCallback->startTXs(p));      }
    static inline void       endTXs(const uint8_t *p)                      { DO(gCallback->endTXs(p));        }
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

        auto outputScript = p;
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
            auto found = (fullContext && !skip && (stopAtIndex==outputIndex));
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
    const Block   *block,
    const uint8_t *&p,
    const uint8_t *txHash,
    uint64_t      inputIndex
) {
    if(!skip) {
        startInput(p);
    }

        auto upTXHash = p;
        const Chunk *upTX = 0;
        if(gNeedTXHash && !skip) {
            auto isGenTX = (0==memcmp(gNullHash.v, upTXHash, sizeof(gNullHash)));
            if(likely(false==isGenTX)) {
                auto i = gTXOMap.find(upTXHash);
                if(unlikely(gTXOMap.end()==i)) {
                    errFatal("failed to locate upstream transaction");
                }
                upTX = i->second;
            }
        }

        SKIP(uint256_t, dummyUpTXhash, p);
        LOAD(uint32_t, upOutputIndex, p);
        LOAD_VARINT(inputScriptSize, p);

        if(!skip && 0!=upTX) {
            auto inputScript = p;
            auto upTXOutputs = upTX->getData();
                parseOutputs<false, true>(
                    upTXOutputs,
                    upTXHash,
                    upOutputIndex,
                    txHash,
                    inputIndex,
                    inputScript,
                    inputScriptSize
                );
            upTX->releaseData();
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
    const Block   *block,
    const uint8_t *&p,
    const uint8_t *txHash
) {
    if(!skip) {
        startInputs(p);
    }

    LOAD_VARINT(nbInputs, p);
    for(uint64_t inputIndex=0; inputIndex<nbInputs; ++inputIndex) {
        parseInput<skip>(block, p, txHash, inputIndex);
    }

    if(!skip) {
        endInputs(p);
    }
}

template<
    bool skip
>
static void parseTX(
    const Block   *block,
    const uint8_t *&p
) {
    auto txStart = p;
    uint8_t *txHash = 0;

    if(gNeedTXHash && !skip) {
        auto txEnd = p;
        txHash = allocHash256();
        parseTX<true>(block, txEnd);
        sha256Twice(txHash, txStart, txEnd - txStart);
    }

    if(!skip) {
        startTX(p, txHash);
    }

        #if defined(CLAM)
            LOAD(uint32_t, nVersion, p);
        #else
            SKIP(uint32_t, nVersion, p);
        #endif

        #if defined(PEERCOIN) || defined(CLAM) || defined(JUMBUCKS)
            SKIP(uint32_t, nTime, p);
        #endif

        parseInputs<skip>(block, p, txHash);

        Chunk *txo = 0;
        size_t txoOffset = -1;
        const uint8_t *outputsStart = p;
        if(gNeedTXHash && !skip) {
            txo = Chunk::alloc();
            txoOffset = block->chunk->getOffset() + (p - block->chunk->getData());
            gTXOMap[txHash] = txo;
        }

        parseOutputs<skip, false>(p, txHash);

        if(txo) {
            size_t txoSize = p - outputsStart;
            txo->init(
                block->chunk->getMap(),
                txoSize,
                txoOffset
            );
        }

        SKIP(uint32_t, lockTime, p);

        #if defined(CLAM)
            if(1<nVersion) {
                LOAD_VARINT(strCLAMSpeechLen, p);
                p += strCLAMSpeechLen;
            }
        #endif

    if(!skip) {
        endTX(p);
    }
}

static void parseBlock(
    const Block *block
) {
    startBlock(block);
        auto p = block->chunk->getData();

            auto header = p;
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

            startTXs(p);
                LOAD_VARINT(nbTX, p);
                for(uint64_t txIndex=0; likely(txIndex<nbTX); ++txIndex) {
                    parseTX<false>(block, p);
                }
            endTXs(p);

            #if defined(PEERCOIN) || defined(CLAM) || defined(JUMBUCKS)
                LOAD_VARINT(vchBlockSigSize, p);
                p += vchBlockSigSize;
            #endif

        block->chunk->releaseData();
    endBlock(block);
}

static void parseLongestChain() {

    info("pass 4 -- full blockchain analysis ...");

    gCallback->startLC();

        auto blk = gNullBlock->next;
        start(blk, gMaxBlock);
        while(likely(0!=blk)) {
            parseBlock(blk);
            blk = blk->next;
        }

    gCallback->wrapup();

    info("pass 4 -- done.");
}

static void wireLongestChain() {

    info("pass 3 -- wire longest chain ...");

    auto block = gMaxBlock;
    while(1) {
        auto prev = block->prev;
        if(unlikely(0==prev)) {
            break;
        }
        prev->next = block;
        block = prev;
    }

    info(
        "pass 3 -- done, maxHeight=%d",
        (int)gMaxHeight
    );
}

static void initCallback(
    int   argc,
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
        auto i = 0;
        while('-'==argv[1][i]) {
            argv[1][i++] = 'x';
        }
    }

    auto ir = gCallback->init(argc, (const char **)argv);
    if(ir<0) {
        errFatal("callback init failed");
    }
    gNeedTXHash = gCallback->needTXHash();
}

static void findBlockParent(
    Block *b
)
{
    auto where = lseek64(
        b->chunk->getMap()->fd,
        b->chunk->getOffset(),
        SEEK_SET
    );
    if(where!=(signed)b->chunk->getOffset()) {
        sysErrFatal(
            "failed to seek into block chain file %s",
            b->chunk->getMap()->name.c_str()
        );
    }

    uint8_t buf[gHeaderSize];
    auto nbRead = read(
        b->chunk->getMap()->fd,
        buf,
        gHeaderSize
    );
    if(nbRead<(signed)gHeaderSize) {
        sysErrFatal(
            "failed to read from block chain file %s",
            b->chunk->getMap()->name.c_str()
        );
    }

    auto i = gBlockMap.find(4 + buf);
    if(unlikely(gBlockMap.end()==i)) {

        uint8_t bHash[2*kSHA256ByteSize + 1];
        toHex(bHash, b->hash);

        uint8_t pHash[2*kSHA256ByteSize + 1];
        toHex(pHash, 4 + buf);

        warning(
            "in block %s failed to locate parent block %s",
            bHash,
            pHash
        );
        return;
    }
    b->prev = i->second;
}

static void computeBlockHeight(
    Block  *block,
    size_t &lateLinks
) {

    if(unlikely(gNullBlock==block)) {
        return;
    }

    auto b = block;
    while(b->height<0) {

        if(unlikely(0==b->prev)) {

            findBlockParent(b);
            ++lateLinks;

            if(0==b->prev) {
                warning("failed to locate parent block");
                return;
            }
        }

        b->prev->next = b;
        b = b->prev;
    }

    auto height = b->height;
    while(1) {

        b->height = height++;

        if(likely(gMaxHeight<b->height)) {
            gMaxHeight = b->height;
            gMaxBlock = b;
        }

        auto next = b->next;
        b->next = 0;

        if(block==b) {
            break;
        }

        b = next;
    }
}

static void computeBlockHeights() {

    size_t lateLinks = 0;
    info("pass 2 -- link all blocks ...");
    for(const auto &pair:gBlockMap) {
        computeBlockHeight(pair.second, lateLinks);
    }

    info(
        "pass 2 -- done, did %d late links",
        (int)lateLinks
    ); 
}

static void getBlockHeader(
    size_t         &size,
    Block         *&prev,
          uint8_t *&hash,
    size_t         &earlyMissCnt,
    const uint8_t *p
) {

    LOAD(uint32_t, magic, p);
    if(unlikely(gExpectedMagic!=magic)) {
        hash = 0;
        return;
    }

    LOAD(uint32_t, sz, p);
    size = sz;
    prev = 0;

    hash = allocHash256();

    #if defined(DARKCOIN)
        h9(hash, p, gHeaderSize);
    #elif defined(CLAM)
        auto pBis = p;
        LOAD(uint32_t, nVersion, pBis);
        if(6<nVersion) {
            sha256Twice(hash, p, gHeaderSize);
        } else {
            scrypt(hash, p, gHeaderSize);
        }
    #elif defined(JUMBUCKS)
        scrypt(hash, p, gHeaderSize);
    #else
        sha256Twice(hash, p, gHeaderSize);
    #endif

    auto i = gBlockMap.find(p + 4);
    if(likely(gBlockMap.end()!=i)) {
        prev = i->second;
    } else {
        ++earlyMissCnt;
    }
}

static void buildBlockHeaders() {

    info("pass 1 -- walk all blocks and build headers ...");

    size_t nbBlocks = 0;
    size_t baseOffset = 0;
    size_t earlyMissCnt = 0;
    uint8_t buf[8+gHeaderSize];
    const auto sz = sizeof(buf);
    const auto startTime = usecs();
    const auto oneMeg = 1024 * 1024;

    for(const auto &map : mapVec) {

        startMap(0);

        while(1) {

            auto nbRead = read(map.fd, buf, sz);
            if(nbRead<(signed)sz) {
                break;
            }

            startBlock((uint8_t*)0);

            uint8_t *hash = 0;
            Block *prevBlock = 0;
            size_t blockSize = 0;

            getBlockHeader(blockSize, prevBlock, hash, earlyMissCnt, buf);
            if(unlikely(0==hash)) {
                break;
            }

            auto where = lseek(map.fd, (blockSize + 8) - sz, SEEK_CUR);
            auto blockOffset = where - blockSize;
            if(where<0) {
                break;
            }

            auto block = Block::alloc();
            block->init(hash, &map, blockSize, prevBlock, blockOffset);
            gBlockMap[hash] = block;
            endBlock((uint8_t*)0);
            ++nbBlocks;
        }
        baseOffset += map.size;

        auto now = usecs();
        auto elapsed = now - startTime;
        auto bytesPerSec = baseOffset / (elapsed*1e-6);
        auto bytesLeft = gChainSize - baseOffset;
        auto secsLeft = bytesLeft / bytesPerSec;
        fprintf(
            stderr,
            "%.2f%% (%.2f/%.2f Gigs) -- %6d blocks -- %.2f Megs/sec -- ETA %.0f secs -- ELAPSED %.0f secs            \r",
            (100.0*baseOffset)/gChainSize,
            baseOffset/(1000.0*oneMeg),
            gChainSize/(1000.0*oneMeg),
            (int)nbBlocks,
            bytesPerSec*1e-6,
            secsLeft,
            elapsed*1e-6
        );
        fflush(stderr);

        endMap(0);
    }

    if(0==nbBlocks) {
        warning("found no blocks - giving up");
        exit(1);
    }

    char msg[128];
    msg[0] = 0;
    if(0<earlyMissCnt) {
        sprintf(msg, ", %d early link misses", (int)earlyMissCnt);
    }

    auto elapsed = 1e-6*(usecs() - startTime);
    info(
        "pass 1 -- took %.0f secs, %6d blocks, %.2f Gigs, %.2f Megs/secs %s                                            ",
        elapsed,
        (int)nbBlocks,
        (gChainSize * 1e-9),
        (gChainSize * 1e-6) / elapsed,
        msg
    );
}

static void buildNullBlock() {
    gBlockMap[gNullHash.v] = gNullBlock = Block::alloc();
    gNullBlock->init(gNullHash.v, 0, 0, 0, 0);
    gNullBlock->height = 0;
}


static void initHashtables() {

    info("initializing hash tables");

    gTXOMap.setEmptyKey(empty);
    gBlockMap.setEmptyKey(empty);

    gChainSize = 0;
    for(const auto &map : mapVec) {
        gChainSize += map.size;
    }

    auto txPerBytes = (52149122.0 / 26645195995.0);
    auto nbTxEstimate = (size_t)(1.1 * txPerBytes * gChainSize);
    gTXOMap.resize(nbTxEstimate);

    auto blocksPerBytes = (331284.0 / 26645195995.0);
    auto nbBlockEstimate = (size_t)(1.1 * blocksPerBytes * gChainSize);
    gBlockMap.resize(nbBlockEstimate);

    info("estimated number of blocks = %.2fK", 1e-3*nbBlockEstimate);
    info("estimated number of transactions = %.2fM", 1e-6*nbTxEstimate);
}

static void makeBlockMaps() {

    const char *home = getenv("HOME");
    if(0==home) {
        warning("could not getenv(\"HOME\"), using \".\" instead.");
        home = ".";
    }

    std::string dataDir(home);
    dataDir += gCoinDirName;
    std::string blockDir = dataDir + std::string("blocks");

    struct stat statBuf;
    auto r = stat(blockDir.c_str(), &statBuf);
    auto oldStyle = (r<0 || !S_ISDIR(statBuf.st_mode));

    int blkDatId = (oldStyle ? 1 : 0);
    auto fmt = oldStyle ? "blk%04d.dat" : "blocks/blk%05d.dat";
    while(1) {

        char buf[64];
        sprintf(buf, fmt, blkDatId++);

        auto blockMapFileName =
            dataDir          +
            std::string(buf)
        ;

        auto blockMapFD = open(blockMapFileName.c_str(), O_RDONLY);
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

        auto mapSize = statBuf.st_size;
        auto st1 = posix_fadvise(blockMapFD, 0, mapSize, POSIX_FADV_NOREUSE);
        if(st1<0) {
            warning(
                "failed to posix_fadvise on block chain file %s",
                blockMapFileName.c_str()
            );
        }

        Map map;
        map.size = mapSize;
        map.fd = blockMapFD;
        map.name = blockMapFileName;
        mapVec.push_back(map);
    }
}

static void cleanMaps() {
    for(const auto &map : mapVec) {
        auto r = close(map.fd);
        if(r<0) {
            sysErr(
                "failed to close block chain file %s",
                map.name.c_str()
            );
        }
    }
}

int main(
    int   argc,
    char *argv[]
) {

    auto start = usecs();

    initCallback(argc, argv);
    makeBlockMaps();
    initHashtables();
    buildNullBlock();
    buildBlockHeaders();
    computeBlockHeights();
    wireLongestChain();
    parseLongestChain();
    cleanMaps();

    auto elapsed = (usecs() - start)*1e-6;
    info("all done in %.2f seconds\n", elapsed);
    return 0;
}

