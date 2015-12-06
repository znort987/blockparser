
#include <util.h>
#include <timer.h>
#include <common.h>
#include <errlog.h>
#include <callback.h>

#include <string>
#include <vector>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(S_ISDIR)
    #define S_ISDIR(mode) (S_IFDIR==((mode) & S_IFMT))
#endif

typedef GoogMap<
    Hash256,
    Chunk*,
    Hash256Hasher,
    Hash256Equal
>::Map TXOMap;

typedef GoogMap<
    Hash256,
    Block*,
    Hash256Hasher,
    Hash256Equal
>::Map BlockMap;

static bool gNeedUpstream;
static Callback *gCallback;

static const BlockFile *gCurBlockFile;
static std::vector<BlockFile> blockFiles;

static TXOMap gTXOMap;
static BlockMap gBlockMap;
static uint8_t empty[kSHA256ByteSize] = { 0x42 };

static Block *gMaxBlock;
static Block *gNullBlock;
static int64_t gMaxHeight;
static uint64_t gChainSize;
static uint256_t gNullHash;

static double getMem() {

    #if defined(linux)
        char statFileName[256];
        sprintf(
            statFileName,
            "/proc/%d/statm",
            (int)getpid()
        );

        uint64_t mem = 0;
        FILE *f = fopen(statFileName, "r");
            if(1!=fscanf(f, "%" PRIu64, &mem)) {
                warning("coudln't read process size");
            }
        fclose(f);
        return (1e-9f*mem)*getpagesize();
    #elif defined(_WIN64)
        return 0;   // TODO
    #else
        return 0;   // TODO
    #endif
}

#if defined BITCOIN
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".bitcoin";
    static const uint32_t gExpectedMagic = 0xd9b4bef9;
#endif

#if defined TESTNET3
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".bitcoin/testnet3";
    static const uint32_t gExpectedMagic = 0x0709110b;
#endif

#if defined LITECOIN
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".litecoin";
    static const uint32_t gExpectedMagic = 0xdbb6c0fb;
#endif

#if defined DARKCOIN
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".darkcoin";
    static const uint32_t gExpectedMagic = 0xbd6b0cbf;
#endif

#if defined PROTOSHARES
    static const size_t gHeaderSize = 88;
    static auto kCoinDirName = ".protoshares";
    static const uint32_t gExpectedMagic = 0xd9b5bdf9;
#endif

#if defined FEDORACOIN
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".fedoracoin";
    static const uint32_t gExpectedMagic = 0xdead1337;
#endif

#if defined PEERCOIN
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".ppcoin";
    static const uint32_t gExpectedMagic = 0xe5e9e8e6;
#endif

#if defined CLAM
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".clam";
    static const uint32_t gExpectedMagic = 0x15352203;
#endif

#if defined PAYCON
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".PayCon";
    static const uint32_t gExpectedMagic = 0x2d3b3c4b;
#endif

#if defined JUMBUCKS
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".coinmarketscoin";
    static const uint32_t gExpectedMagic = 0xb6f1f4fc;
#endif

#if defined MYRIADCOIN
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".myriadcoin";
    static const uint32_t gExpectedMagic = 0xee7645af;
#endif

#if defined UNOBTANIUM
    static const size_t gHeaderSize = 80;
    static auto kCoinDirName = ".unobtanium";
    static const uint32_t gExpectedMagic = 0x03b5d503;
#endif

#define DO(x) x
    static inline void   startBlock(const uint8_t *p)                      { DO(gCallback->startBlock(p));         }
    static inline void     endBlock(const uint8_t *p)                      { DO(gCallback->endBlock(p));           }
    static inline void     startTXs(const uint8_t *p)                      { DO(gCallback->startTXs(p));           }
    static inline void       endTXs(const uint8_t *p)                      { DO(gCallback->endTXs(p));             }
    static inline void      startTX(const uint8_t *p, const uint8_t *hash) { DO(gCallback->startTX(p, hash));      }
    static inline void        endTX(const uint8_t *p)                      { DO(gCallback->endTX(p));              }
    static inline void  startInputs(const uint8_t *p)                      { DO(gCallback->startInputs(p));        }
    static inline void    endInputs(const uint8_t *p)                      { DO(gCallback->endInputs(p));          }
    static inline void   startInput(const uint8_t *p)                      { DO(gCallback->startInput(p));         }
    static inline void     endInput(const uint8_t *p)                      { DO(gCallback->endInput(p));           }
    static inline void startOutputs(const uint8_t *p)                      { DO(gCallback->startOutputs(p));       }
    static inline void   endOutputs(const uint8_t *p)                      { DO(gCallback->endOutputs(p));         }
    static inline void  startOutput(const uint8_t *p)                      { DO(gCallback->startOutput(p));        }
    static inline void        start(const Block *s, const Block *e)        { DO(gCallback->start(s, e));           }
