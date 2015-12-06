
// Dump balance of all addresses ever used in the blockchain

#include <util.h>
#include <timer.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <rmd160.h>
#include <sha256.h>
#include <callback.h>

#include <vector>
#include <string.h>
#include <signal.h>
#include <algorithm>

struct Addr;
struct AllBalances;
AllBalances *theObject;
static uint8_t emptyKey[kSHA256ByteSize] = { 0x52 };
typedef GoogMap<Hash160, Addr*, Hash160Hasher, Hash160Equal>::Map AddrMap;
typedef GoogMap<Hash160, int, Hash160Hasher, Hash160Equal>::Map RestrictMap;

struct Output {
    int64_t time;
    int64_t value;
    uint64_t inputIndex;
    uint64_t outputIndex;
    const uint8_t *upTXHash;
    const uint8_t *downTXHash;
};
typedef std::vector<Output> OutputVec;

struct Addr {
    uint8_t type;
    uint64_t sum;
    uint64_t nbIn;
    uint64_t nbOut;
    uint160_t hash;
    uint32_t lastIn;
    uint32_t lastOut;
    OutputVec *outputVec;
};

template<> uint8_t *PagedAllocator<Addr>::pool = 0;
template<> uint8_t *PagedAllocator<Addr>::poolEnd = 0;
static inline Addr *allocAddr() { return (Addr*)PagedAllocator<Addr>::alloc(); }

struct CompareAddr {
    bool operator()(
        const Addr *const &a,
        const Addr *const &b
    ) const {
        return (b->sum) < (a->sum);
    }
};

struct AllBalances:public Callback {

    bool csv;
    bool isDone;
    bool detailed;
    int64_t limit;
    uint64_t offset;
    bool interrupted;
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
            .description("dump the balance for all specified addresses (default: all addresses that ever appear in the blockchain)")
            .epilog("")
        ;
        parser
            .add_option("-a", "--atBlock")
            .action("store")
            .type("int")
            .set_default(-1)
            .help("only take into account transactions in blocks strictly older than <block> (default: use all transactions)")
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
        parser
            .add_option("-d", "--detailed")
            .action("store_true")
            .set_default(false)
            .help("also show all unspent outputs")
        ;
        parser
            .add_option("-c", "--csv")
            .action("store_true")
            .set_default(false)
            .help("produce CSV-formatted output instead column-formatted")
        ;

