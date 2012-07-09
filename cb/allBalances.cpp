
// Dump balance of all addresses ever used in the blockchain

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <rmd160.h>
#include <callback.h>

#include <vector>
#include <string.h>

struct Addr
{
    uint64_t sum;
    uint160_t hash;
};

template<> uint8_t *PagedAllocator<Addr>::pool = 0;
template<> uint8_t *PagedAllocator<Addr>::poolEnd = 0;
static inline Addr *allocAddr() { return (Addr*)PagedAllocator<Addr>::alloc(); }

struct CompareAddr
{
    bool operator()(
        const Addr *const &a,
        const Addr *const &b
    ) const
    {
        return (a->sum) < (b->sum);
    }
};

typedef GoogMap<Hash160, Addr*, Hash160Hasher, Hash160Equal>::Map AddrMap;
static uint8_t emptyKey[kRIPEMD160ByteSize] = { 0x52 };
static std::vector<Addr*> gAllAddrs;
static AddrMap gAddrMap;

static const uint8_t *gLastBlock;
static const uint8_t *gFirstBlock;

struct AllBalances:public Callback
{
    virtual bool needTXHash()
    {
        return true;
    }

    virtual int init(
        int argc,
        char *argv[]
    )
    {
        if(0!=argc) return -1;
        return 0;
    }

    virtual void startMap(
        const uint8_t *p
    )
    {
        gFirstBlock = p;
        gAddrMap.setEmptyKey(emptyKey);
        gAddrMap.resize(15 * 1000 * 1000);
        gAllAddrs.reserve(15 * 1000 * 1000);
    }

    void move(
        const uint8_t *script,
        uint64_t      scriptSize,
        const uint8_t *txHash,
        int64_t        value,
        const uint8_t *downTXHash = 0
    )
    {
        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(pubKeyHash.v, script, scriptSize, addrType);
        if(unlikely(type<0))
            return;

        Addr *addr;
        auto i = gAddrMap.find(pubKeyHash.v);
        if(unlikely(gAddrMap.end()!=i))
            addr = i->second;
        else {
            addr = allocAddr();
            memcpy(addr->hash.v, pubKeyHash.v, kRIPEMD160ByteSize);
            addr->sum = 0;

            gAddrMap[addr->hash.v] = addr;
            gAllAddrs.push_back(addr);
        }

        addr->sum += value;

        static uint64_t cnt = 0;
        if(unlikely(0==((cnt++)&0xFFFFF))) {

            double progress = (script-gFirstBlock)/(double)(gLastBlock-gFirstBlock);
            printf(
                "%8.3f MMoves , "
                "%8.3f MAddrs , "
                "%5.2f%%"
                "\n",
                cnt*1e-6,
                gAddrMap.size()*1e-6,
                100.0*progress
            );
        }
    }

    virtual void endOutput(
        const uint8_t *p,
        uint64_t      value,
        const uint8_t *txHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize
    )
    {
        move(
            outputScript,
            outputScriptSize,
            txHash,
            value
        );
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
        move(
            outputScript,
            outputScriptSize,
            upTXHash,
            -(int64_t)value,
            downTXHash
        );
    }

    virtual void endMap(
        const uint8_t *p
    )
    {
        printf("sorting by balance ...\n");

        CompareAddr compare;
        auto e = gAllAddrs.end();
        auto s = gAllAddrs.begin();
        std::sort(s, e, compare);

        uint64_t i = 0;
        uint64_t n = gAllAddrs.size() - 5000;
        while(likely(s<e)) {
            Addr *addr = *(s++);
            printf("%24.8f ", (1e-8)*addr->sum);
            showHex(addr->hash.v, kRIPEMD160ByteSize, false);

            if(n<i) {
                uint8_t buf[64];
                hash160ToAddr(buf, addr->hash.v);
                printf(" %s", buf);
            }
            printf("\n");
            ++i;
        }
    }

    virtual const char *name()
    {
        return "allBalances";
    }
};

static AllBalances allBalances;

