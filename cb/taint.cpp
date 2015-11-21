/*
    Compute blockchain taint

    How it works:

        - Each TX can be seen as a mixer : stuff flows in (inputs), stuff gets mixed in the middle, and stuff flows out (outputs)

        - Each input i of a given TX has a value Vi and a "taint" Ti (taint = fraction of Vi that's "bad")

        - The TX mixer gets from each input:

                Vi * (  Ti)     "bad content"
                Vi * (1-Ti)     "good content"

        The TX mixer final taint is :  (sum of all "bad content" in inputs) / (sum of all content in inputs)

        - In other words:

                taint = Sum(i, Vi * Ti)/Sum(i, Vi)

        - All outputs of TX have the exact same taint (fraction of the TX mixer that's "bad")

        - Apply this recursively from initial TX that's the source of taint to all downstream TX

*/

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <string.h>
#include <callback.h>

typedef long double Number;
typedef GoogMap<Hash256, int, Hash256Hasher, Hash256Equal >::Map TxMap;
typedef GoogMap<Hash256, Number, Hash256Hasher, Hash256Equal >::Map TaintMap;

static inline void printNumber(
    const Number &x
) {
    printf("%.32Lf ", x);
}

struct Taint : public Callback {

    optparse::OptionParser parser;

    Number txBad;
    uint64_t bTime;
    TxMap srcTxMap;
    Number threshold;
    uint128_t txTotal;
    TaintMap taintMap;
    uint64_t currBlock;
    const uint8_t *txHash;
    std::vector<uint256_t> rootHashes;

    Taint() {
        parser
            .usage("[list of transaction hashes]")
            .version("")
            .description(
                "compute the taint from list of specified transactions"
                "to *all* existing transactions found in the blockchain"
            )
            .epilog("")
       ;
    }

    virtual const char                   *name() const         { return "taint"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser; }
    virtual bool                       needUpstream() const    { return true;    }

    virtual void aliases(
        std::vector<const char*> &v
    ) const {
        v.push_back("trace");
    }

    virtual int init(
        int argc,
        const char *argv[]
    ) {
        threshold = 1e-20;

        optparse::Values &values = parser.parse_args(argc, argv);

        auto args = parser.args();
        for(size_t i=1; i<args.size(); ++i) {
            loadHash256List(rootHashes, args[i].c_str());
        }

        if(0<rootHashes.size()) {
            info("computing taint from %d source transactions\n", (int)rootHashes.size());
        } else {
            warning("no TX hashes specified, using the infamous 10K pizza TX");

            //const char *defaultTX = "34b84108a142ad7b6c36f0f3549a3e83dcdbb60e0ba0df96cd48f852da0b1acb"; // Linode slush hack
            const char *defaultTX = "a1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"; // Very expensive pizza
            loadHash256List(rootHashes, defaultTX);
        }

        static uint8_t empty[kSHA256ByteSize] = { 0x42 };
        static uint64_t sz = 15 * 1000 * 1000;
        srcTxMap.setEmptyKey(empty);
        taintMap.setEmptyKey(empty);
        taintMap.resize(sz);

        for(const auto &txHash : rootHashes) {
            taintMap[txHash.v] = 1.0;
            srcTxMap[txHash.v] = 1;
        }

        return 0;
    }

    virtual void wrapup() {
        info("found %" PRIu64 " tainted transactions.\n", (uint64_t)taintMap.size());
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    ) {
        currBlock = b->height;

        const uint8_t *p = b->chunk->getData();
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        bTime = blkTime;
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    ) {
        txHash = hash;
        txTotal = 0;
        txBad = 0;
    }

    virtual void endTX(
        const uint8_t *p
    ) {
        auto i = srcTxMap.find(txHash);
        bool isSrcTX = (srcTxMap.end() != i);

        Number taint = 0;
        if(unlikely(isSrcTX)) {
            taint = 1;
        } else if(0<txTotal && 0<txBad) {
            taintMap[txHash] = taint = txBad/txTotal;
        }

        if(threshold<taint) {

            auto absTaint = txTotal * taint;
            printNumber(absTaint);
            printNumber(taint);
            showHex(txHash);

            struct tm gmTime;
            time_t blockTime = bTime;
            gmtime_r(&blockTime, &gmTime);

            char timeBuf[256];
            asctime_r(&gmTime, timeBuf);

            size_t sz =strlen(timeBuf);
            if(0<sz) {
                timeBuf[sz-1] = 0;
            }

            printf(
                " %" PRIu64
                " %s"
                " %16.8f"
                "\n",
                currBlock-1,
                timeBuf,
                satoshisToNormaForm(txTotal)
            );
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
        auto e = taintMap.end();
        auto i = taintMap.find(upTXHash);
        if(e!=i) {
            txBad += (value * i->second);
        }
        txTotal += value;
    }
};

static Taint taint;

