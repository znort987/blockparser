
// Very simple blockchain stats

#include <util.h>
#include <common.h>
#include <option.h>
#include <callback.h>

struct SimpleStats : public Callback {

    optparse::OptionParser parser;

    uint128_t volume;
    uint128_t nbBlocks;
    uint128_t nbInputs;
    uint128_t nbOutputs;
    uint128_t nbBlockFiles;
    uint128_t nbValidBlocks;
    uint128_t nbTransactions;

    SimpleStats() {
        parser
            .usage("")
            .version("")
            .description("gather simple stats about the blockchain")
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "simpleStats"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;       }
    virtual bool                       needUpstream() const    { return false;         }

    virtual void aliases(
        std::vector<const char*> &v
    ) const {
        v.push_back("stats");
    }

    virtual int init(
        int argc,
        const char *argv[]
    ) {
        volume = 0;
        nbBlocks = 0;
        nbInputs = 0;
        nbOutputs = 0;
        nbBlockFiles = 0;
        nbValidBlocks = 0;
        nbTransactions = 0;
        return 0;
    }

    virtual void endOutput(
        const uint8_t *p,
        uint64_t      value,
        const uint8_t *txHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize
    ) {
        volume += value;
    }

    virtual void wrapup() {
        printf("\n");
        #define P(x) (pr128(x).c_str())
            printf("    nbBlocks = %s\n", P(nbBlocks));
            printf("    nbBlockFiles = %s\n", P(nbBlockFiles));
            printf("    nbValidBlocks = %s\n", P(nbValidBlocks));
            printf("    nbOrphanedBlocks in maps = %s\n", P(nbBlocks - nbValidBlocks));
            printf("\n");

            printf("    nbInputs = %s\n", P(nbInputs));
            printf("    nbOutputs = %s\n", P(nbOutputs));
            printf("    nbTransactions = %s\n", P(nbTransactions));
            printf("    volume = %.2f (%s satoshis)\n", satoshisToNormaForm(volume), P(volume)); 
            printf("\n");

            printf("    avg tx per block = %.2f\n", nbTransactions/(double)nbValidBlocks);
            printf("    avg inputs per tx = %.2f\n", nbInputs/(double)nbTransactions);
            printf("    avg outputs per tx = %.2f\n", nbOutputs/(double)nbTransactions);
            printf("    avg output value = %.2f\n", satoshisToNormaForm(volume/(double)nbOutputs));
            printf("\n");
        #undef P
    }

    virtual void startBlockFile(const uint8_t *p                      ) { ++nbBlockFiles;  }
    virtual void      startBlock(const uint8_t *p                     ) { ++nbBlocks;      }
    virtual void         startTX(const uint8_t *p, const uint8_t *hash) { ++nbTransactions;}
    virtual void      startInput(const uint8_t *p                     ) { ++nbInputs;      }
    virtual void     startOutput(const uint8_t *p                     ) { ++nbOutputs;     }
    virtual void      startBlock(const Block *b, uint64_t             ) { ++nbValidBlocks; }
};

static SimpleStats simpleStats;

