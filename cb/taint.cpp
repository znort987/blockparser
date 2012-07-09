/*
    Compute blockchain taint

    How it works:

        - Each input i of a given TX has a value Vi and a "taint" Ti (taint = fraction of Vi that's "bad")

        - Each transaction mixer gets from each input:

                Vi * (  Ti)     "bad content"
                Vi * (1-Ti)     "good content"

        - Mixer final taint is :  (sum of all "bad content" in inputs) / (sum of all content in inputs)

        - In other words:

                taint = Sum(i, Vi * Ti)/Sum(i, Vi)

        - Finally outputs of a given TX have the exact same taint (fraction of the mixer that's "bad content")

        - Apply this recursively from initial TX that's the source of taint

*/

#include <util.h>
#include <string.h>
#include <common.h>
#include <errlog.h>
#include <callback.h>
typedef long double Number;
typedef GoogMap<Hash256, Number, Hash256Hasher, Hash256Equal >::Map TaintMap;

static inline void printNumber(
    const Number &x
)
{
    printf("%.32Lf ", x);
}

struct Taint:public Callback
{
    Number txBad;
    uint128_t txTotal;
    TaintMap taintMap;
    uint256_t srcTXHash;
    const uint8_t *txHash;

    virtual bool needTXHash()
    {
        return true;
    }

    virtual int init(
        int argc,
        char *argv[]
    )
    {
        bool ok = (0==argc || 1==argc);
        if(!ok) return -1;

        //const uint8_t *defaultTX = (const uint8_t*)"34b84108a142ad7b6c36f0f3549a3e83dcdbb60e0ba0df96cd48f852da0b1acb"; // Linode slush hack
        const uint8_t *defaultTX = (const uint8_t*)"a1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"; // Expensive pizza
        const uint8_t *tx = argc ? (const uint8_t*)argv[0] : defaultTX;

        size_t sz = strlen((const char*)tx);
        if(2*kSHA256ByteSize!=sz) errFatal("%s is not a valid TX hash", tx);

        static uint8_t empty[kSHA256ByteSize] = { 0x42 };
        taintMap.setEmptyKey(empty);
        taintMap.resize(15 * 1000 * 1000);

        fromHex(srcTXHash.v, tx);
        taintMap[srcTXHash.v] = 1.0;
        return 0;
    }

    virtual void endMap(
        const uint8_t *p
    )
    {
        printf("\n");
        printf("found %" PRIu64 " tainted transactions.\n", (uint64_t)taintMap.size());
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    )
    {
        txBad = 0;
        txTotal = 0;
        txHash = hash;
    }

    virtual void endTX(
        const uint8_t *p
    )
    {
        Number taint = 0;
        bool isSrcTX = (0==memcmp(txHash, srcTXHash.v, kSHA256ByteSize));
        if(unlikely(isSrcTX))
            taint = 1;
        else if(0<txTotal && 0<txBad) {
            taintMap[txHash] = taint = txBad/txTotal;
        }

        if(0<taint) {
            printNumber(taint);
            showHex(txHash);
            putchar('\n');
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
    )
    {
        auto e = taintMap.end();
        auto i = taintMap.find(upTXHash);
        if(e!=i) txBad += (value * i->second);
        txTotal += value;
    }

    virtual const char *name()
    {
        return "taint";
    }
};

static Taint taint;