        if(theObject) {
            errFatal("only one allowed");
        }
        theObject = this;
    }

    virtual const char                   *name() const         { return "allBalances"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;       }
    virtual bool                         needUpstream() const  { return true;          }

    virtual bool done() {
        if(interrupted) {

            isDone = true;

            info("                                                             ");
            info("       ... interrupted ...");
            info("");

            char endTime[256];
            gmTime(endTime, blockTime);
            info(
                "dumping coherent state of ledger as of block %d, minted on %s",
                (int)(curBlock->height),
                endTime
            );
        }
        return isDone;
    }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("balances");
    }

    static void forceDone(int) {
        theObject->interrupted = true;
        signal(SIGINT, SIG_DFL);
    }

    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        offset = 0;
        curBlock = 0;
        lastBlock = 0;
        firstBlock = 0;

        addrMap.setEmptyKey(emptyKey);
        addrMap.resize(15 * 1000 * 1000);
        allAddrs.reserve(15 * 1000 * 1000);

	optparse::Values &values = parser.parse_args(argc, argv);


        cutoffBlock = values.get("atBlock").asInt64();
        showAddr = values.get("withAddr").asInt64();
        detailed = values.get("detailed");
        limit = values.get("limit").asInt64();
        csv = values.get("csv");
        interrupted = false;
        isDone = false;

        auto args = parser.args();
        for(size_t i=1; i<args.size(); ++i) {
            loadKeyList(restricts, args[i].c_str());
        }

        if(0<=cutoffBlock) {
            info("only taking into account transactions before block %" PRIu64 "\n", cutoffBlock);
        }

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
        } else {
            if(detailed && 50000<cutoffBlock) {
                warning("              !!!!!!!!!!!!!!!!");
                warning("asking for --detailed for *all* addresses in the blockchain will be *very* slow");
                warning("as a matter of fact, it likely won't ever finish unless you have *lots* of RAM");
                warning("              !!!!!!!!!!!!!!!!");
            }
        }

        signal(SIGINT, forceDone);
        return 0;
    }

    void move(
        const uint8_t *script,
        uint64_t      scriptSize,
        const uint8_t *upTXHash,
        int64_t       outputIndex,
        int64_t       value,
        const uint8_t *downTXHash = 0,
        uint64_t      inputIndex = -1
    )
    {
        uint8_t addrType[3];
        uint160_t pubKeyHash;
        auto scriptType = solveOutputScript(
            pubKeyHash.v,
            script,
            scriptSize,
            addrType
        );
        if(unlikely(scriptType<0)) {
            return;
        }

        if(0!=restrictMap.size()) {
            auto r = restrictMap.find(pubKeyHash.v);
            if(restrictMap.end()==r) {
                return;
            }
        }

        Addr *addr;
        auto i = addrMap.find(pubKeyHash.v);
        if(unlikely(addrMap.end()!=i)) {
            addr = i->second;
        } else {

            addr = allocAddr();

            memcpy(addr->hash.v, pubKeyHash.v, kRIPEMD160ByteSize);
            addr->type = addrType[0];
            addr->outputVec = 0;
            addr->nbOut = 0;
            addr->nbIn = 0;
            addr->sum = 0;

            if(detailed) {
                addr->outputVec = new OutputVec;
            }

            addrMap[addr->hash.v] = addr;
            allAddrs.push_back(addr);
        }

        if(0<value) {
            addr->lastIn = blockTime;
            ++(addr->nbIn);
        } else {
            addr->lastOut = blockTime;
            ++(addr->nbOut);
        }
        addr->sum += value;

        if(detailed) {
            struct Output output;
            output.value = value;
            output.time = blockTime;
            output.upTXHash = upTXHash;
            output.downTXHash = downTXHash;
            output.inputIndex = inputIndex;
            output.outputIndex = outputIndex;
            addr->outputVec->push_back(output);
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
            outputIndex,
            value
        );
    }

    static void gmTime(
        char *timeBuf,
        const time_t &last
    )
    {
        struct tm gmTime;
        gmtime_r(&last, &gmTime);
        asctime_r(&gmTime, timeBuf);

        size_t sz =strlen(timeBuf);
        if(0<sz) timeBuf[sz-1] = 0;
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
            outputIndex,
            -(int64_t)value,
            downTXHash,
            inputIndex
        );
    }

    virtual void wrapup() {

        CompareAddr compare;
        auto e = allAddrs.end();
        auto s = allAddrs.begin();
        if(false==csv) {
            info("sorting by balance ...");
            std::sort(s, e, compare);
        }

        uint64_t nbRestricts = (uint64_t)restrictMap.size();
        if(0==nbRestricts) {
            info("dumping all balances ...");
        } else {
            info("dumping balances for %" PRIu64 " addresses ...", nbRestricts);
        }

        if(false==csv) {

            char endTime[256];
            gmTime(endTime, blockTime);
            printf(
                "---------------------------------------------------------------------------\n"
                "          State of the ledger at block %d (minted : %s)\n"
                "---------------------------------------------------------------------------\n"
                ,
                (int)(curBlock->height),
                endTime
            );

            printf(
                "---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
                "              Balance                      Hash160                             Base58                  nbIn        lastTimeIn          nbOut        lastTimeOut\n"
                "---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
            );
        }

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

            if(csv) {
                printf("%" PRIu64 "\t", addr->sum);
            } else {
                printf("%24.8f ", satoshisToNormaForm(addr->sum));
            }

            if(csv) {
                printEscapedBinaryBuffer(addr->hash.v, kRIPEMD160ByteSize);
                putchar('\t');
            } else {
                showHex(
                    addr->hash.v,
                    kRIPEMD160ByteSize,
                    false
                );
            }

            if(0<addr->sum) {
                ++nonZeroCnt;
            }

            if(csv) {
                printf(
                    "%" PRIu64 "\t%" PRIu32 "\t%" PRIu64 "\t%" PRIu32 "\n",
                    addr->nbIn,
                    addr->lastIn,
                    addr->nbOut,
                    addr->lastOut
                );
            } else {
                if(i<showAddr || 0!=nbRestricts) {
                    uint8_t buf[64];
                    hash160ToAddr(buf, addr->hash.v, true, addr->type);
                    printf(" %s", buf);
                } else {
                    printf(" XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                }

                char timeBuf[256];
                gmTime(timeBuf, addr->lastIn);
                printf(" %6" PRIu64 " %s ", addr->nbIn, timeBuf);

                gmTime(timeBuf, addr->lastOut);
                printf(" %6" PRIu64 " %s\n", addr->nbOut, timeBuf);

                if(detailed) {
                    auto e = addr->outputVec->end();
                    auto s = addr->outputVec->begin();
                    while(s!=e) {
                        printf("    %24.8f ", satoshisToNormaForm(s->value));
                        gmTime(timeBuf, s->time);
                        showHex(s->upTXHash);
                        printf("%4" PRIu64 " %s", s->outputIndex, timeBuf);
                        if(s->downTXHash) {
                            printf(" -> %4" PRIu64 " ", s->inputIndex);
                            showHex(s->upTXHash);
                        }
                        printf("\n");
                        ++s;
                    }
                    printf("\n");
                }
            }

            ++i;
        }

        info("done\n");
        info("found %" PRIu64 " addresses with non zero balance", nonZeroCnt);
        info("found %" PRIu64 " addresses in total", (uint64_t)allAddrs.size());
        info("shown:%" PRIu64 " addresses", (uint64_t)i);
        printf("\n");
    }

    virtual void start(
        const Block *s,
        const Block *e
    )
    {
        firstBlock = s;
        lastBlock = e;
    }

    virtual void startLC() {
        info("computing balance for all addresses");
        info("hit ^C to interrupt and dump current, valid state of ledger");
    }

    virtual void startBlock(
        const Block *b,
        uint64_t chainSize
    ) {
        curBlock = b;

        const uint8_t *p = b->chunk->getData();
        const uint8_t *sz = -4 + p;
        LOAD(uint32_t, size, sz);
        offset += size;

        double now = Timer::usecs();
        static double startTime = 0;
        static double lastStatTime = 0;
        double elapsed = now - lastStatTime;
        bool longEnough = (5*1000*1000<elapsed);
        bool closeEnough = ((chainSize - offset)<80);
        if(unlikely(longEnough || closeEnough)) {

            if(0==startTime) {
                startTime = now;
            }

            info(
                "%8" PRIu64 " blocks, "
                "%8.3f MegaAddrs , "
                "                                            "
                ,
                curBlock->height,
                addrMap.size()*1e-6
            );

            lastStatTime = now;
        }

        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, bTime, p);
        blockTime = bTime;

        if(0<=cutoffBlock && cutoffBlock<=curBlock->height) {
            isDone = true;
        }
    }

};

static AllBalances allBalances;