#undef DO

static inline void   startBlockFile(const uint8_t *p)                      { gCallback->startBlockFile(p);         }
static inline void     endBlockFile(const uint8_t *p)                      { gCallback->endBlockFile(p);           }
static inline void         startBlock(const Block *b)                      { gCallback->startBlock(b, gChainSize); }
static inline void           endBlock(const Block *b)                      { gCallback->endBlock(b);               }
static inline bool                             done()                      { return gCallback->done();             }

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
        if(gNeedUpstream && !skip) {
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
        parseInput<skip>(
            block,
            p,
            txHash,
            inputIndex
        );
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

    if(gNeedUpstream && !skip) {
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

        #if defined(PEERCOIN) || defined(CLAM) || defined(JUMBUCKS) || defined(PAYCON)
            SKIP(uint32_t, nTime, p);
        #endif

        parseInputs<skip>(block, p, txHash);

        Chunk *txo = 0;
        size_t txoOffset = -1;
        const uint8_t *outputsStart = p;
        if(gNeedUpstream && !skip) {
            txo = Chunk::alloc();
            gTXOMap[txHash] = txo;
            txoOffset = block->chunk->getOffset() + (p - block->chunk->getData());
        }

        parseOutputs<skip, false>(p, txHash);

        if(txo) {
            size_t txoSize = p - outputsStart;
            txo->init(
                block->chunk->getBlockFile(),
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

static bool parseBlock(
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
                    if(done()) {
                        return true;
                    }
                }
            endTXs(p);

            #if defined(PEERCOIN) || defined(CLAM) || defined(JUMBUCKS) || defined(PAYCON)
                LOAD_VARINT(vchBlockSigSize, p);
                p += vchBlockSigSize;
            #endif

        block->chunk->releaseData();
    endBlock(block);
    return done();
}

static void parseLongestChain() {

    info(
        "pass 4 -- full blockchain analysis (with%s index)...",
        gNeedUpstream ? "" : "out"
    );

    auto startTime = Timer::usecs();
    gCallback->startLC();

        uint64_t bytesSoFar =  0;
        auto blk = gNullBlock->next;
        start(blk, gMaxBlock);

        while(likely(0!=blk)) {

            if(0==(blk->height % 10)) {
   
                auto now = Timer::usecs();
                static auto last = -1.0;
                auto elapsedSinceLastTime = now - last;
                auto elapsedSinceStart = now - startTime;
                auto progress =  bytesSoFar/(double)gChainSize;
                auto bytesPerSec = bytesSoFar / (elapsedSinceStart*1e-6);
                auto bytesLeft = gChainSize - bytesSoFar;
                auto secsLeft = bytesLeft / bytesPerSec;
                if((1.0 * 1000 * 1000)<elapsedSinceLastTime) {
                    fprintf(
                        stderr,
                        "block %6d/%6d, %.2f%% done, ETA = %.2fsecs, mem = %.3f Gig           \r",
                        (int)blk->height,
                        (int)gMaxHeight,
                        progress*100.0,
                        secsLeft,
                        getMem()
                    );
                    fflush(stderr);
                    last = now;
                }
            }

            if(parseBlock(blk)) {
                break;
            }

            bytesSoFar += blk->chunk->getSize();
            blk = blk->next;
        }

    fprintf(stderr, "                                                          \r");
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
    gNeedUpstream = gCallback->needUpstream();

    if(done()) {
        fprintf(stderr, "\n");
        exit(0);
    }
}

static void findBlockParent(
    Block *b
) {
    auto where = lseek64(
        b->chunk->getBlockFile()->fd,
        b->chunk->getOffset(),
        SEEK_SET
    );
    if(where!=(signed)b->chunk->getOffset()) {
        sysErrFatal(
            "failed to seek into block chain file %s",
            b->chunk->getBlockFile()->name.c_str()
        );
    }

    uint8_t buf[gHeaderSize];
    auto nbRead = read(
        b->chunk->getBlockFile()->fd,
        buf,
        gHeaderSize
    );
    if(nbRead<(signed)gHeaderSize) {
        sysErrFatal(
            "failed to read from block chain file %s",
            b->chunk->getBlockFile()->name.c_str()
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
    size_t        &size,
    Block        *&prev,
    uint8_t      *&hash,
    size_t        &earlyMissCnt,
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
    #elif defined(PAYCON)
        h13(hash, p, gHeaderSize);
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
    const auto startTime = Timer::usecs();
    const auto oneMeg = 1024 * 1024;

    for(const auto &blockFile : blockFiles) {

        startBlockFile(0);

        while(1) {

            auto nbRead = read(blockFile.fd, buf, sz);
            if(nbRead<(signed)sz) {
                break;
            }

            startBlock((uint8_t*)0);

            uint8_t *hash = 0;
            Block *prevBlock = 0;
            size_t blockSize = 0;

            getBlockHeader(
                blockSize,
                prevBlock,
                hash,
                earlyMissCnt,
                buf
            );
            if(unlikely(0==hash)) {
                break;
            }

            auto where = lseek(blockFile.fd, (blockSize + 8) - sz, SEEK_CUR);
            auto blockOffset = where - blockSize;
            if(where<0) {
                break;
            }

            auto block = Block::alloc();
            block->init(hash, &blockFile, blockSize, prevBlock, blockOffset);
            gBlockMap[hash] = block;
            endBlock((uint8_t*)0);
            ++nbBlocks;
        }
        baseOffset += blockFile.size;

        auto now = Timer::usecs();
        auto elapsed = now - startTime;
        auto bytesPerSec = baseOffset / (elapsed*1e-6);
        auto bytesLeft = gChainSize - baseOffset;
        auto secsLeft = bytesLeft / bytesPerSec;
        fprintf(
            stderr,
            " %.2f%% (%.2f/%.2f Gigs) -- %6d blocks -- %.2f Megs/sec -- ETA %.0f secs -- ELAPSED %.0f secs            \r",
            (100.0*baseOffset)/gChainSize,
            baseOffset/(1000.0*oneMeg),
            gChainSize/(1000.0*oneMeg),
            (int)nbBlocks,
            bytesPerSec*1e-6,
            secsLeft,
            elapsed*1e-6
        );
        fflush(stderr);

        endBlockFile(0);
    }

    if(0==nbBlocks) {
        warning("found no blocks - giving up                                                       ");
        exit(1);
    }

    char msg[128];
    msg[0] = 0;
    if(0<earlyMissCnt) {
        sprintf(msg, ", %d early link misses", (int)earlyMissCnt);
    }

    auto elapsed = 1e-6*(Timer::usecs() - startTime);
    info(
        "pass 1 -- took %.0f secs, %6d blocks, %.2f Gigs, %.2f Megs/secs %s, mem=%.3f Gigs                                            ",
        elapsed,
        (int)nbBlocks,
        (gChainSize * 1e-9),
        (gChainSize * 1e-6) / elapsed,
        msg,
        getMem()
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

    auto kAvgBytesPerTX = 542.0;
    auto nbTxEstimate = (size_t)(1.1 * (gChainSize / kAvgBytesPerTX));
    if(gNeedUpstream) {
        gTXOMap.resize(nbTxEstimate);
    }

    auto kAvgBytesPerBlock = 140000;
    auto nbBlockEstimate = (size_t)(1.1 * (gChainSize / kAvgBytesPerBlock));
    gBlockMap.resize(nbBlockEstimate);

    info("estimated number of blocks = %.2fK", 1e-3*nbBlockEstimate);
    info("estimated number of transactions = %.2fM", 1e-6*nbTxEstimate);
    info("done initializing hash tables - mem = %.3f Gigs", getMem());
}

#if defined(__CYGWIN__)
    #include <sys/cygwin.h>
    #include <cygwin/version.h>
    static char *canonicalize_file_name(
        const char *fileName
    ) {
        auto r = (char*)cygwin_create_path(CCP_WIN_A_TO_POSIX, fileName);
        if(0==r) {
            errFatal("can't canonicalize path %s", fileName);
        }
        return r;
    }
#endif

#if defined(_WIN64)
    static char *canonicalize_file_name(
        const char *fileName
    ) {
        return strdup(fileName);
    }
#endif


static std::string getNormalizedDirName(
    const std::string &dirName
) {

    auto t = canonicalize_file_name(dirName.c_str());
    if(0==t) {
        errFatal(
            "problem accessing directory %s",
            dirName.c_str()
        );
    }

    auto r = std::string(t);
    free(t);

    auto sz = r.size();
    if(0<sz) {
        if('/'==r[sz-1]) {
            r = std::string(r, 0, sz-2);
        }
    }

    return r;
}

static std::string getBlockchainDir() {
    auto dir = getenv("BLOCKCHAIN_DIR");
    if(0==dir) {
        dir = getenv("HOME");
        if(0==dir) {
            errFatal("please  specify either env. variable HOME or BLOCKCHAIN_DIR");
        }
    }
    return getNormalizedDirName(
        dir              +
        std::string("/") +
        kCoinDirName
    );
}

static void findBlockFiles() {

    gChainSize = 0;

    auto blockChainDir = getBlockchainDir();
    auto blockDir = blockChainDir + std::string("/blocks");
    info("loading block chain from directory: %s", blockChainDir.c_str());

    struct stat statBuf;
    auto r = stat(blockDir.c_str(), &statBuf);
    auto oldStyle = (r<0 || !S_ISDIR(statBuf.st_mode));

    int blkDatId = (oldStyle ? 1 : 0);
    auto fmt = oldStyle ? "/blk%04d.dat" : "/blocks/blk%05d.dat";
    while(1) {

        char buf[64];
        sprintf(buf, fmt, blkDatId++);

        auto fileName = blockChainDir + std::string(buf) ;
        auto fd = open(fileName.c_str(), O_RDONLY);
        if(fd<0) {
            if(1<blkDatId) {
                break;
            }
            sysErrFatal(
                "failed to open block chain file %s",
                fileName.c_str()
            );
        }

        struct stat statBuf;
        auto r0 = fstat(fd, &statBuf);
        if(r0<0) {
            sysErrFatal(
                "failed to fstat block chain file %s",
                fileName.c_str()
            );
        }

        auto fileSize = statBuf.st_size;
	#if !defined(_WIN64)
	    auto r1 = posix_fadvise(fd, 0, fileSize, POSIX_FADV_NOREUSE);
	    if(r1<0) {
		warning(
		    "failed to posix_fadvise on block chain file %s",
		    fileName.c_str()
		);
	    }
	#endif

        BlockFile blockFile;
        blockFile.fd = fd;
        blockFile.size = fileSize;
        blockFile.name = fileName;
        blockFiles.push_back(blockFile);
        gChainSize += fileSize;
    }
    info("block chain size = %.3f Gigs", 1e-9*gChainSize);
}

static void cleanBlockFiles() {
    for(const auto &blockFile : blockFiles) {
        auto r = close(blockFile.fd);
        if(r<0) {
            sysErr(
                "failed to close block chain file %s",
                blockFile.name.c_str()
            );
        }
    }
}

int main(
    int   argc,
    char *argv[]
) {

    auto start = Timer::usecs();
    fprintf(stderr, "\n");
    info("mem at start = %.3f Gigs", getMem());

    initCallback(argc, argv);
    findBlockFiles();
    initHashtables();
    buildNullBlock();
    buildBlockHeaders();
    computeBlockHeights();
    wireLongestChain();
    parseLongestChain();
    cleanBlockFiles();

    auto elapsed = (Timer::usecs() - start)*1e-6;
    info("all done in %.2f seconds", elapsed);
    info("mem at end = %.3f Gigs\n", getMem());
    return 0;
}

