
// Dump balance of all addresses ever used in the blockchain

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <rmd160.h>
#include <callback.h>

#include <vector>
#include <string.h>

struct Addr
{
    uint64_t sum;
    uint160_t hash;
    uint32_t lastTouched;
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
        return (b->sum) < (a->sum);
    }
};

static uint8_t emptyKey[kRIPEMD160ByteSize] = { 0x52 };
typedef GoogMap<Hash160, Addr*, Hash160Hasher, Hash160Equal>::Map AddrMap;
typedef GoogMap<Hash160, int, Hash160Hasher, Hash160Equal>::Map RestrictMap;

struct AllBalances:public Callback
{
    int64_t limit;
    int64_t showAddr;
    int64_t cutoffBlock;
    optparse::OptionParser parser;

    AddrMap addrMap;
    uint32_t blockTime;
    const Block *curBlock;
    const Block *lastBlock;
    const Block *firstBlock;
    RestrictMap restrictMap;
    std::vector<Addr*> allAddrs;
    std::vector<uint160_t> restricts;

    AllBalances()
    {
        parser
            .usage("[options] [list of addresses to restrict output to]")
            .version("")
            .description("dump the balance for all addresses that appear in the blockchain")
            .epilog("")
        ;
        parser
            .add_option("-a", "--atBlock")
            .action("store")
            .type("int")
            .set_default(-1)
            .help("only take into account transactions in blocks strictly older than <block> (default: all)")
        ;
        parser
            .add_option("-l", "--limit")
            .action("store")
            .type("int")
            .set_default(-1)
            .help("limit output to top N balances, (default : output all addresses)")
        ;
        parser
            .add_option("-w", "--withAddr")
            .action("store")
            .type("int")
            .set_default(500)
            .help("only show address for top N results (default: N=%default)")
        ;
    }

    virtual const char                   *name() const         { return "allBalances"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;       }
    virtual bool                         needTXHash() const    { return true;          }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("balances");
    }

    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        curBlock = 0;
        lastBlock = 0;
        firstBlock = 0;

        addrMap.setEmptyKey(emptyKey);
        addrMap.resize(15 * 1000 * 1000);
        allAddrs.reserve(15 * 1000 * 1000);

        optparse::Values &values = parser.parse_args(argc, argv);
        cutoffBlock = values.get("atBlock");
        showAddr = values.get("withAddr");
        limit = values.get("limit");

        auto args = parser.args();
        for(size_t i=1; i<args.size(); ++i) {
            loadKeyList(restricts, args[i].c_str());
        }

        if(0<=cutoffBlock)
            info("only taking into account transactions before block %" PRIu64 "\n", cutoffBlock);

        if(0!=restricts.size()) {
            info(
                "restricting output to %" PRIu64 " addresses ...\n",
                (uint64_t)restricts.size()
            );

            auto e = restricts.end();
            auto i = restricts.begin();
            restrictMap.setEmptyKey(emptyKey);
            while(e!=i) {
                const uint160_t &h = *(i++);
                restrictMap[h.v] = 1;
            }
        }

        info("analyzing blockchain ...");
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
        if(unlikely(type<0)) return;

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

        addr->lastTouched = blockTime;
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
                info(
                    "%8" PRIu64 " blocks, "
                    "%8.3f MegaMoves , "
                    "%8.3f MegaAddrs , "
                    "%5.2f%%",
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
        info("done\n");

        info("sorting by balance ...");

            CompareAddr compare;
            auto e = allAddrs.end();
            auto s = allAddrs.begin();
            std::sort(s, e, compare);

        info("done\n");

        uint64_t nbRestricts = (uint64_t)restrictMap.size();
        if(0==nbRestricts) info("dumping all balances ...");
        else               info("dumping balances for %" PRIu64 " addresses ...", nbRestricts);

        int64_t i = 0;
        int64_t nonZeroCnt = 0;
        while(likely(s<e)) {

            if(0<=limit && limit<=i)
                break;

            Addr *addr = *(s++);
            if(0!=nbRestricts) {
                auto r = restrictMap.find(addr->hash.v);
                if(restrictMap.end()==r) continue;
            }

            printf("%24.8f ", (1e-8)*addr->sum);
            showHex(addr->hash.v, kRIPEMD160ByteSize, false);
            if(0<addr->sum) ++nonZeroCnt;

            if(i<showAddr || 0!=nbRestricts) {
                uint8_t buf[64];
                hash160ToAddr(buf, addr->hash.v);
                printf(" %s", buf);
            } else {
                printf(" XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            }

            struct tm gmTime;
            time_t last = addr->lastTouched;
            gmtime_r(&last, &gmTime);

            char timeBuf[256];
            asctime_r(&gmTime, timeBuf);

            size_t sz =strlen(timeBuf);
            if(0<sz) timeBuf[sz-1] = 0;

            printf(" %s\n", timeBuf);
            ++i;
        }
        info("done\n");

        info("found %" PRIu64 " addresses with non zero balance", nonZeroCnt);
        info("found %" PRIu64 " addresses in total", (uint64_t)allAddrs.size());
        info("shown:%" PRIu64 " addresses", (uint64_t)i);
        printf("\n");
        exit(0);
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
        const Block *b,
        uint64_t
    )
    {
        curBlock = b;

        const uint8_t *p = b->data;
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, bTime, p);
        blockTime = bTime;

        if(0<=cutoffBlock && cutoffBlock<=curBlock->height)
            wrapup();
    }

};

static AllBalances allBalances;

