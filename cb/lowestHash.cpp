
// Compute lowest block hash

#include <util.h>
#include <common.h>
#include <option.h>
#include <callback.h>

struct LowestHash : public Callback {

    optparse::OptionParser parser;

    const Block *bestBlock;

    LowestHash()
        : bestBlock(NULL)
    {
        parser
            .usage("")
            .version("")
            .description("Compute the lowest block hash in chain")
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "lowestHash";  }
    virtual const optparse::OptionParser *optionParser() const { return &parser;       }
    virtual bool                         needTXHash() const    { return false;         }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("low");
    }

    virtual void wrapup()
    {
        uint8_t bestHash[2*kSHA256ByteSize + 1];
        toHex(bestHash, bestBlock->hash);

        printf("\n");
        printf(
            "    lowest block hash in chain = %s in block %" PRIu64 "\n",
            bestHash,
            bestBlock->height - 1
        );
        printf("\n");
    }

    virtual void startBlock(
        const Block *b,
        uint64_t chainSize
    )
    {
        if(NULL==bestBlock) {
            bestBlock = b;
            return;
        }

        for(int i=kSHA256ByteSize-1; i>=0; --i) {
            if (b->hash[i] < bestBlock->hash[i]) {
                bestBlock = b;
                return;
            }
            if(b->hash[i]> bestBlock->hash[i]) {
                break;
            }
        }
    }
};

static LowestHash lowestHash;

