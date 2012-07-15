
// Dump balance of all addresses ever used in the blockchain

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <rmd160.h>
#include <callback.h>

#include <vector>
#include <string.h>

#define CBNAME "allBalances"
enum  optionIndex { kUnknown };
static const option::Descriptor usageDescriptor[] =
{
    { kUnknown, 0, "", "", option::Arg::None, CBNAME ":\n" },
    { 0,        0,  0,  0,                 0,        0     }
};

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

static uint8_t emptyKey[kRIPEMD160ByteSize] = { 0x52 };
typedef GoogMap<Hash160, Addr*, Hash160Hasher, Hash160Equal>::Map AddrMap;

struct AllBalances:public Callback
{
    AddrMap addrMap;
    const Block *curBlock;
    const Block *lastBlock;
    const Block *firstBlock;
    std::vector<Addr*> allAddrs;

    virtual bool needTXHash()
    {
        return true;
    }

    virtual int init(
        int argc,
        char *argv[]
    )
    {
        curBlock = 0;
        lastBlock = 0;
        firstBlock = 0;

        addrMap.setEmptyKey(emptyKey);
        addrMap.resize(15 * 1000 * 1000);
        allAddrs.reserve(15 * 1000 * 1000);

        option::Stats  stats(usageDescriptor, argc, argv);
        option::Option *buffer  = new option::Option[stats.buffer_max];
        option::Option *options = new option::Option[stats.options_max];
        option::Parser parse(usageDescriptor, argc, argv, options, buffer);
        if(parse.error()) exit(1);

        delete [] options;
        delete [] buffer;
        return 0;
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
        auto i = addrMap.find(pubKeyHash.v);
        if(unlikely(addrMap.end()!=i))
            addr = i->second;
        else {
            addr = allocAddr();
            memcpy(addr->hash.v, pubKeyHash.v, kRIPEMD160ByteSize);
            addr->sum = 0;

            addrMap[addr->hash.v] = addr;
            allAddrs.push_back(addr);
        }

        addr->sum += value;

        static uint64_t cnt = 0;
        if(unlikely(0==((cnt++)&0xFFFFF))) {

            if(
                curBlock   &&
                lastBlock  &&
                firstBlock
            )
            {
                double progress = curBlock->height/(double)lastBlock->height;
                printf(
                    "%8" PRIu64 " blocks, "
                    "%8.3f MMoves , "
                    "%8.3f MAddrs , "
                    "%5.2f%%"
                    "\n",
                    curBlock->height,
                    cnt*1e-6,
                    addrMap.size()*1e-6,
                    100.0*progress
                );
            }
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

    virtual void wrapup()
    {
        printf("sorting by balance ...\n");

        CompareAddr compare;
        auto e = allAddrs.end();
        auto s = allAddrs.begin();
        std::sort(s, e, compare);

        uint64_t i = 0;
        uint64_t nonZeroCnt = 0;
        uint64_t n = allAddrs.size() - 5000;
        while(likely(s<e)) {
            Addr *addr = *(s++);
            printf("%24.8f ", (1e-8)*addr->sum);
            showHex(addr->hash.v, kRIPEMD160ByteSize, false);
            if(0<addr->sum) ++nonZeroCnt;

            if(n<i) {
                uint8_t buf[64];
                hash160ToAddr(buf, addr->hash.v);
                printf(" %s", buf);
            }
            printf("\n");
            ++i;
        }

        printf("found %" PRIu64 " addresses with non zero balance\n", nonZeroCnt);
        printf("found %" PRIu64 " addresses\n", (uint64_t)allAddrs.size());
    }

    virtual void start(
        const Block *s,
        const Block *e
    )
    {
        firstBlock = s;
        lastBlock = e;
    }

    virtual void startBlock(
        const Block *b
    )
    {
        curBlock = b;
    }

    virtual const option::Descriptor *usage() const
    {
        return usageDescriptor;
    }

    virtual const char *name() const
    {
        return CBNAME;
    }
};

static AllBalances allBalances;

