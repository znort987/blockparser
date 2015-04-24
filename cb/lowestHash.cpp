
// Compute lowest block hash

#include <util.h>
#include <common.h>
#include <option.h>
#include <callback.h>

struct LowestHash:public Callback
{
    optparse::OptionParser parser;

    const Block *mBestBlock;

    LowestHash()
    : mBestBlock(NULL)
    {
        parser
            .usage("")
            .version("")
            .description("Get the lowest hash ever")
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "lowestHash";  }
    virtual const optparse::OptionParser *optionParser() const { return &parser;       }
    virtual bool                         needTXHash() const    { return false;         }

    virtual void aliases(std::vector<const char*> &v) const
    {
        v.push_back("low");
    }

    virtual void wrapup()
    {
        uint8_t bestHash[2*kSHA256ByteSize + 1];
        toHex(bestHash, mBestBlock->hash);

        printf("\n");
        printf("    lowest hash ever = %s in block %" PRIu64 "\n", bestHash, mBestBlock->height - 1);
        printf("\n");
    }

    virtual void startBlock(const Block *b, uint64_t chainSize)
    {
        if(NULL == mBestBlock)
        {
            mBestBlock = b;
            return;
        }
        for (int i = kSHA256ByteSize - 1; i >= 0; i--)
        {
            if (b->hash[i] < mBestBlock->hash[i])
            {
                mBestBlock = b;
                return;
            }
            if (b->hash[i] > mBestBlock->hash[i])
            {
                return;
            }
        }
    }
};

static LowestHash lowestHash;
