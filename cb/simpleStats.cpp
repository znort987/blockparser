
// Very simple blockchain stats

#include <string>
#include <util.h>
#include <common.h>
#include <callback.h>

static std::string pr128(
    const uint128_t &y
)
{
    static char result[1024];
    char *p = 1023+result;
    *(p--) = 0;

    uint128_t x = y;
    while(1) {
        *(p--) = (char)((x % 10) + '0');
        if(unlikely(0==x)) break;
        x /= 10;
    }
    ++p;

    return std::string(p[0]!='0' ? p : (1022+result==p) ? p : p+1);
}

struct SimpleStats:public Callback
{
    uint128_t volume;
    uint128_t nbBlocks;
    uint128_t nbInputs;
    uint128_t nbOutputs;
    uint128_t nbValidBlocks;
    uint128_t nbTransactions;

    virtual bool needTXHash()
    {
        return false;
    }

    virtual int init(
        int argc,
        char *argv[]
    )
    {
        return 0;
    }

    virtual void startMap(
        const uint8_t *p
    )
    {
        volume = 0;
        nbBlocks = 0;
        nbInputs = 0;
        nbOutputs = 0;
        nbValidBlocks = 0;
        nbTransactions = 0;
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
        volume += value;
    }

    virtual void endMap(
        const uint8_t *p
    )
    {
        printf("\n");
        #define P(x) (pr128(x).c_str())
            printf("    nbBlocks = %s\n", P(nbBlocks));
            printf("    nbValidBlocks = %s\n", P(nbValidBlocks));
            printf("    nbOrphanedBlocks in file = %s\n", P(nbBlocks - nbValidBlocks));
            printf("\n");

            printf("    nbInputs = %s\n", P(nbInputs));
            printf("    nbOutputs = %s\n", P(nbOutputs));
            printf("    nbTransactions = %s\n", P(nbTransactions));
            printf("    volume = %.2f (%s satoshis)\n", volume*1e-8, P(volume)); 
            printf("\n");

            printf("    avg tx per block = %.2f\n", nbTransactions/(double)nbValidBlocks);
            printf("    avg inputs per tx = %.2f\n", nbInputs/(double)nbTransactions);
            printf("    avg outputs per tx = %.2f\n", nbOutputs/(double)nbTransactions);
            printf("    avg output value = %.2f\n", (volume/(double)nbOutputs)*1e-8);
            printf("\n");
        #undef P
    }

    virtual void   startBlock(const uint8_t *p                     ) { ++nbBlocks;      }
    virtual void      startTX(const uint8_t *p, const uint8_t *hash) { ++nbTransactions;}
    virtual void   startInput(const uint8_t *p                     ) { ++nbInputs;      }
    virtual void  startOutput(const uint8_t *p                     ) { ++nbOutputs;     }
    virtual void   startBlock(  const Block *b                     ) { ++nbValidBlocks; }

    virtual const option::Descriptor *usage() const
    {
        return 0;
    }

    virtual const char *name() const
    {
        return "simpleStats";
    }
};

static SimpleStats simpleStats;

