
// Dump all transactions affecting a specific address

#include <time.h>
#include <util.h>
#include <vector>
#include <common.h>
#include <rmd160.h>
#include <string.h>
#include <errlog.h>
#include <callback.h>

#define CBNAME "transactions"
enum  optionIndex { kUnknown, kCSV };
static const option::Descriptor usageDescriptor[] =
{
    { kUnknown, 0, "",    "", option::Arg::None,                               CBNAME ":\n" },
    { kCSV,     0, "", "csv", option::Arg::None, "--csv      Dump CSV instead of formatted" },
    { 0,        0,  0,     0,                 0,                                          0 }
};

static uint8_t emptyKey[kRIPEMD160ByteSize] = { 0x52 };
typedef GoogMap<Hash160, int, Hash160Hasher, Hash160Equal>::Map AddrMap;

struct Transactions:public Callback
{
    bool csv;
    uint64_t sum;
    uint64_t adds;
    uint64_t subs;
    uint64_t nbTX;
    uint64_t bTime;
    AddrMap addrMap;
    std::vector<uint160_t> rootHashes;

    virtual bool needTXHash()
    {
        return true;
    }

    virtual int init(
        int argc,
        char *argv[]
    )
    {
        sum = 0;
        adds = 0;
        subs = 0;
        nbTX = 0;

        option::Stats  stats(usageDescriptor, argc, argv);
        option::Option *buffer  = new option::Option[stats.buffer_max];
        option::Option *options = new option::Option[stats.options_max];
        option::Parser parse(usageDescriptor, argc, argv, options, buffer);
        if(parse.error()) exit(1);

        csv = (0<options[kCSV].count());

        for(int i=0; i<parse.nonOptionsCount(); ++i) loadKeyList(rootHashes, parse.nonOption(i));
        if(0==rootHashes.size()) errFatal("no addresses to work with");

        auto e = rootHashes.end();
        auto i = rootHashes.begin();
        addrMap.setEmptyKey(emptyKey);
        while(e!=i) {
            const uint160_t &h = *(i++);
            addrMap[h.v] = 1;
        }

        delete [] options;
        delete [] buffer;
        return 0;
    }

    void move(
        const uint8_t *script,
        uint64_t      scriptSize,
        const uint8_t *txHash,
        uint64_t       value,
        bool           add,
        const uint8_t *downTXHash = 0
    )
    {
        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(pubKeyHash.v, script, scriptSize, addrType);
        if(unlikely(type<0))
            return;

        bool match = (addrMap.end() != addrMap.find(pubKeyHash.v));
        if(unlikely(match)) {

            int64_t newSum = sum + value*(add ? 1 : -1);

            if(csv) {
                printf("%10" PRIu64 ", \"", bTime/86400 + 25569);
                showHex(downTXHash ? downTXHash : txHash);
                printf("\", \"");
                showHex(downTXHash ? downTXHash : txHash);
                printf(
                    "\",%17.08f,%17.08f\n",
                    (add ? 1e-8 : -1e-8)*value,
                    newSum*1e-8
                );
            } else {

                struct tm gmTime;
                time_t blockTime = bTime;
                gmtime_r(&blockTime, &gmTime);

                char timeBuf[256];
                asctime_r(&gmTime, timeBuf);

                size_t sz =strlen(timeBuf);
                if(0<sz) timeBuf[sz-1] = 0;

                printf("    %s    ", timeBuf);
                showHex(pubKeyHash.v, kRIPEMD160ByteSize, false);

                printf("    ");
                showHex(downTXHash ? downTXHash : txHash);

                printf(
                    " %24.08f %c %24.08f = %24.08f\n",
                    sum*1e-8,
                    add ? '+' : '-',
                    value*1e-8,
                    newSum*1e-8
                );
            }

            (add ? adds : subs) += value;
            sum = newSum;
            ++nbTX;
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
            value,
            true
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
            value,
            false,
            downTXHash
        );
    }

    virtual void startBlock(
        const Block *b
    )
    {
        const uint8_t *p = b->data;
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        bTime = blkTime;
    }

    virtual void start(
        const Block *,
        const Block *
    )
    {
        if(csv) {
            printf(
                "    \"Time\","
                " \"Address\","
                "                                                          \"TXId\","
                "                                                                   \"TXAmount\","
                "     \"NewBalance\""
                "\n"
            );
        }
        else {
            printf("Dumping all transactions for %d addresse(s)\n\n", (int)addrMap.size());
            printf("    Time (GMT)                  Address                                     Transaction                                                                    OldBalance                     Amount                 NewBalance\n");
            printf("    =======================================================================================================================================================================================================================\n");
        }
    }

    virtual void wrapup()
    {
        if(false==csv) {
            printf(
                "    =======================================================================================================================================================================================================================\n"
                "\n"
                "    transactions  = %" PRIu64 "\n"
                "    received      = %17.08f\n"
                "    spent         = %17.08f\n"
                "    balance       = %17.08f\n"
                "\n",
                nbTX,
                adds*1e-8,
                subs*1e-8,
                sum*1e-8
            );
        }
    }

    virtual const option::Descriptor *usage() const
    {
        return usageDescriptor;
    }

    virtual const char *name() const
    {
        return CBNAME;
    }

    virtual void aliases(
        std::vector<const char*> &v
    )
    {
        v.push_back("txs");
        v.push_back("book");
        v.push_back("tally");
    }
};

static Transactions transactions;

