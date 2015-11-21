
// Find pristine blocks and rewards

#include <util.h>
#include <string.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <callback.h>

typedef GoogMap<
    Hash256,
    uint64_t,
    Hash256Hasher,
    Hash256Equal
>::Map TxMap;

struct Pristine:public Callback {

    optparse::OptionParser parser;

    TxMap txMap;
    size_t nbInputs;
    bool hasGenInput;
    uint64_t currTime;
    uint64_t currBlock;
    uint64_t nbPristine;
    const uint8_t *currTXHash;

    Pristine() {
        parser
            .usage("")
            .version("")
            .description("find all pristine blocks in the blockchain")
            .epilog("")
        ;
    }

    virtual const char                           *name() const { return "pristine"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;    }
    virtual bool                          needUpstream() const { return true;       }

    virtual int init(
        int argc,
        const char *argv[]
    ) {
        info("Finding all pristine blocks in blockchain");
        static uint8_t empty[kSHA256ByteSize] = { 0x42 };
        static uint64_t sz = 15 * 1000 * 1000;
        txMap.setEmptyKey(empty);
        txMap.resize(sz);
        nbPristine = 0;
        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    ) {
        const uint8_t *p = b->chunk->getData();
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        currBlock = b->height;
        currTime = blkTime;
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    ) {
        currTXHash = hash;
    }

    virtual void startInputs(
        const uint8_t *p
    ) {
        hasGenInput = false;
        nbInputs = 0;
    }

    virtual void startInput(
        const uint8_t *p
    ) {
        static uint256_t gNullHash;
        bool isGenInput = (0==memcmp(gNullHash.v, p, sizeof(gNullHash)));
        if(isGenInput) {
            hasGenInput = true;
        }
        ++nbInputs;
    }

    virtual void  endInputs(
        const uint8_t *p
    ) {
        if(hasGenInput) {

            if(1!=nbInputs) {
                abort();
            }

            uint64_t age = currTime;
            uint64_t blk = currBlock;
            txMap[currTXHash] = (age<<32) + blk;
            ++nbPristine;
        }
    }

    virtual void edge(
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
        auto i = txMap.find(upTXHash);
        if(txMap.end()!=i) {
            if(0<i->second) --nbPristine;
            i->second = 0;
        }
    }

    virtual void wrapup() {
        auto e = txMap.end();
        auto i = txMap.begin();
        info("Found %" PRIu64 " pristine blocks", nbPristine);
        printf("Block #  Time       TX hash\n");
        printf("===========================\n");

        while(i!=e) {
            if(0<i->second) {
                uint64_t blk = (i->second & 0xFFFFFFFF);
                uint64_t cTime = (i->second >>32);

                printf(
                    " %7" PRIu64
                    " %7" PRIu64
                    " ",
                    blk,
                    cTime
                );
        
                showHex(i->first);
                putchar('\n');
            }
            ++i;
        }
    }
};

static Pristine pristine;

